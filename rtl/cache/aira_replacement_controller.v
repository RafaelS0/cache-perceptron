module aira_replacement #(
    parameter GHR_LEN      = 8,
    parameter WEIGHT_BITS  = 8,
    parameter TABLE_LINES  = 64,
    parameter THRESHOLD    = 29
)(
    input  wire                   clk,
    input  wire                   rst,

    // Predição: só é usada em miss com as duas vias válidas
    input  wire                   req_victim,
    input  wire [31:0]            pc_way0,
    input  wire [31:0]            pc_way1,
    input  wire [GHR_LEN-1:0]     current_ghr,
    output reg                    victim_way,

    // Treinamento: ocorre a cada acesso válido
    input  wire                   train_valid,
    input  wire [31:0]            train_pc,
    input  wire                   train_hit,
    input  wire [GHR_LEN-1:0]     train_ghr
);

    localparam integer PC_INDEX_BITS = $clog2(TABLE_LINES);
    localparam integer SCORE_BITS =
        WEIGHT_BITS + $clog2(GHR_LEN + 1) + 1;

    localparam signed [WEIGHT_BITS-1:0] WEIGHT_MAX =
        {1'b0, {(WEIGHT_BITS-1){1'b1}}};

    localparam signed [WEIGHT_BITS-1:0] WEIGHT_MIN =
        {1'b1, {(WEIGHT_BITS-1){1'b0}}};

    localparam [SCORE_BITS-1:0] THRESHOLD_EXT = THRESHOLD;

    // 64 entradas; cada entrada tem bias + 8 pesos do GHR
    reg signed [WEIGHT_BITS-1:0]
        weight_table [0:TABLE_LINES-1][0:GHR_LEN];

    // Calcula o score de um PC usando o GHR atual
    function signed [SCORE_BITS-1:0] calc_score;
        input [31:0] pc;
        input [GHR_LEN-1:0] hist;

        integer k;
        reg [PC_INDEX_BITS-1:0] table_index;
        reg signed [SCORE_BITS-1:0] sum;

        begin
            table_index = pc[PC_INDEX_BITS+1:2];
            sum = weight_table[table_index][0]; // bias

            for (k = 0; k < GHR_LEN; k = k + 1) begin
                if (hist[k])
                    sum = sum +
                          $signed(weight_table[table_index][k+1]);
                else
                    sum = sum -
                          $signed(weight_table[table_index][k+1]);
            end

            calc_score = sum;
        end
    endfunction

    wire signed [SCORE_BITS-1:0] score_way0;
    wire signed [SCORE_BITS-1:0] score_way1;

    assign score_way0 = calc_score(pc_way0, current_ghr);
    assign score_way1 = calc_score(pc_way1, current_ghr);

    // Menor score = menor chance de reutilização = vítima.
    // Empate: escolhe via 0.
    always @(*) begin
        victim_way = 1'b0;

        if (req_victim && (score_way1 < score_way0))
            victim_way = 1'b1;
    end

    // ---------- Treinamento ----------

    wire [PC_INDEX_BITS-1:0] train_idx;
    wire signed [SCORE_BITS-1:0] train_score;
    wire predicted_miss;
    wire [SCORE_BITS-1:0] abs_score;
    wire train_error;
    wire do_train;

    assign train_idx      = train_pc[PC_INDEX_BITS+1:2];
    assign train_score    = calc_score(train_pc, train_ghr);

    // Score negativo = previsão de miss.
    assign predicted_miss = train_score[SCORE_BITS-1];

    assign abs_score =
        predicted_miss ? (~train_score + 1'b1) : train_score;

    // predicted_miss: 0 = previu hit, 1 = previu miss
    // train_hit:      1 = hit real,    0 = miss real
    assign train_error = (predicted_miss == train_hit);

    assign do_train =
        train_valid &&
        (train_error || (abs_score <= THRESHOLD_EXT));

    integer i, j;

    always @(posedge clk) begin
        if (rst) begin
            for (i = 0; i < TABLE_LINES; i = i + 1) begin
                for (j = 0; j <= GHR_LEN; j = j + 1) begin
                    weight_table[i][j] <= {WEIGHT_BITS{1'b0}};
                end
            end
        end
        else if (do_train) begin

            // Atualiza bias
            if (train_hit) begin
                if (weight_table[train_idx][0] < WEIGHT_MAX)
                    weight_table[train_idx][0] <=
                        weight_table[train_idx][0] + 1'sd1;
            end
            else begin
                if (weight_table[train_idx][0] > WEIGHT_MIN)
                    weight_table[train_idx][0] <=
                        weight_table[train_idx][0] - 1'sd1;
            end

            // Atualiza os pesos segundo hit/miss e GHR
            for (j = 0; j < GHR_LEN; j = j + 1) begin
                if (train_hit == train_ghr[j]) begin
                    if (weight_table[train_idx][j+1] < WEIGHT_MAX)
                        weight_table[train_idx][j+1] <=
                            weight_table[train_idx][j+1] + 1'sd1;
                end
                else begin
                    if (weight_table[train_idx][j+1] > WEIGHT_MIN)
                        weight_table[train_idx][j+1] <=
                            weight_table[train_idx][j+1] - 1'sd1;
                end
            end
        end
    end

endmodule