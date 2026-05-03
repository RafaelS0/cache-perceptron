/*Estrutura de dados da cache e funções de acesso */

#ifndef CACHE_H
#define CACHE_H

// Parâmetros da cache definidos em config.h
#include "config.h"
#include <stdint.h>
#include <stdlib.h>

// Estrutura de um BLOCO de cache
typedef struct
{
    int valid;        // bit de validade
    unsigned int tag; // tag do bloco (parte mais significativa do endereço)
    int lru_counter;  // necessário pro LRU
} cache_block;        // bloco de cache

// Estrutura de um conjunto de cache
typedef struct
{
    cache_block *ways; // array de vias (cada via pode conter um bloco)
    int num_ways;      // número de vias por conjunto (set)
} cache_set;           // conjunto (set)

// Estrutura da cache
typedef struct
{
    cache_set *sets;    // array de conjuntos
    int num_sets;           // número de conjuntos
    int num_ways;           // número de vias por conjunto
    int block_size;         // tamanho do bloco em bytes
    int offset_bits;        // número de bits para o deslocamento dentro do bloco (log2(block_size))
    int index_bits;         // número de bits para o índice do conjunto (log2(num_sets))
    int hit_count;          // contagem de acertos
    int miss_count;         // contagem de falhas
    int replacement_policy; // 0: LRU, 1: PERCEPTRON
} cache;

/* Funções de acesso à cache */

/*Inicializa a cache com os parâmetros especificados */
cache *cache_init(int num_sets, int num_ways, int block_size, int replacement_policy);

/*Libera a memória da cache*/
void cache_free(cache *c);

/*Acessa a cache com um endereço de memória e retorna 1 para acerto e 0 para falha

 */
int cache_access(cache *c, unsigned int address);

#endif // CACHE_H
