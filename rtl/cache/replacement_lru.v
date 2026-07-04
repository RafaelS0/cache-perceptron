module replacement_lru #(
    parameter NUM_SETS = 64
)(
    input  wire clk,
    input  wire reset,
    input  wire access,      // 1 = houve requisição da cpu nesse ciclo (cpu_req)
    input  wire [$clog2(NUM_SETS)-1:0] index,
    input  wire hit,
    input  wire hit_way0,
    input  wire hit_way1,
    input  wire valid_way0,  // valid[index][0], já decodificado no set atual
    input  wire valid_way1,  // valid[index][1]
    output reg  victim_way,     // via escolhida em caso de miss
    output wire victim_ready    // LRU decide em 1 ciclo -> sempre 1
);

    // lru[set] = 0 -> via 0 é a vítima
    // lru[set] = 1 -> via 1 é a vítima
    reg lru [0:NUM_SETS-1];

    assign victim_ready = 1'b1; // LRU de 2 vias é combinacional, nunca espera

    // Escolha de vítima (combinacional, mesmo critério que já estava no cache_l1)
    always @(*) begin
        if (!valid_way0)
            victim_way = 1'b0;
        else if (!valid_way1)
            victim_way = 1'b1;
        else
            victim_way = lru[index];
    end

    integer i;
    always @(posedge clk) begin
        if (reset) begin
            for (i = 0; i < NUM_SETS; i = i + 1)
                lru[i] <= 1'b0;
        end
        else if (access) begin
            if (hit) begin
                // Atualiza LRU: a via que acabou de dar hit vira MRU,
                // então a OUTRA via passa a ser a próxima vítima
                if (hit_way0)
                    lru[index] <= 1'b1;
                else if (hit_way1)
                    lru[index] <= 1'b0;
            end
            else begin
                // Atualiza LRU: a via recém-alocada (victim_way) vira MRU
                if (victim_way == 1'b0)
                    lru[index] <= 1'b1;
                else
                    lru[index] <= 1'b0;
            end
        end
    end

endmodule
