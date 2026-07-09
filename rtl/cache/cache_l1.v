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

   
    // ============================================================\\
    // 			  Memoria cache
    reg [53:0] cache_mem [0:127][0:1];  // cache_mem[128 sets][2 vias], 54 bits por linha
    // ============================================================\\
   

    // Histórico global: 1 = hit, 0 = miss
    reg [GHR_LEN-1:0] ghr;

    // Separação do endereço
    wire [6:0]  index;
    wire [20:0] tag;

    // Campos extraídos da cache
    wire        valid_way0;
    wire        valid_way1;

    wire [20:0] tag_way0;
    wire [20:0] tag_way1;

    wire [31:0] pc_way0;
    wire [31:0] pc_way1;

    // Sinais de controle
    wire hit_way0;
    wire hit_way1;
    wire set_full;

    wire lru_victim_way;
    wire perceptron_victim_way;
    wire policy_victim_way;
    wire selected_way;

    wire aira_req_victim;

    // ============================================================\\
    // 			  Divisão do endereço
    //
    // pc_addr[3:0]   -> offset
    // pc_addr[10:4]  -> index
    // pc_addr[31:11] -> tag
    //
    // offset = 4 bits
    // index  = 7 bits
    // tag    = 21 bits
    //============================================================\\

    assign index = pc_addr[10:4];
    assign tag   = pc_addr[31:11];

    // ============================================================\\
    // 			Extração dos campos da cache
    //
    // cache_mem[set][way][bits]
    //
    // [53]    -> valid
    // [52:32] -> tag
    // [31:0]  -> pc
    // ============================================================\\

    assign valid_way0 = cache_mem[index][0][53];
    assign valid_way1 = cache_mem[index][1][53];

    assign tag_way0 = cache_mem[index][0][52:32];
    assign tag_way1 = cache_mem[index][1][52:32];

    assign pc_way0 = cache_mem[index][0][31:0];
    assign pc_way1 = cache_mem[index][1][31:0];

    // ============================================================\\
    // 			Verificação de hit/miss/way
    // ============================================================\\

    assign hit_way0 =
        valid_way0 && (tag_way0 == tag);

    assign hit_way1 =
        valid_way1 && (tag_way1 == tag);

    assign hit  = req_valid && (hit_way0 || hit_way1);
    assign miss = req_valid && !hit;

    assign set_full = valid_way0 && valid_way1; 
  
                         
    assign policy_victim_way = policy_select ? perceptron_victim_way : lru_victim_way; // se 0 LRU 1 AIRA

    // Tenta alocar primeiro em uma via inválida/vazia
    assign selected_way =
        !valid_way0 ? 1'b0 :
        !valid_way1 ? 1'b1 :
                      policy_victim_way;

    // Se for um miss e o set estiver cheio chama politica de substituiçao
    assign aira_req_victim =
        miss && set_full && policy_select;

    // ============================================================\\
    //			      Módulo LRU
    // ============================================================\\

    replacement_lru #(
        .NUM_SETS(128)
    ) u_lru (
        .clk(clk),
        .reset(rst),

        .access(req_valid),
        .index(index),

        .hit(hit),
        .hit_way0(hit_way0),
        .hit_way1(hit_way1),

        .valid_way0(valid_way0),
        .valid_way1(valid_way1),

        .victim_way(lru_victim_way),
        .victim_ready()
    );

    // ============================================================\\
    // 			Módulo AIRA/perceptron
    // ============================================================\\

    aira_replacement #(
        .GHR_LEN(GHR_LEN),
        .WEIGHT_BITS(8),
        .TABLE_LINES(128),
        .THRESHOLD(29)
    ) u_aira (
        .clk(clk),
        .rst(rst),

        .req_victim(aira_req_victim),
        .pc_way0(pc_way0),
        .pc_way1(pc_way1),
        .current_ghr(ghr),
        .victim_way(perceptron_victim_way),

        .train_valid(req_valid),
        .train_pc(pc_addr),
        .train_hit(hit),
        .train_ghr(ghr)
    );

    //============================================================\\
    // 			Lógica sequencial
    //============================================================\\

    integer i;
    integer w;

    always @(posedge clk) begin
        if (rst) begin

            // Zera todos os metadados da cache
            for (i = 0; i < 128; i = i + 1) begin
                for (w = 0; w < 2; w = w + 1) begin
                    cache_mem[i][w] <= 54'd0;
                end
            end

            ghr        <= {GHR_LEN{1'b0}};
            victim_way <= 1'b0;

            // Zera contadores
            hit_count  <= 32'd0;
            miss_count <= 32'd0;
        end
        else begin

            // Atualiza contadores apenas quando existe uma requisição válida
            if (req_valid) begin
                if (hit) begin
                    hit_count <= hit_count + 1'b1;
                end
                else begin
                    miss_count <= miss_count + 1'b1;
                end
            end

            // O AIRA usa o GHR antigo no acesso atual.
            // O novo resultado entra no histórico para o próximo acesso.
            if (req_valid) begin
                ghr <= {ghr[GHR_LEN-2:0], hit};
            end

            // Em caso de miss, insere o novo endereço na via selecionada
            if (miss) begin
                victim_way <= selected_way;

                // Formato da linha:
                // {valid, tag, pc}
                //
                // 1'b1    -> bit [53]
                // tag     -> bits [52:32]
                // pc_addr -> bits [31:0]
                cache_mem[index][selected_way] <= {1'b1, tag, pc_addr};
            end
        end
    end

endmodule