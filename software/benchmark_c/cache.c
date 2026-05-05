/*
 * cache.c
 * Implementação das funções da cache.
 */

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------
 * Função auxiliar: Calcula quantidade de bits necessários para representar n
 * Equivalente a log2(n)
 * --------------------------------------------------------------------- */
static int log2_int(int n) {
    int bits = 0;
    while (n > 1) {
        n >>= 1; // divide por 2
        bits++;  // incrementa o contador de bits
    }
    return bits;
}

/* ---------------------------------------------------------------------
 * cache_init
 * Aloca e inicializa a cache com os parâmetros fornecidos.
 * Retorna ponteiro para a cache criada, ou NULL em caso de erro.
 * --------------------------------------------------------------------- */
cache *cache_init(int num_sets, int num_ways, int block_size, int replacement_policy) {

    /* 1. Aloca a struct principal */
    cache *c = (cache *) malloc(sizeof(cache));
    if (c == NULL) {
        printf("Erro: falha ao alocar cache\n");
        return NULL;
    }

    /* 2. Preenche os parâmetros */
    c->num_sets           = num_sets;
    c->num_ways           = num_ways;
    c->block_size         = block_size;
    c->offset_bits        = log2_int(block_size); // log2(block_size)
    c->index_bits         = log2_int(num_sets);   // log2(num_sets)
    c->hit_count          = 0;
    c->miss_count         = 0;
    c->replacement_policy = replacement_policy;   // 0: LRU, 1: Perceptron

    /* 3. Aloca o array de conjuntos */
    c->sets = (cache_set *) malloc(num_sets * sizeof(cache_set));
    if (c->sets == NULL) {
        printf("Erro: falha ao alocar conjuntos\n");
        free(c);
        return NULL;
    }

    /* 4. Para cada conjunto, aloca o array de vias e inicializa os blocos */
    for (int i = 0; i < num_sets; i++) {
        c->sets[i].num_ways = num_ways;
        c->sets[i].ways = (cache_block *) malloc(num_ways * sizeof(cache_block));

        if (c->sets[i].ways == NULL) {
            printf("Erro: falha ao alocar vias do conjunto %d\n", i);
            /* Libera o que já foi alocado antes de sair */
            for (int j = 0; j < i; j++) free(c->sets[j].ways);
            free(c->sets);
            free(c);
            return NULL;
        }

        /* 5. Inicializa cada bloco do conjunto */
        for (int j = 0; j < num_ways; j++) {
            c->sets[i].ways[j].valid       = 0; // inválido
            c->sets[i].ways[j].tag         = 0;
            c->sets[i].ways[j].lru_counter = 0;
        }
    }

    return c;
}

/* ---------------------------------------------------------------------
 * cache_access
 * Processa um acesso ao endereço fornecido e aplica a política de substituição.
 * Retorna 1 (hit) ou 0 (miss).
 * --------------------------------------------------------------------- */
int cache_access(cache *c, unsigned int address) {

    /* --- Passo 1: fatiar o endereço em offset, índice e tag --- */
    unsigned int offset = address & ((1 << c->offset_bits) - 1);                    // não utilizado ainda
    unsigned int index  = (address >> c->offset_bits) & ((1 << c->index_bits) - 1); // índice do conjunto
    unsigned int tag    = address >> (c->offset_bits + c->index_bits);               // tag do bloco

    cache_set *set = &c->sets[index]; // conjunto correspondente ao índice

    /* --- Passo 2: verificar hit --- */
    for (int i = 0; i < c->num_ways; i++) {
        if (set->ways[i].valid && set->ways[i].tag == tag) {

            /* HIT: atualiza contadores LRU */
            int current_lru = set->ways[i].lru_counter; // contador do bloco acessado
            for (int j = 0; j < c->num_ways; j++) {
                // blocos mais recentes que o acessado ficam mais antigos
                if (set->ways[j].valid && set->ways[j].lru_counter < current_lru)
                    set->ways[j].lru_counter++;
            }
            set->ways[i].lru_counter = 0; // bloco acessado = mais recente

            c->hit_count++;
            return 1; // HIT
        }
    }

    /* --- Passo 3: MISS — encontrar a vítima --- */
    int victim = -1;

    /* Primeiro tenta encontrar uma via vazia */
    for (int i = 0; i < c->num_ways; i++) {
        if (!set->ways[i].valid) {
            victim = i;
            break;
        }
    }

    /* Se não houver via vazia, escolhe a de maior lru_counter (mais antiga) */
    if (victim == -1) {
        int max_counter = -1;
        for (int i = 0; i < c->num_ways; i++) {
            if (set->ways[i].lru_counter > max_counter) {
                max_counter = set->ways[i].lru_counter;
                victim = i;
            }
        }
    }

    /* --- Passo 4: inserir novo bloco na via escolhida e atualizar LRU --- */
    /* Incrementa contador de todos os blocos válidos antes de inserir */
    for (int i = 0; i < c->num_ways; i++) {
        if (set->ways[i].valid)
            set->ways[i].lru_counter++;
    }

    set->ways[victim].valid       = 1;
    set->ways[victim].tag         = tag;
    set->ways[victim].lru_counter = 0; // recém inserido = mais recente

    c->miss_count++;
    return 0; // MISS
}

/* ---------------------------------------------------------------------
 * cache_print_stats
 * Imprime hits, misses e hit rate.
 * --------------------------------------------------------------------- */
void cache_print_stats(cache *c) {
    int total = c->hit_count + c->miss_count;
    float hit_rate = (total > 0) ? (100.0f * c->hit_count / total) : 0.0f;
    printf("Hits:     %d\n", c->hit_count);
    printf("Misses:   %d\n", c->miss_count);
    printf("Hit Rate: %.2f%%\n", hit_rate);
}

/* ---------------------------------------------------------------------
 * cache_free
 * Libera toda a memória alocada pela cache.
 * --------------------------------------------------------------------- */
void cache_free(cache *c) {
    if (c == NULL) return;
    for (int i = 0; i < c->num_sets; i++)
        free(c->sets[i].ways);
    free(c->sets);
    free(c);
}
