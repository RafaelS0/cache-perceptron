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
    output reg         victim_way
);

    // Cache: 64 conjuntos, 2 vias
    reg valid_way0 [0:63];
    reg valid_way1 [0:63];

    reg [21:0] tag_way0 [0:63];
    reg [21:0] tag_way1 [0:63];

    // PC associado à linha, usado pelo perceptron
    reg [31:0] pc_way0 [0:63];
    reg [31:0] pc_way1 [0:63];

    // Histórico global: 1 = hit, 0 = miss
    reg [GHR_LEN-1:0] ghr;

    wire [5:0]  index;
    wire [21:0] tag;

    wire hit_way0;
    wire hit_way1;
    wire set_full;

    wire lru_victim_way;
    wire perceptron_victim_way;
    wire policy_victim_way;
    wire selected_way;

  
    wire aira_req_victim;

    assign index = pc_addr[9:4];
    assign tag   = pc_addr[31:10];

    assign hit_way0 =
        valid_way0[index] && (tag_way0[index] == tag);

    assign hit_way1 =
        valid_way1[index] && (tag_way1[index] == tag);

    assign hit  = req_valid && (hit_way0 || hit_way1);
    assign miss = req_valid && !hit;

    assign set_full = valid_way0[index] && valid_way1[index];

    // 0 = LRU | 1 = AIRA/perceptron
    assign policy_victim_way =
        policy_select ? perceptron_victim_way : lru_victim_way;

    
// Via inválida sempre tem prioridade
    assign selected_way =
        !valid_way0[index] ? 1'b0 :
        !valid_way1[index] ? 1'b1 :
                             policy_victim_way;

   

    // Só pede decisão ao perceptron em miss com o conjunto cheio.
    assign aira_req_victim =
        miss && set_full && policy_select;

 replacement_lru #(
    .NUM_SETS(64)
) u_lru (
    .clk(clk),
    .reset(rst),

    .access(req_valid),
    .index(index),

    .hit(hit),
    .hit_way0(hit_way0),
    .hit_way1(hit_way1),

    .valid_way0(valid_way0[index]),
    .valid_way1(valid_way1[index]),

    .victim_way(lru_victim_way),
    .victim_ready()
);

    aira_replacement #(
        .GHR_LEN(GHR_LEN),
        .WEIGHT_BITS(8),
        .TABLE_LINES(64),
        .THRESHOLD(29)
    ) u_aira (
        .clk(clk),
        .rst(rst),

        .req_victim(aira_req_victim),
        .pc_way0(pc_way0[index]),
        .pc_way1(pc_way1[index]),
        .current_ghr(ghr),
        .victim_way(perceptron_victim_way),

        .train_valid(req_valid),
        .train_pc(pc_addr),
        .train_hit(hit),
        .train_ghr(ghr)
    );

    integer i;

    always @(posedge clk) begin
        if (rst) begin
            for (i = 0; i < 64; i = i + 1) begin
                valid_way0[i] <= 1'b0;
                valid_way1[i] <= 1'b0;
            end

            ghr        <= {GHR_LEN{1'b0}};
            victim_way <= 1'b0;
        end
        else begin

            // O AIRA usa o GHR anterior no acesso atual.
            // O novo resultado vale para o próximo acesso.
            if (req_valid)
                ghr <= {ghr[GHR_LEN-2:0], hit};

            if (miss) begin
                victim_way <= selected_way;

                if (selected_way == 1'b0) begin
                    valid_way0[index] <= 1'b1;
                    tag_way0[index]   <= tag;
                    pc_way0[index]    <= pc_addr;
                end
                else begin
                    valid_way1[index] <= 1'b1;
                    tag_way1[index]   <= tag;
                    pc_way1[index]    <= pc_addr;
                end
            end
        end
    end

endmodule