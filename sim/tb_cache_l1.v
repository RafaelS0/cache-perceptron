
`timescale 1ps/1ps

module tb_cache_lru;

    localparam integer GHR_LEN = 8;

    // Deve acompanhar os parâmetros do perceptron:
    // WEIGHT_BITS + clog2(GHR_LEN + 1) + 1
    localparam integer SCORE_BITS =
        8 + $clog2(GHR_LEN + 1) + 1;

    reg         clk;
    reg         rst;
    reg         req_valid;
    reg [31:0]  pc_addr;
    reg         policy_select;

    wire        hit;
    wire        miss;
    wire        victim_way;

    wire [31:0] hit_count;
    wire [31:0] miss_count;

    // Todos pertencem ao conjunto(set) 4
    localparam [31:0] ADDR_A = 32'h0000_0840; // tag 1, index 4
    localparam [31:0] ADDR_B = 32'h0000_1040; // tag 2, index 4
    localparam [31:0] ADDR_C = 32'h0000_1840; // tag 3, index 4

    // Sinais auxiliares para a forma de onda
    reg [3:0] etapa;
    reg [2:0] repeticao;
    integer k;

    // ============================================================
    // Instância da cache
    // ============================================================

    cache #(
        .GHR_LEN(GHR_LEN)
    ) dut (
        .clk           (clk),
        .rst           (rst),
        .req_valid     (req_valid),
        .pc_addr       (pc_addr),
        .policy_select (policy_select),

        .hit           (hit),
        .miss          (miss),
        .victim_way    (victim_way),

        .hit_count     (hit_count),
        .miss_count    (miss_count)
    );

    // ============================================================
    // Conteúdo do conjunto 4
    //
    // Formato atual:
    // cache_mem[set][way][21]    = valid
    // cache_mem[set][way][20:0]  = tag
    // ============================================================

    wire        cache_valid_way0_set4;
    wire        cache_valid_way1_set4;
    wire [20:0] cache_tag_way0_set4;
    wire [20:0] cache_tag_way1_set4;

    assign cache_valid_way0_set4 = dut.cache_mem[4][0][21];
    assign cache_valid_way1_set4 = dut.cache_mem[4][1][21];

    assign cache_tag_way0_set4 = dut.cache_mem[4][0][20:0];
    assign cache_tag_way1_set4 = dut.cache_mem[4][1][20:0];

    // ============================================================
    // Sinais internos do AIRA para visualização
    // ============================================================

    wire signed [SCORE_BITS-1:0] aira_score_way0;
    wire signed [SCORE_BITS-1:0] aira_score_way1;

    wire [5:0] aira_hash_way0;
    wire [5:0] aira_hash_way1;
    wire [5:0] aira_hash_train;

    wire [GHR_LEN-1:0] aira_ghr;

    wire aira_req_victim;
    wire aira_victim_comb;
    wire selected_way_comb;

    assign aira_score_way0 = dut.u_aira.score_w0;
    assign aira_score_way1 = dut.u_aira.score_w1;

    assign aira_hash_way0  = dut.aira_idx_way0;
    assign aira_hash_way1  = dut.aira_idx_way1;
    assign aira_hash_train = dut.aira_train_idx;

    assign aira_ghr = dut.ghr;

    assign aira_req_victim   = dut.aira_req_victim;
    assign aira_victim_comb  = dut.perceptron_victim_way;
    assign selected_way_comb = dut.selected_way;

    // ============================================================
    // Clock: período de 5 ps
    // ============================================================

    always #5 clk = ~clk;

    // ============================================================
    // Envia uma requisição válida durante um ciclo
    // ============================================================

    task envia_endereco;
        input [31:0] endereco;
        begin
            @(negedge clk); //durante a borda de descida

            pc_addr   = endereco;
            req_valid = 1'b1;

            @(posedge clk);
            #1;

            req_valid = 1'b0;
        end
    endtask

    // ============================================================
    // Teste
    // ============================================================

    initial begin
        $dumpfile("tb_cache_aira.vcd");
        $dumpvars(0, tb_cache_lru);

        clk           = 1'b0;
        rst           = 1'b1;
        req_valid     = 1'b0;
        pc_addr       = 32'h0000_0000;
        policy_select = 1'b1; // 0 = LRU, 1 = AIRA

        etapa     = 4'd0;
        repeticao = 3'd0;

        // Reset
        #12;
        rst = 1'b0;

        // Etapa 1:
        // A causa miss e entra na via 0
        etapa = 4'd1;
        envia_endereco(ADDR_A);

        // Etapa 2:
        // B causa miss e entra na via 1
        etapa = 4'd2;
        envia_endereco(ADDR_B);

        // Etapa 3:
        // Cinco hits em B para treinar o AIRA
        etapa = 4'd3;

        for (k = 0; k < 5; k = k + 1) begin
            repeticao = k + 1;
            envia_endereco(ADDR_B);
        end

        repeticao = 3'd0;

        // Etapa 4:
        // C causa miss com o conjunto cheio.
        // O AIRA compara os scores de A e B.
        etapa = 4'd4;
        envia_endereco(ADDR_C);

        // Espera para observar o resultado
      
    end

endmodule
```

