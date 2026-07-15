module cache #(
    parameter GHR_LEN = 8
)(
    input  wire        clk,
    input  wire        rst,
    input  wire        req_valid,
    input  wire [31:0] pc_addr,
    input  wire        policy_select,

    output wire        hit,
    output wire        miss,
    output reg         victim_way,

    // Contadores de desempenho
    output reg [31:0]  hit_count,
    output reg [31:0]  miss_count
);

    // ============================================================
    // Memória cache
    //
    // 128 conjuntos, 2 vias e 22 bits por linha:
    // [21]    -> valid
    // [20:0]  -> tag
    //
    // ============================================================

    reg [21:0] cache_mem [0:127][0:1];

    // Histórico global: 1 = hit, 0 = miss
    reg [GHR_LEN-1:0] ghr;

    // ============================================================
    // Divisão do endereço
    //
    // pc_addr[3:0]   -> offset
    // pc_addr[10:4]  -> index (7 bits = 128 conjuntos)
    // pc_addr[31:11] -> tag   (21 bits)
    // ============================================================

    wire [6:0]  index;
    wire [20:0] tag;

    assign index = pc_addr[10:4];
    assign tag   = pc_addr[31:11];

    // ============================================================
    // Campos extraídos da cache
    // ============================================================

    wire        valid_way0;
    wire        valid_way1;
    wire [20:0] tag_way0;
    wire [20:0] tag_way1;

    assign valid_way0 = cache_mem[index][0][21];
    assign valid_way1 = cache_mem[index][1][21];

    assign tag_way0 = cache_mem[index][0][20:0];
    assign tag_way1 = cache_mem[index][1][20:0];

    // ============================================================
    // Verificação de hit e miss
    // ============================================================

    wire hit_way0;
    wire hit_way1;
    wire set_full;

    assign hit_way0 = valid_way0 && (tag_way0 == tag);
    assign hit_way1 = valid_way1 && (tag_way1 == tag);

    assign hit  = req_valid && (hit_way0 || hit_way1);
    assign miss = req_valid && !(hit_way0 || hit_way1);

    assign set_full = valid_way0 && valid_way1;

    // ============================================================
    // Índices da tabela do AIRA
    //
    // A tabela possui 64 linhas, portanto o índice possui 6 bits.
    // O bit index[6] é dobrado para dentro dos 6 bits disponíveis.
    //
    // A mesma função deve ser usada:
    //   1. para consultar as linhas nas vias;
    //   2. para treinar com o endereço da requisição atual.
    // ============================================================

    wire [5:0] aira_idx_way0;
    wire [5:0] aira_idx_way1;
    wire [5:0] aira_train_idx;

    assign aira_idx_way0 =
        tag_way0[5:0] ^ index[5:0] ^ {5'b0, index[6]};

    assign aira_idx_way1 =
        tag_way1[5:0] ^ index[5:0] ^ {5'b0, index[6]};

    assign aira_train_idx =
        tag[5:0] ^ index[5:0] ^ {5'b0, index[6]};

    // O AIRA original lê os bits [7:2] dos PCs recebidos.
    // Estes PCs colocam o bits exatamente nessa posição.
    wire [31:0] pc_recon_way0;
    wire [31:0] pc_recon_way1;
    wire [31:0] pc_recon_train;

    assign pc_recon_way0 = {24'b0, aira_idx_way0, 2'b00};
    assign pc_recon_way1 = {24'b0, aira_idx_way1, 2'b00};
    assign pc_recon_train = {24'b0, aira_train_idx, 2'b00};

    // ============================================================
    // Seleção da política e da via de substituição
    // ============================================================

    wire lru_victim_way;
    wire perceptron_victim_way;
    wire policy_victim_way;
    wire selected_way;
    wire aira_req_victim;

    // policy_select:
    // 0 -> LRU
    // 1 -> AIRA
    assign policy_victim_way =
        policy_select ? perceptron_victim_way : lru_victim_way;

    // Primeiro ocupa uma via inválida.
    // A política só decide quando as duas vias estão ocupadas.
    assign selected_way =
        !valid_way0 ? 1'b0 :
        !valid_way1 ? 1'b1 :
                      policy_victim_way;

    // O AIRA só precisa escolher uma vítima em miss com o conjunto cheio.
    assign aira_req_victim =
        miss && set_full && policy_select;

    // ============================================================
    // Módulo LRU
    // ============================================================

    replacement_lru #(
        .NUM_SETS(128)
    ) u_lru (
        .clk        (clk),
        .reset      (rst),
        .access     (req_valid),
        .index      (index),
        .hit        (hit),
        .hit_way0   (hit_way0),
        .hit_way1   (hit_way1),
        .valid_way0 (valid_way0),
        .valid_way1 (valid_way1),
        .victim_way (lru_victim_way),
        .victim_ready()
    );

    // ============================================================
    // Módulo AIRA/perceptron
    // ============================================================

    aira_controller_2way #(
        .WAYS(2),
        .GHR_LEN(GHR_LEN)
    ) u_aira (
        .clk         (clk),
        .rst         (rst),

        .req_victim  (aira_req_victim),
        .pc_way0     (pc_recon_way0),
        .pc_way1     (pc_recon_way1),
        .current_ghr (ghr),
        .victim_way  (perceptron_victim_way),

        // O treinamento usa o mesmo dado usado na predição.
        .train_valid (req_valid),
        .train_pc    (pc_recon_train),
        .train_hit   (hit),
        .train_ghr   (ghr)
    );

    // ============================================================
    // Lógica sequencial
    // ============================================================

    integer i;
    integer w;

    always @(posedge clk) begin
        if (rst) begin

            // Zera valid e tag de todas as linhas.
            for (i = 0; i < 128; i = i + 1) begin
                for (w = 0; w < 2; w = w + 1) begin
                    cache_mem[i][w] <= 22'd0;
                end
            end

            ghr        <= {GHR_LEN{1'b0}};
            victim_way <= 1'b0;
            hit_count  <= 32'd0;
            miss_count <= 32'd0;
        end
        else begin

            // Atualiza contadores somente em requisições válidas.
            if (req_valid) begin
                if (hit)
                    hit_count <= hit_count + 1'b1;
                else
                    miss_count <= miss_count + 1'b1;

                // O AIRA usa o GHR antigo no acesso atual.
                // O resultado atual entra no histórico do próximo acesso.
                ghr <= {ghr[GHR_LEN-2:0], hit};
            end

            // Em caso de miss, insere a nova tag na via selecionada.
            if (miss) begin
                victim_way <= selected_way;

                // Formato da linha: {valid, tag}
                cache_mem[index][selected_way] <= {
                    1'b1,
                    tag
                };
            end
        end
    end

endmodule
