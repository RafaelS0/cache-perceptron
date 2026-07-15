`timescale 1ps/1ps

module tb_cache_benchmarks;

    localparam integer GHR_LEN    = 8;
    localparam integer ARRAY_SIZE = 1024;

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

    integer hits_lru;
    integer misses_lru;
    integer hits_aira;
    integer misses_aira;

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

    // Clock de 10 ps
    always #5 clk = ~clk;

    // ============================================================
    // Reset da cache
    // ============================================================

    task reset_cache;
        begin
            @(negedge clk);

            rst       = 1'b1;
            req_valid = 1'b0;
            pc_addr   = 32'd0;

            
            repeat (2) @(posedge clk);

            @(negedge clk);
            rst = 1'b0;
        end
    endtask

    // ============================================================
    // Um acesso à cache
    // ============================================================

    task cache_access;
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

    // ============================================================
    // Benchmark 1: streaming
    // ============================================================

    task benchmark_streaming;
        integer it;
        integer i;

        reg [31:0] base_array;
        reg [31:0] base_hot;
        begin
            base_array = 32'h0001_0000;
            base_hot   = 32'h0002_0000;

            for (it = 0; it < 10; it = it + 1) begin
                for (i = 0; i < ARRAY_SIZE; i = i + 1) begin

                    // Acesso sequencial ao array
                    cache_access(base_array + (i * 4));

                    // Dado reutilizado periodicamente
                    if ((i % 64) == 0) begin
                        cache_access(base_hot);
                    end
                end
            end
        end
    endtask

    // ============================================================
    // Benchmark 2: convolução de matriz
    // ============================================================

    task benchmark_matrix_conv;
        integer x;
        integer y;
        integer width;
        integer height;

        reg [31:0] base_img;
        reg [31:0] base_out;
        begin
            base_img = 32'h0003_0000;
            base_out = 32'h0008_0000;

            width  = 128;
            height = ARRAY_SIZE / width;

            for (y = 1; y < height - 1; y = y + 1) begin
                for (x = 1; x < width - 1; x = x + 1) begin

                    cache_access(
                        base_img +
                        ((((y - 1) * width) + x) * 4)
                    );

                    cache_access(
                        base_img +
                        (((y * width) + x) * 4)
                    );

                    cache_access(
                        base_img +
                        ((((y + 1) * width) + x) * 4)
                    );

                    cache_access(
                        base_out +
                        (((y * width) + x) * 4)
                    );
                end
            end
        end
    endtask

    // ============================================================
    // Execução
    // ============================================================

    initial begin
        $dumpfile("tb_cache_benchmarks.vcd");
        $dumpvars(0, tb_cache_benchmarks);

        clk           = 1'b0;
        rst           = 1'b0;
        req_valid     = 1'b0;
        pc_addr       = 32'd0;
        policy_select = 1'b0;

        // --------------------------------------------------------
        // Streaming com LRU
        // --------------------------------------------------------

        policy_select = 1'b0;
        reset_cache;
        benchmark_streaming;

        hits_lru   = hit_count;
        misses_lru = miss_count;

        $display("======================================");
        $display("STREAMING - LRU");
        $display("Hits   = %0d", hits_lru);
        $display("Misses = %0d", misses_lru);
        $display("Total  = %0d", hits_lru + misses_lru);

        // --------------------------------------------------------
        // Streaming com AIRA
        // --------------------------------------------------------

        policy_select = 1'b1;
        reset_cache;
        benchmark_streaming;

        hits_aira   = hit_count;
        misses_aira = miss_count;

        $display("--------------------------------------");
        $display("STREAMING - AIRA");
        $display("Hits   = %0d", hits_aira);
        $display("Misses = %0d", misses_aira);
        $display("Total  = %0d", hits_aira + misses_aira);

        // --------------------------------------------------------
        // Matriz com LRU
        // --------------------------------------------------------

        policy_select = 1'b0;
        reset_cache;
        benchmark_matrix_conv;

        hits_lru   = hit_count;
        misses_lru = miss_count;

        $display("======================================");
        $display("MATRIX CONV - LRU");
        $display("Hits   = %0d", hits_lru);
        $display("Misses = %0d", misses_lru);
        $display("Total  = %0d", hits_lru + misses_lru);

        // --------------------------------------------------------
        // Matriz com AIRA
        // --------------------------------------------------------

        policy_select = 1'b1;
        reset_cache;
        benchmark_matrix_conv;

        hits_aira   = hit_count;
        misses_aira = miss_count;

        $display("--------------------------------------");
        $display("MATRIX CONV - AIRA");
        $display("Hits   = %0d", hits_aira);
        $display("Misses = %0d", misses_aira);
        $display("Total  = %0d", hits_aira + misses_aira);
        $display("======================================");

        $finish;
    end

endmodule
