/*
 * cache.c
 * Implementação das funções da cache com suporte a LRU e Perceptron com Histerese Corrigida.
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

// =====================================================================
// MÓDULO PERCEPTRON (AIRA)
// =====================================================================
#define PERCEPTRON_LINES 1024  // Linhas da tabela de pesos
#define NUM_WEIGHTS      9     // 1 Bias + 8 bits do GHR
#define GHR_SIZE         8     // Tamanho do histórico global
#define THRESHOLD        18    // Limiar de treinamento

/* Calcula o score 'u' para um determinado PC */
static int perceptron_predict(cache *c, unsigned int pc) {
    // Isola os bits significativos do PC para indexar a tabela
    unsigned int index = (pc >> 2) & (PERCEPTRON_LINES - 1);
    
    // u inicia com o valor do Bias (w0)
    int u = c->weights_table[index][0];
    
    // Soma ponderada com os bits do GHR
    for (int i = 1; i <= GHR_SIZE; i++) {
        int x_i = (c->ghr & (1 << (i - 1))) ? 1 : -1;
        u += c->weights_table[index][i] * x_i;
    }
    return u;
}

/* Treina (atualiza) os pesos quando necessário */
static void perceptron_train(cache *c, unsigned int pc, int d, int u) {
    // Só treina se errou a predição ou se a confiança (u) for menor/igual ao THRESHOLD
    if (((u >= 0) != (d == 1)) || abs(u) <= THRESHOLD) {
        unsigned int index = (pc >> 2) & (PERCEPTRON_LINES - 1);
        
        // Atualiza o Bias com saturação para evitar overflow de 8 bits
        int w = c->weights_table[index][0] + d;
        if (w > 127) w = 127; else if (w < -128) w = -128;
        c->weights_table[index][0] = (int8_t)w;

        // Atualiza os pesos associados ao histórico (GHR)
        for (int i = 1; i <= GHR_SIZE; i++) {
            int x_i = (c->ghr & (1 << (i - 1))) ? 1 : -1;
            w = c->weights_table[index][i] + (d * x_i);
            if (w > 127) w = 127; else if (w < -128) w = -128;
            c->weights_table[index][i] = (int8_t)w;
        }
    }
}

// =====================================================================
// INTERFACE DE CONTROLO DA CACHE
// =====================================================================

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
    c->offset_bits        = log2_int(block_size);
    c->index_bits         = log2_int(num_sets);
    c->hit_count          = 0;
    c->miss_count         = 0;
    c->replacement_policy = replacement_policy;

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
            for (int j = 0; j < i; j++) free(c->sets[j].ways);
            free(c->sets);
            free(c);
            return NULL;
        }

        /* 5. Inicializa cada bloco do conjunto */
        for (int j = 0; j < num_ways; j++) {
            c->sets[i].ways[j].valid        = 0;
            c->sets[i].ways[j].tag          = 0;
            c->sets[i].ways[j].lru_counter  = 0;
            c->sets[i].ways[j].pc           = 0;
            c->sets[i].ways[j].hysteresis   = 0; // Inicializa contador de histerese zerado
        }
    }
    
    // Inicializa as variáveis da Inteligência Artificial
    c->ghr = 0;
    c->weights_table = (int8_t **) malloc(PERCEPTRON_LINES * sizeof(int8_t *));
    for (int i = 0; i < PERCEPTRON_LINES; i++) {
        c->weights_table[i] = (int8_t *) calloc(NUM_WEIGHTS, sizeof(int8_t)); // calloc zera os pesos iniciais
    }

    return c;
}

