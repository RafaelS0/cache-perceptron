module l1_cache #(
    parameter ADDR_WIDTH   = 32,   // largura de endereço, em bits
    parameter DATA_WIDTH   = 32,   // largura da palavra de dados, em bits
    parameter CACHE_SIZE   = 4096, // tamanho total da cache em bytes
    parameter BLOCK_SIZE   = 32,   // tamanho de cada bloco, em bytes
    parameter NUM_WAYS     = 2,    // número de vias
    parameter NUM_SETS     = 64,   // número de conjuntos
    parameter TAG_WIDTH    = 21,   // tamanho da tag, em bits
    parameter INDEX_WIDTH  = 6,    // tamanho do index, em bits
    parameter OFFSET_WIDTH = 5     // tamanho do offset, em bits
)(
    input wire clk,
    input wire reset,

    input wire cpu_req,  // processador pedindo acesso
    input wire cpu_we,   // 0 = leitura, 1 = escrita
    input wire [ADDR_WIDTH-1:0] cpu_addr,  // endereço acessado pela CPU
    input wire [DATA_WIDTH-1:0] cpu_wdata, // dado para escrever

    output reg cpu_ready, // cache terminou a requisição
    output reg cpu_hit,   // 0 = miss, 1 = hit
    output reg [DATA_WIDTH-1:0] cpu_rdata // valor lido pela CPU
);

    // Campos do endereço
    wire [OFFSET_WIDTH-1:0] offset;
    wire [INDEX_WIDTH-1:0] index;
    wire [TAG_WIDTH-1:0] tag;

    assign offset = cpu_addr[OFFSET_WIDTH-1:0];
    assign index  = cpu_addr[OFFSET_WIDTH + INDEX_WIDTH - 1 : OFFSET_WIDTH];
    assign tag    = cpu_addr[ADDR_WIDTH-1 : OFFSET_WIDTH + INDEX_WIDTH];

    // Metadados da cache
    reg valid [0:NUM_SETS-1][0:NUM_WAYS-1];
    reg dirty [0:NUM_SETS-1][0:NUM_WAYS-1];
    reg [TAG_WIDTH-1:0] tag_array [0:NUM_SETS-1][0:NUM_WAYS-1];

    // Dados da cache: data_array[set][way][byte do bloco]
    reg [7:0] data_array [0:NUM_SETS-1][0:NUM_WAYS-1][0:BLOCK_SIZE-1];

    // LRU para cache 2-vias
    // Convenção:
    // lru[set] = 0 -> via 0 é vítima
    // lru[set] = 1 -> via 1 é vítima
    reg lru [0:NUM_SETS-1];

    // Lógica de hit
    wire hit_way0;
    wire hit_way1;
    wire hit;

    assign hit_way0 = valid[index][0] && (tag_array[index][0] == tag);
    assign hit_way1 = valid[index][1] && (tag_array[index][1] == tag);
    assign hit = hit_way0 || hit_way1;

    // Estados da FSM
    localparam IDLE   = 2'b00;
    localparam LOOKUP = 2'b01;
    localparam MISS   = 2'b10;
    localparam REFILL = 2'b11;

    reg [1:0] state;

    // Como NUM_WAYS = 2, 1 bit basta para escolher a via
    reg victim_way;

    integer i, j;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            
  	    cpu_ready <= 1'b0; //atribuindo 0
            cpu_hit   <= 1'b0; // atribuindo 0
            cpu_rdata <= {DATA_WIDTH{1'b0}};  // atribuindo 0
            state     <= IDLE;  // atribuindo 0
            victim_way <= 1'b0;  // atribuindo 0

            //zerando todos valores do LRU
            for (i = 0; i < NUM_SETS; i = i + 1) begin
                lru[i] <= 1'b0;

                //zerando todos os valores de VIA
                for (j = 0; j < NUM_WAYS; j = j + 1) begin
                    valid[i][j] <= 1'b0;
                    dirty[i][j] <= 1'b0;
                    tag_array[i][j] <= {TAG_WIDTH{1'b0}};
                end
            end
        end

        else begin
            case (state)

                IDLE: begin //PARADA
                    cpu_ready <= 1'b0;
                    cpu_hit   <= 1'b0;

                    if (cpu_req) begin
                        state <= LOOKUP;
                    end
                end

                LOOKUP: begin //Iniciando BUSCA
                    if (hit) begin
                        cpu_ready <= 1'b1;
                        cpu_hit   <= 1'b1;

                        if (!cpu_we) begin
                            // Leitura
                            if (hit_way0) begin
                                cpu_rdata <= {
                                    data_array[index][0][offset + 3],
                                    data_array[index][0][offset + 2],
                                    data_array[index][0][offset + 1],
                                    data_array[index][0][offset]
                                };

                                // Via 0 foi usada, então via 1 vira menos recente
                                lru[index] <= 1'b1;
                            end
                            else if (hit_way1) begin
                                cpu_rdata <= {
                                    data_array[index][1][offset + 3],
                                    data_array[index][1][offset + 2],
                                    data_array[index][1][offset + 1],
                                    data_array[index][1][offset]
                                };

                                // Via 1 foi usada, então via 0 vira menos recente
                                lru[index] <= 1'b0;
                            end
                        end
                        else begin

                            // Escrita em hit
                            if (hit_way0) begin
                                data_array[index][0][offset]     <= cpu_wdata[7:0];
                                data_array[index][0][offset + 1] <= cpu_wdata[15:8];
                                data_array[index][0][offset + 2] <= cpu_wdata[23:16];
                                data_array[index][0][offset + 3] <= cpu_wdata[31:24];

                                dirty[index][0] <= 1'b1;
                                lru[index] <= 1'b1;
                            end

                            else if (hit_way1) begin
                                data_array[index][1][offset]     <= cpu_wdata[7:0];
                                data_array[index][1][offset + 1] <= cpu_wdata[15:8];
                                data_array[index][1][offset + 2] <= cpu_wdata[23:16];
                                data_array[index][1][offset + 3] <= cpu_wdata[31:24];

                                dirty[index][1] <= 1'b1;
                                lru[index] <= 1'b0;
                            end
                        end

                        state <= IDLE; //PARADA
                    end
                    else begin
                        cpu_ready <= 1'b0;
                        cpu_hit   <= 1'b0;
                        state     <= MISS;
                    end
                end

                MISS: begin
                    // Primeiro tenta encontrar via inválida
                    if (!valid[index][0]) begin
                        victim_way <= 1'b0;
                    end
                    else if (!valid[index][1]) begin
                        victim_way <= 1'b1;
                    end
                    else begin
                        victim_way <= lru[index];
                    end

                    state <= REFILL;
                end

                REFILL: begin
                    // Refill simplificado, ainda sem L2/memória real
                    valid[index][victim_way] <= 1'b1;
                    tag_array[index][victim_way] <= tag;

                    // Para teste inicial, salva o próprio endereço como dado fake
                    data_array[index][victim_way][offset]     <= cpu_addr[7:0];
                    data_array[index][victim_way][offset + 1] <= cpu_addr[15:8];
                    data_array[index][victim_way][offset + 2] <= cpu_addr[23:16];
                    data_array[index][victim_way][offset + 3] <= cpu_addr[31:24];

                    if (cpu_we) begin
                        // Se a requisição original era escrita, escreve cpu_wdata
                        data_array[index][victim_way][offset]     <= cpu_wdata[7:0];
                        data_array[index][victim_way][offset + 1] <= cpu_wdata[15:8];
                        data_array[index][victim_way][offset + 2] <= cpu_wdata[23:16];
                        data_array[index][victim_way][offset + 3] <= cpu_wdata[31:24];

                        dirty[index][victim_way] <= 1'b1;
                    end
                    else begin
                        dirty[index][victim_way] <= 1'b0;
                        cpu_rdata <= cpu_addr; // dado fake para teste inicial
                    end

                    // Atualiza LRU
                    if (victim_way == 1'b0)
                        lru[index] <= 1'b1;
                    else
                        lru[index] <= 1'b0;

                    cpu_ready <= 1'b1;
                    cpu_hit   <= 1'b0;

                    state <= IDLE;
                end

            endcase
        end
    end

endmodule
