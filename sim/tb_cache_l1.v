`timescale 1ps/1ps

module tb_cache_l1;

    localparam ADDR_WIDTH   = 32;
    localparam CACHE_SIZE   = 4096;
    localparam BLOCK_SIZE   = 32;
    localparam NUM_WAYS     = 2;
    localparam NUM_SETS     = 64;
    localparam TAG_WIDTH    = 21;
    localparam INDEX_WIDTH  = 6;
    localparam OFFSET_WIDTH = 5;

    reg clk;
    reg reset;

    reg cpu_req;
    reg [ADDR_WIDTH-1:0] cpu_addr;

    wire cpu_hit;
    wire cpu_ready;

    // Sinais auxiliares para waveform
    reg [1:0] instr_id;
    reg [7:0] loop_counter;
    reg [31:0] current_addr;
    reg [31:0] current_pc;

    /*
        instr_id:
        0 = ACCESS HOT A
        1 = ACCESS HOT A de novo
        2 = ACCESS STREAM
    */

    cache_l1 #(
        .ADDR_WIDTH(ADDR_WIDTH),
        .CACHE_SIZE(CACHE_SIZE),
        .BLOCK_SIZE(BLOCK_SIZE),
        .NUM_WAYS(NUM_WAYS),
        .NUM_SETS(NUM_SETS),
        .TAG_WIDTH(TAG_WIDTH),
        .INDEX_WIDTH(INDEX_WIDTH),
        .OFFSET_WIDTH(OFFSET_WIDTH)
    ) dut (
        .clk(clk),
        .reset(reset),

        .cpu_req(cpu_req),
        .cpu_addr(cpu_addr),

        .cpu_hit(cpu_hit),
        .cpu_ready(cpu_ready)
    );

    always #5 clk = ~clk;

    task access_cache;
        input [1:0] op_id;
        input [31:0] addr;
        input [31:0] pc;
        begin
            instr_id     <= op_id;
            current_addr <= addr;
            current_pc   <= pc;

            @(posedge clk);
            cpu_req  <= 1'b1;
            cpu_addr <= addr;

            @(posedge clk);
            cpu_req <= 1'b0;

            wait(cpu_ready == 1'b1);

            @(posedge clk);
        end
    endtask

    integer k;

    initial begin
        clk          = 1'b0;
        reset        = 1'b1;
        cpu_req      = 1'b0;
        cpu_addr     = 32'b0;

        instr_id     = 2'b00;
        loop_counter = 8'b0;
        current_addr = 32'b0;
        current_pc   = 32'b0;

        repeat (4) @(posedge clk);
        reset = 1'b0;
        repeat (2) @(posedge clk);

        /*
            Sequência:
            A = endereço quente
            STREAM = endereços diferentes no mesmo set

            Todos os STREAMs abaixo caem no set 0:
            0x00000800
            0x00001000
            0x00001800
            ...
        */

        for (k = 0; k < 8; k = k + 1) begin
            loop_counter <= k[7:0];

            // I0: acesso quente A
            access_cache(
                2'd0,
                32'h00000000,
                32'h00400000
            );

            // I1: acesso quente A novamente
            access_cache(
                2'd1,
                32'h00000000,
                32'h00400004
            );

            // I2: acesso streaming no mesmo set
            access_cache(
                2'd2,
                32'h00000800 + (k * 32'h00000800),
                32'h00400008
            );
        end

        #50;
        $stop;
    end

endmodule
