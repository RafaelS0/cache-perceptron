`timescale 1ps/1ps


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
	localparam [31:0] ADDR_A = 32'h0000_0840; // tag 1, index 4
	localparam [31:0] ADDR_B = 32'h0000_1040; // tag 2, index 4
	localparam [31:0] ADDR_C = 32'h0000_1840; // tag 3, index 4

    // Sinais auxiliares para facilitar a leitura na forma de onda
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
	wire        cache_valid_way0_set4;
	wire        cache_valid_way1_set4;
	wire [20:0] cache_tag_way0_set4;
	wire [20:0] cache_tag_way1_set4;
	wire [31:0] cache_pc_way0_set4;
	wire [31:0] cache_pc_way1_set4;

	assign cache_valid_way0_set4 = dut.cache_mem[4][0][53];
	assign cache_valid_way1_set4 = dut.cache_mem[4][1][53];

	assign cache_tag_way0_set4 = dut.cache_mem[4][0][52:32];
	assign cache_tag_way1_set4 = dut.cache_mem[4][1][52:32];

	assign cache_pc_way0_set4 = dut.cache_mem[4][0][31:0];
	assign cache_pc_way1_set4 = dut.cache_mem[4][1][31:0];

    // Clock: período de 10 ps
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
        $dumpvars;

        clk           = 1'b0;
        rst           = 1'b1;
        req_valid     = 1'b0;
        pc_addr       = 32'h0000_0000;
        policy_select = 1'b1; // 0 = LRU
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
        // Cinco hits em B. Aumentando ou SCORE / sendo a mais recentemente usada
        // A fica como a via menos recentemente usada / menor score
        
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

    end

endmodule
