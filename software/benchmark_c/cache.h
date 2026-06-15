/* Estrutura de dados da cache e funções de acesso */

#ifndef CACHE_H
#define CACHE_H

#include "config.h"
#include <stdint.h>
#include <stdlib.h>

// Estrutura de um BLOCO de cache
typedef struct {
    int valid;           // bit de validade
    unsigned int tag;    // tag do bloco (parte mais significativa do endereço)
    int lru_counter;     // contador LRU
    unsigned int pc;
} cache_block;

// Estrutura de um conjunto de cache
typedef struct {
    cache_block *ways;   // array de vias (cada via pode conter um bloco)
    int num_ways;        // número de vias por conjunto (set)
} cache_set;

// Estrutura da cache
typedef struct cache {
    cache_set *sets;            // array de conjuntos
    int num_sets;               // número de conjuntos
    int num_ways;               // número de vias por conjunto
    int block_size;             // tamanho do bloco em bytes
    int offset_bits;            // número de bits para o deslocamento dentro do bloco (log2(block_size))
    int index_bits;             // número de bits para o índice do conjunto (log2(num_sets))
    int hit_count;              // contagem de acertos
    int miss_count;             // contagem de falhas
    int replacement_policy;     // 0: LRU, 1: PERCEPTRON
    
    // perceptron
    int hysteresis;
    uint8_t ghr;                // Global History Register (8 bits ou X bits)
    int8_t **weights_table;
    int ghr_size;               
    int threshold;
    
    // cache L2
    struct cache *l2;          // ponteiro para cache L2 

} cache;

/* Inicializa a cache com os parâmetros especificados */
cache *cache_init(int num_sets, int num_ways, int block_size, int replacement_policy, int ghr_size, int threshold, int hysteresis);

/* Libera a memória da cache */
void cache_free(cache *c);

/* Acessa a cache com um endereço de memória e retorna 1 para acerto e 0 para falha */
int cache_access(cache *c, unsigned int data_address, unsigned int pc);

/* Imprime hits, misses e hit rate */
void cache_print_stats(cache *c);

void cache_print_set(cache *c, int set_index);

void cache_debug_perceptron_pc(cache *c, unsigned int pc);

void cache_debug_set_perceptron(cache *c, int set_index);


#endif // CACHE_/* Estrutura de dados da cache e funções de acesso */