int cache_access(cache *c, unsigned int data_address, unsigned int pc) {
    /* --- Passo 1: fatiar o endereço em offset, índice e tag --- */
    unsigned int index  = (data_address >> c->offset_bits) & ((1 << c->index_bits) - 1); // índice do conjunto
    unsigned int tag    = data_address >> (c->offset_bits + c->index_bits);               // tag do bloco

    cache_set *set = &c->sets[index]; // conjunto correspondente ao índice

    /* --- Passo 2: verificar HIT --- */
    for (int i = 0; i < c->num_ways; i++) {
        if (set->ways[i].valid && set->ways[i].tag == tag) {

            // HIT LRU: Atualiza os contadores de idade
            if (c->replacement_policy == 0) {
                int current_lru = set->ways[i].lru_counter;
                for (int j = 0; j < c->num_ways; j++) {
                    if (set->ways[j].valid && set->ways[j].lru_counter < current_lru)
                        set->ways[j].lru_counter++;
                }
                set->ways[i].lru_counter = 0;
            }
            // HIT AIRA (Perceptron): Recompensa o bloco, recarrega histerese e atualiza GHR
            else if (c->replacement_policy == 1) {
                int u = perceptron_predict(c, set->ways[i].pc);
                perceptron_train(c, set->ways[i].pc, 1, u); // d = 1 (Acertou em manter)
                
                // Desloca histórico global e insere 1 (Hit) mascarado pelo tamanho
                c->ghr = ((c->ghr << 1) | 1) & ((1 << GHR_SIZE) - 1);
                
                // Atribui confiança máxima (3) por ter dado Hit recente
                set->ways[i].hysteresis = 3;
                set->ways[i].pc = pc; // Atualiza o PC dinamicamente
            }

            c->hit_count++;
            return 1; // HIT
        }
    }

    /* --- Passo 3: MISS — encontrar a vítima --- */
    int victim = -1;

    /* Primeiro tenta encontrar uma via vazia (Válido para ambas as políticas) */
    for (int i = 0; i < c->num_ways; i++) {
        if (!set->ways[i].valid) {
            victim = i;
            break;
        }
    }

    /* Se não houver via vazia, aplica o algoritmo de substituição */
    if (victim == -1) {
        if (c->replacement_policy == 0) { // LRU
            int max_counter = -1;
            for (int i = 0; i < c->num_ways; i++) {
                if (set->ways[i].lru_counter > max_counter) {
                    max_counter = set->ways[i].lru_counter;
                    victim = i;
                }
            }
        }
        else if (c->replacement_policy == 1) { // AIRA (Perceptron com Histerese Independente)
            int min_u = 9999999;
            victim = -1;
            
            // Passo A: Procura a via de menor score predito que já tenha expirado a histerese (igual a 0)
            for (int i = 0; i < c->num_ways; i++) {
                int u = perceptron_predict(c, set->ways[i].pc);
                if (u < min_u && set->ways[i].hysteresis == 0) {
                    min_u = u;
                    victim = i;
                }
            }
            
            // Passo B: Se todas as linhas estiverem protegidas (> 0), quebra o escudo
            // selecionando o menor score absoluto do conjunto para desempate.
            if (victim == -1) {
                min_u = 9999999;
                for (int i = 0; i < c->num_ways; i++) {
                    int u = perceptron_predict(c, set->ways[i].pc);
                    if (u < min_u) {
                        min_u = u;
                        victim = i;
                    }
                }
            }
            
            // Passo C: Se a linha escolhida ainda tinha escudo ativo, ela gasta 1 ponto de proteção
            if (set->ways[victim].hysteresis > 0) {
                set->ways[victim].hysteresis--;
            }
            
            // Treina negativamente os pesos do Perceptron para a linha que está saindo da cache
            perceptron_train(c, set->ways[victim].pc, -1, min_u);
        }
    }

    /* --- Passo 4: inserir novo bloco na via escolhida --- */
    if (c->replacement_policy == 0) {
        for (int i = 0; i < c->num_ways; i++) {
            if (set->ways[i].valid) set->ways[i].lru_counter++;
        }
        set->ways[victim].lru_counter = 0;
    }
    else if (c->replacement_policy == 1) { // Atualiza o GHR no Miss
        // Desloca inserindo bit 0 (Miss) limitado à máscara de tamanho real do GHR
        c->ghr = (c->ghr << 1) & ((1 << GHR_SIZE) - 1);
    }
    
    // Grava as informações estáveis do novo bloco alocado
    set->ways[victim].valid      = 1;
    set->ways[victim].tag        = tag;
    set->ways[victim].pc         = pc; // Guarda o PC da instrução geradora do Miss
    set->ways[victim].hysteresis = 2;  // Começa com proteção em nível médio (2 de 3) de fábrica

    c->miss_count++;
    return 0; // MISS
}

void cache_print_stats(cache *c) {
    int total = c->hit_count + c->miss_count;
    float hit_rate = (total > 0) ? (100.0f * c->hit_count / total) : 0.0f;
    printf("Hits:     %d\n", c->hit_count);
    printf("Misses:   %d\n", c->miss_count);
    printf("Hit Rate: %.2f%%\n", hit_rate);
}

void cache_free(cache *c) {
    if (c == NULL) return;
    for (int i = 0; i < c->num_sets; i++)
        free(c->sets[i].ways);
    free(c->sets);
    
    if (c->weights_table != NULL) {
        for (int i = 0; i < PERCEPTRON_LINES; i++) {
            free(c->weights_table[i]);
        }
        free(c->weights_table);
    }
    free(c);
}