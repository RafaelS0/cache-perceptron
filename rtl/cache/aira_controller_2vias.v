
module aira_controller_2way #(
    parameter WAYS         = 2,
    parameter GHR_LEN      = 8,
    parameter WEIGHT_BITS  = 8,
    parameter TABLE_LINES  = 64,  // 64 linhas (usa 6 bits do PC)
    parameter THRESHOLD    = 29
)(
    input  wire                   clk,
    input  wire                   rst,
    
    // Interface de Predicao (ocorre no MISS)
    input  wire                   req_victim,      
    input  wire [31:0]            pc_way0,         // PC armazenado na Via 0
    input  wire [31:0]            pc_way1,         // PC armazenado na Via 1
    input  wire [GHR_LEN-1:0]     current_ghr,     // GHR atual
    output reg                    victim_way,      // Qual via expulsar (0 ou 1)
    
    // Interface de Treinamento (ocorre a cada acesso)
    input  wire                   train_valid,     // Habilita o treinamento
    input  wire [31:0]            train_pc,        // PC da instrucao que gerou o acesso
    input  wire                   train_hit,       // 1 = Acertou, 0 = Errou
    input  wire [GHR_LEN-1:0]     train_ghr        // GHR no momento do acesso
);

    // TABELA DE PESOS (Memoria de Registradores)
    // [Linhas][0:GHR_LEN]. O indice 0 eh o Bias. 1 a GHR_LEN sao os pesos
    reg signed [WEIGHT_BITS-1:0] weight_table [0:TABLE_LINES-1][0:GHR_LEN];

    integer i, j;
    
    // FUNCAO COMBINACIONAL: CALCULO DE SCORE (u)
    // bloco combinacional (always @*) para calcular a arvore de somas
    reg signed [WEIGHT_BITS+3:0] score_w0, score_w1;
    wire [5:0] idx_w0 = pc_way0[7:2]; // Isola bits para indexar a tabela (ex: 6 bits = 64 posicoes)
    wire [5:0] idx_w1 = pc_way1[7:2];

    always @(*) begin
        // Inicializa com o Bias
        score_w0 = weight_table[idx_w0][0];
        score_w1 = weight_table[idx_w1][0];
        
        // Arvore de soma XNOR com o GHR
        for (i = 0; i < GHR_LEN; i = i + 1) begin
            score_w0 = score_w0 + (current_ghr[i] ? weight_table[idx_w0][i+1] : -weight_table[idx_w0][i+1]);
            score_w1 = score_w1 + (current_ghr[i] ? weight_table[idx_w1][i+1] : -weight_table[idx_w1][i+1]);
        end
    end

    // SELETOR DE VITIMA (Acha o menor Score)
    always @(*) begin
        if (req_victim) begin
            victim_way = 1'b0;
            if (score_w1 < score_w0) victim_way = 1'b1;
        end else begin
            victim_way = 1'b0;
        end
    end

    // LOGICA DE TREINAMENTO (Sincrono com o Clock)
    wire [5:0] train_idx = train_pc[7:2];
    
    // Calculo do score para ver se a confiança foi baixa
    reg signed [WEIGHT_BITS+3:0] train_score;
    always @(*) begin
        train_score = weight_table[train_idx][0];
        for (i = 0; i < GHR_LEN; i = i + 1) begin
            train_score = train_score + (train_ghr[i] ? weight_table[train_idx][i+1] : -weight_table[train_idx][i+1]);
        end
    end
    
    wire pred_bit = train_score[WEIGHT_BITS+3]; // Sinal: 0=Hit, 1=Miss
    wire [WEIGHT_BITS+3:0] abs_score = pred_bit ? (~train_score + 1) : train_score;
    wire error = (pred_bit == train_hit); // XNOR: errou a predicao
    wire do_train = train_valid & (error | (abs_score <= THRESHOLD));

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            for (i = 0; i < TABLE_LINES; i = i + 1) begin
                for (j = 0; j <= GHR_LEN; j = j + 1) begin
                    weight_table[i][j] <= 0;
                end
            end
        end 
        else if (do_train) begin
            // Atualiza Bias
            if (train_hit) begin
                if (weight_table[train_idx][0] != 8'sd127) weight_table[train_idx][0] <= weight_table[train_idx][0] + 1;
            end else begin
                if (weight_table[train_idx][0] != -8'sd128) weight_table[train_idx][0] <= weight_table[train_idx][0] - 1;
            end
            
            // Atualiza Pesos
            for (j = 0; j < GHR_LEN; j = j + 1) begin
                if (train_hit == train_ghr[j]) begin
                    if (weight_table[train_idx][j+1] != 8'sd127) weight_table[train_idx][j+1] <= weight_table[train_idx][j+1] + 1;
                end else begin
                    if (weight_table[train_idx][j+1] != -8'sd128) weight_table[train_idx][j+1] <= weight_table[train_idx][j+1] - 1;
                end
            end
        end
    end

endmodule