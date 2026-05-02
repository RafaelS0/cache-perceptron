/*
 * cache.c
 * Implementação das funções da cache.
 */

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------
 * Função auxiliar: calcula log2 de um número inteiro
 * Ex: log2_int(32) = 5, log2_int(64) = 6
 * --------------------------------------------------------------------- */
static int log2_int(int n) {
    int bits = 0;
    while (n > 1) {
        n >>= 1;
        bits++;
    }
    return bits;
}

/* ---------------------------------------------------------------------
 * cache_init
 * Aloca e inicializa a cache com os parâmetros fornecidos.
 * Retorna ponteiro para a cache criada, ou NULL em caso de erro.
 * --------------------------------------------------------------------- */
cache* cache_init(int num_sets, int num_ways, int block_size, int replacement_policy) {

    /* 1. Aloca a struct principal */
    cache *c = (cache*) malloc(sizeof(cache));
    if (c == NULL) {
        printf("Erro: falha ao alocar cache\n");
        return NULL;
    }

    /* 2. Preenche os parâmetros */
    c->num_sets          = num_sets;
    c->num_ways          = num_ways;
    c->block_size        = block_size;
    c->offset_bits       = log2_int(block_size);
    c->index_bits        = log2_int(num_sets);
    c->hit_count         = 0;
    c->miss_count        = 0;
    c->replacement_policy = replacement_policy;

    /* 3. Aloca o array de conjuntos (sets) */
    c->sets = (cache_set*) malloc(num_sets * sizeof(cache_set));
    if (c->sets == NULL) {
        printf("Erro: falha ao alocar sets\n");
        free(c);
        return NULL;
    }

    /* 4. Para cada conjunto, aloca o array de vias e inicializa os blocos */
    for (int i = 0; i < num_sets; i++) {
        c->sets[i].num_ways = num_ways;
        c->sets[i].ways = (cache_block*) malloc(num_ways * sizeof(cache_block));
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
            c->sets[i].ways[j].valid       = 0;
            c->sets[i].ways[j].tag         = 0;
            c->sets[i].ways[j].lru_counter = 0;
        }
    }

    return c;
}

/* ---------------------------------------------------------------------
 * cache_access
 * Processa um acesso ao endereço fornecido.
 * Retorna 1 (hit) ou 0 (miss).
 * --------------------------------------------------------------------- */
int cache_access(cache *c, unsigned int address) {

    /* --- Passo 1: fatiar o endereço em offset, índice e tag --- */
    unsigned int offset = address & ((1 << c->offset_bits) - 1); // nao utilizado ainda
    unsigned int index  = (address >> c->offset_bits) & ((1 << c->index_bits) - 1); // índice do conjunto (set)
    unsigned int tag    = address >> (c->offset_bits + c->index_bits); // tag do bloco

    cache_set *conjunto = &c->sets[index];

    /* --- Passo 2: verificar hit --- */
    for (int i = 0; i < c->num_ways; i++) {
        if (conjunto->ways[i].valid && conjunto->ways[i].tag == tag) {

            /* HIT: atualiza contadores LRU */
            int contador_atual = conjunto->ways[i].lru_counter;
            for (int j = 0; j < c->num_ways; j++) {

                /* blocos mais recentes que o acessado ficam ainda mais antigos */
                if (conjunto->ways[j].valid && conjunto->ways[j].lru_counter < contador_atual) {
                    conjunto->ways[j].lru_counter++;
                }
            }
            conjunto->ways[i].lru_counter = 0; /* bloco acessado = mais recente */

            c->hit_count++; // incrementa contagem de acertos
            return 1; // HIT
        }
    }

    /* --- Passo 3: MISS — Substituir bloco (LRU) --- */
    int need_space = -1;

    /* Primeiro tenta encontrar uma via vazia */
    for (int i = 0; i < c->num_ways; i++) {
        if (!conjunto->ways[i].valid) {
            need_space = i;
            break;
        }
    }

    /* Se não houver via vazia, escolhe a de maior lru_counter (mais antiga) */
    if (need_space == -1) {
        int maior_contador = -1;
        for (int i = 0; i < c->num_ways; i++) {
            if (conjunto->ways[i].lru_counter > maior_contador) {
                maior_contador = conjunto->ways[i].lru_counter;
                need_space = i;
            }
        }
    }

    /* --- Passo 4: inserir novo bloco na via escolhida --- e atualizar LRU --- */
    /* Incrementa contador de todos os blocos válidos antes de inserir */
    for (int i = 0; i < c->num_ways; i++) {
        if (conjunto->ways[i].valid) {
            conjunto->ways[i].lru_counter++;
        }
    }

    conjunto->ways[need_space].valid       = 1;
    conjunto->ways[need_space].tag         = tag;
    conjunto->ways[need_space].lru_counter = 0; /* recém inserido → mais recente */

    c->miss_count++;
    return 0;
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

    for (int i = 0; i < c->num_sets; i++) {
        free(c->sets[i].ways);
    }
    free(c->sets);
    free(c);
}