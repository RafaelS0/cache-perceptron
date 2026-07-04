module tb_cache_lru;

    reg         clk;
    reg         rst;
    reg         req_valid;
    reg [31:0]  pc_addr;
    reg         policy_select;

    wire        hit;
    wire        miss;
    wire        victim_way;

    // Endereços: todos pertencem ao set 0
    localparam [31:0] ADDR_A = 32'h0000_0000; // tag 0, index 0
    localparam [31:0] ADDR_B = 32'h0000_0400; // tag 1, index 0
    localparam [31:0] ADDR_C = 32'h0000_0800; // tag 2, index 0

    // Sinais auxiliares para facilitar a leitura no GTKWave
    reg [3:0] etapa;
    reg [2:0] repeticao;
    integer k;

    // Instância da cache
    cache dut (
        .clk(clk),
        .rst(rst),
        .req_valid(req_valid),
        .pc_addr(pc_addr),
        .policy_select(policy_select),
        .hit(hit),
        .miss(miss),
        .victim_way(victim_way)
    );

    // Aliases para enxergar explicitamente o set 0 na forma de onda
    wire        valid_way0_set0;
    wire        valid_way1_set0;
    wire [21:0] tag_way0_set0;
    wire [21:0] tag_way1_set0;
    wire        lru_victim_set0;
    wire        policy_victim_set0;

    assign valid_way0_set0  = dut.valid_way0[0];
    assign valid_way1_set0  = dut.valid_way1[0];
    assign tag_way0_set0    = dut.tag_way0[0];
    assign tag_way1_set0    = dut.tag_way1[0];
    assign lru_victim_set0  = dut.lru_victim_way;
    assign policy_victim_set0 = dut.policy_victim_way;

    // Clock: período de 10 ns
    always #5 clk = ~clk;

    // Envia uma requisição válida por um ciclo
    task envia_endereco;
        input [31:0] endereco;
        begin
            @(negedge clk);
            pc_addr   = endereco;
            req_valid = 1'b1;

            @(posedge clk);
            #1;
            req_valid = 1'b0;
        end
    endtask

    initial begin
        $dumpfile("tb_cache_lru.vcd");
        $dumpvars(0, tb_cache_lru);

        clk           = 1'b0;
        rst           = 1'b1;
        req_valid     = 1'b0;
        pc_addr       = 32'h0000_0000;
        policy_select = 1'b0; // 0 = LRU
        etapa         = 4'd0;
        repeticao     = 3'd0;

        // Reset
        #12;
        rst = 1'b0;

        // Etapa 1:
        // Miss compulsório: A entra na via 0
        etapa = 4'd1;
        envia_endereco(ADDR_A);

        // Etapa 2:
        // Miss inicial: B entra na via 1
        etapa = 4'd2;
        envia_endereco(ADDR_B);

        // Etapa 3:
        // Cinco hits em B.
        // B fica como a via mais recentemente usada.
        // A deve permanecer como LRU.
        etapa = 4'd3;
        for (k = 0; k < 5; k = k + 1) begin
            repeticao = k + 1;
            envia_endereco(ADDR_B);
        end

        repeticao = 3'd0;

        // Etapa 4:
        // C causa miss no set cheio.
        // Como B foi acessado por último, o LRU deve escolher a via 0,
        // removendo A e colocando C na via 0.
        etapa = 4'd4;
        envia_endereco(ADDR_C);

        // Etapa final para observar o resultado
        etapa = 4'd5;
        repeat (2) @(posedge clk);

        $finish;
    end

endmodule
