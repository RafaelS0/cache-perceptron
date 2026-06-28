
module l1_cache #(
parameter ADDR_WIDTH = 32, // largura de endereço, em bits
parameter DATA_WIDTH = 32, //largura da palavra de dados, em bits
parameter CACHE_SIZE = 4096, //tamanho total da cache em bytes
parameter BLOCK_SIZE = 32, //tamanho de cada bloco, em bytes
parameter NUM_WAYS   = 2, //numero de vias
parameter NUM_SETS   = 64, //numero de conjuntos
parameter TAG_WIDTH  = 21, // tamanho da tag, em bits
parameter INDEX_WIDTH = 6, // tamanho do index, em bits
parameter OFFSET_WIDTH = 5 // tamanho do offset, em bits

)(

  input wire clk,
  input wire reset,

  input wire cpu_req, // processador pedindo acesso
  input wire cpu_we,  // 0 = leitura, 1 = escrita
  input wire [ADDR_WIDTH-1:0] cpu_addr, //endereço acessado pela CPU
  input wire [DATA_WIDTH-1:0] cpu_wdata, //dado para escrever
  
  output reg cpu_ready, // se cache terminou a requisiçao
  output reg cpu_hit, // 0 = miss, 1= hit
  output reg [DATA_WIDTH-1:0] cpu_rdata, // valor lido pela cpu

);


  //Campos do endereço
  wire [OFFSET_WIDTH-1:0] offset;
  wire [INDEX_WIDTH-1:0] index;
  wire [TAG_WIDTH-1:0] tag;


  assign offset = cpu_addr[OFFSET_WIDTH-1:0];
  assign index  = cpu_addr[OFFSET_WIDTH + INDEX_WIDTH - 1 : OFFSET_WIDTH];
  assign tag    = cpu_addr[ADDR_WIDTH-1 : OFFSET_WIDTH + INDEX_WIDTH];

  //Metadados da cache

  reg valid [0:NUM_SETS-1][0:NUM_WAYS-1]; //valid[set][way]
  reg dirty [0:NUM_SETS-1][0:NUM_WAYS-1]; //dirt[set][way]
  reg [TAG_WIDTH-1:0] tag_array[0:NUM_SETS-1][0:NUM_WAYS-1]; //tag_arry[set][way]

  //Dados da cache
  reg [7:0] data_array [0:NUM_SETS-1][0:NUM_WAYS-1][0:BLOCK_SIZE-1];   //data_array[set][way][bytes do bloco]


  //LRU para cache 2-vias
  reg lru [0:NUM_SETS-1];



