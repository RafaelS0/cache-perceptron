module cache_l1 #(
    parameter ADDR_WIDTH   = 32, //tamanho do endereço
    parameter BLOCK_SIZE   = 32,
    parameter NUM_WAYS     = 2,   //// capacidade = SETS_64 × WAYS_2 × BLOCK_32 = 4096 bytes = 4 KB
    parameter NUM_SETS     = 64,
    parameter TAG_WIDTH    = 21, // 21 bits de TAG
    parameter INDEX_WIDTH  = 6,  // 6 bits de INDEX
    parameter OFFSET_WIDTH = 5   // 5 bits de offset
)(
    input wire clk,
    input wire reset,

    input wire cpu_req, // se 1 o endereço e´ lido, se 0 don't care
    input wire [ADDR_WIDTH-1:0] cpu_addr, //declarando cpu_addr com 32 bits

    output reg cpu_hit,  // 1 = hit 0= miss
    output reg cpu_ready // se a cache terminou de acessar
);

    // Campos do endereço
    wire [INDEX_WIDTH-1:0] index;  // criando index com 6 bits
    wire [TAG_WIDTH-1:0] tag; // criando tag com 21 bits

    assign index = cpu_addr[OFFSET_WIDTH + INDEX_WIDTH - 1 : OFFSET_WIDTH]; // index recebe dados da cpu_addr
    assign tag   = cpu_addr[ADDR_WIDTH-1 : OFFSET_WIDTH + INDEX_WIDTH]; // tag recebe os valores da cpu_addr

    // Metadados da cache
    reg valid [0:NUM_SETS-1][0:NUM_WAYS-1];
    reg [TAG_WIDTH-1:0] tag_array [0:NUM_SETS-1][0:NUM_WAYS-1];

    // LRU para 2 vias
    // lru[set] = 0 -> via 0 é a vítima
    // lru[set] = 1 -> via 1 é a vítima
    reg lru [0:NUM_SETS-1];

    // Sinais auxiliares
    wire hit_way0;
    wire hit_way1;
    wire hit;

    assign hit_way0 = valid[index][0] && (tag_array[index][0] == tag);//recebe 1 se via 0 do set escolhido esta valida E a tag armazenada nela for igual a tag do end 
    assign hit_way1 = valid[index][1] && (tag_array[index][1] == tag);// mesma coisa so que para via 1
    assign hit = hit_way0 || hit_way1; 

    reg victim_way;

    integer i, j;

    always @(posedge clk or posedge reset) begin
       /////////////////////////RESET/////////////////////////
	 if (reset) begin
            cpu_hit   <= 1'b0;
            cpu_ready <= 1'b0;
            victim_way <= 1'b0;

            for (i = 0; i < NUM_SETS; i = i + 1) begin
                lru[i] <= 1'b0;

                for (j = 0; j < NUM_WAYS; j = j + 1) begin
                    valid[i][j] <= 1'b0;
                    tag_array[i][j] <= {TAG_WIDTH{1'b0}};
                end
            end
        end/////////////////////////RESET/////////////////////////
        else begin
            cpu_ready <= 1'b0; // cpu = 0 
            cpu_hit   <= 1'b0; // hit = 0

            if (cpu_req) begin //se for 1 - cpu recebeu uma requesiçao 
                cpu_ready <= 1'b1; // cpu = 1 - cpu fica pronta

                // HIT
                if (hit) begin // se o acesso for um hit
                    cpu_hit <= 1'b1;

                    // Atualiza LRU
                    if (hit_way0) begin
                        lru[index] <= 1'b1; // via 1 vira vítima
                    end
                    else if (hit_way1) begin
                        lru[index] <= 1'b0; // via 0 vira vítima
                    end
                end

                // MISS
                else begin //se for um miss
                    cpu_hit <= 1'b0; 

                    // Escolhe vítima
                    if (!valid[index][0]) begin
                        victim_way = 1'b0;
                    end
                    else if (!valid[index][1]) begin
                        victim_way = 1'b1;
                    end
                    else begin
                        victim_way = lru[index];
                    end

                    // Substitui bloco
                    valid[index][victim_way] <= 1'b1;
                    tag_array[index][victim_way] <= tag;

                    // Atualiza LRU
                    if (victim_way == 1'b0) begin
                        lru[index] <= 1'b1;
                    end
                    else begin
                        lru[index] <= 1'b0;
                    end
                end
            end
        end
    end

endmodule
