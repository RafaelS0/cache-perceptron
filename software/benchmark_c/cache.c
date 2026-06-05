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


// MÓDULO PERCEPTRON (AIRA)
#define PERCEPTRON_LINES 1024  // Linhas da tabela
#define NUM_WEIGHTS 9          // 1 Bias + bits do GHR
#define GHR_SIZE 8             // Tamanho do histórico (TESTAR 8, 16 E 32. NÃO ESQUEÇA DE MUDAR A VARIÁVEL DE CIMA E INVERTER OS COMENTÁRIOS DE HIT/MISS DO AIRA)
#define THRESHOLD 18           // Limiar de treinamento (TESTAR 18 [valor da tese], 36 E 72)

// calcula o score 'u' para um determinado PC
static int perceptron_predict(cache *c, unsigned int pc) {
    // Isola os 10 bits do PC para indexar a tabela
    //unsigned int index = (pc >> c->offset_bits) & (PERCEPTRON_LINES - 1);
    unsigned int index = (pc >> 2) & (PERCEPTRON_LINES - 1);
    
    // u inicia com o valor do Bias (w0 * x0, onde x0 é sempre 1)
    int u = c->weights_table[index][0]; 
    
    // Soma ponderada com os 8 bits do GHR
    for (int i = 1; i <= GHR_SIZE; i++) {
        // pega o bit específico do GHR. Se for 1, x_i = 1. Se for 0, x_i = -1.
        int x_i = (c->ghr & (1 << (i - 1))) ? 1 : -1;
        u += c->weights_table[index][i] * x_i;
    }
    return u;
}

/* treina (atualiza) os pesos quando necessário */
static void perceptron_train(cache *c, unsigned int pc, int d, int u) {
    // só treina se errou a predição ou se a confiança (u) for menor que o THRESHOLD
    if ((u >= 0) != (d == 1) || abs(u) <= THRESHOLD) {
        //unsigned int index = (pc >> c->offset_bits) & (PERCEPTRON_LINES - 1);
        unsigned int index = (pc >> 2) & (PERCEPTRON_LINES - 1);
        
        // Atualiza o Bias
        int w = c->weights_table[index][0] + d; // d é 1 (Hit) ou -1 (Miss)
        // saturação: Impede que o número de 8 bits dê inverta (overflow)
        if (w > 127) w = 127; else if (w < -128) w = -128;
        c->weights_table[index][0] = (int8_t)w;

        // Atualiza os pesos do GHR
        for (int i = 1; i <= GHR_SIZE; i++) {
            int x_i = (c->ghr & (1 << (i - 1))) ? 1 : -1;
            w = c->weights_table[index][i] + (d * x_i);
            // saturação
            if (w > 127) w = 127; else if (w < -128) w = -128;
            c->weights_table[index][i] = (int8_t)w;
        }
    }
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
    
    
    // inicializa as variáveis do Perceptron
    c->ghr = 0;
    c->weights_table = (int8_t **) malloc(PERCEPTRON_LINES * sizeof(int8_t *));
    for (int i = 0; i < PERCEPTRON_LINES; i++) {
        c->weights_table[i] = (int8_t *) calloc(NUM_WEIGHTS, sizeof(int8_t)); // calloc zera pesos
    }
    

    return c;
}

/* ---------------------------------------------------------------------
 * cache_access
 * Processa um acesso ao endereço fornecido e aplica a política de substituição.
 * Retorna 1 (hit) ou 0 (miss).
 * --------------------------------------------------------------------- */
int cache_access(cache *c, unsigned int data_address, unsigned int pc) {

    /* --- Passo 1: fatiar o endereço em offset, índice e tag --- */
    unsigned int offset = data_address & ((1 << c->offset_bits) - 1);                    // não utilizado ainda
    unsigned int index  = (data_address >> c->offset_bits) & ((1 << c->index_bits) - 1); // índice do conjunto
    unsigned int tag    = data_address >> (c->offset_bits + c->index_bits);               // tag do bloco

    cache_set *set = &c->sets[index]; // conjunto correspondente ao índice

    /* --- Passo 2: verificar hit --- */
    for (int i = 0; i < c->num_ways; i++) {
        if (set->ways[i].valid && set->ways[i].tag == tag) {

            // HIT LRU: Atualiza contadores
            if (c->replacement_policy == 0) {
                int current_lru = set->ways[i].lru_counter;// contador do bloco acessado
                for (int j = 0; j < c->num_ways; j++) {
                    // blocos mais recentes que o acessado ficam mais antigos
                    if (set->ways[j].valid && set->ways[j].lru_counter < current_lru)
                        set->ways[j].lru_counter++;
                }
                set->ways[i].lru_counter = 0;// bloco acessado = mais recente
            }
            // HIT AIRA (Perceptron): Recompensa o bloco e atualiza o GHR
            else if (c->replacement_policy == 1) {
                int u = perceptron_predict(c, set->ways[i].pc);
                perceptron_train(c, set->ways[i].pc, 1, u); // d = 1 (Acertou em manter)
                c->ghr = ((c->ghr << 1) | 1) & 0xFF;        // GHR recebe 1 (Hit)
                //c->ghr = (c->ghr << 1) | 1;
            }

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
        if (c->replacement_policy == 0) {
            int max_counter = -1;
            for (int i = 0; i < c->num_ways; i++) {
                if (set->ways[i].lru_counter > max_counter) {
                    max_counter = set->ways[i].lru_counter;
                    victim = i;
                }
            }
        }
        else if (c->replacement_policy == 1) {
            int min_u = 9999999; // inicializa com um valor alto para encontrar o mínimo
            victim = 0; // assume a primeira via como vítima inicial
            
            for (int i = 0; i < c->num_ways; i++) {
                int u = perceptron_predict(c, set->ways[i].pc);
                // expulsa o bloco de menor score
                if (u < min_u) {
                    min_u = u;
                    victim = i;
                }
            }
            // pune o bloco que foi expulso (causou o Miss ou não foi útil)
            perceptron_train(c, set->ways[victim].pc, -1, min_u); // d = -1 (Miss)
        }
        
    }

    /* --- Passo 4: inserir novo bloco na via escolhida e atualizar LRU --- */
    /* Incrementa contador de todos os blocos válidos antes de inserir */
    if (c->replacement_policy == 0) {
        for (int i = 0; i < c->num_ways; i++) {
            if (set->ways[i].valid) set->ways[i].lru_counter++;
        }
        set->ways[victim].lru_counter = 0;
    }
    else if (c->replacement_policy == 1) {//atualiza GHR
        c->ghr = (c->ghr << 1) & 0xFF; // GHR recebe 0 (Miss)
        //c->ghr = (c->ghr << 1);
    }
    
    
    set->ways[victim].valid = 1;
    set->ways[victim].tag = tag;
    set->ways[victim].pc = pc; // MUDOU: Agora salva o PC da instrução, não o endereço do dado


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


void cache_print_set(cache *c, int set_index) {
    if (c == NULL) return;

    if (set_index < 0 || set_index >= c->num_sets) {
        printf("Set invalido: %d\n", set_index);
        return;
    }

    cache_set *set = &c->sets[set_index];

    printf("\n--- Estado do Set %d ---\n", set_index);

    for (int i = 0; i < c->num_ways; i++) {
        printf(
            "Via %d | valid=%d | tag=%u | lru=%d | pc=0x%08X\n",
            i,
            set->ways[i].valid,
            set->ways[i].tag,
            set->ways[i].lru_counter,
            set->ways[i].pc
        );
    }

    printf("------------------------\n");
}


void cache_debug_perceptron_pc(cache *c, unsigned int pc) {
    if (c == NULL) return;

    unsigned int index = (pc >> 2) & (PERCEPTRON_LINES - 1);
    int u = perceptron_predict(c, pc);

    printf("\n--- Debug Perceptron para PC 0x%08X ---\n", pc);
    printf("Indice tabela: %u\n", index);
    printf("GHR: 0x%02X\n", c->ghr);
    printf("Score u: %d\n", u);

    printf("Pesos: ");
    for (int i = 0; i < NUM_WEIGHTS; i++) {
        printf("%d ", c->weights_table[index][i]);
    }
    printf("\n");
    printf("----------------------------------------\n");
}

void cache_debug_set_perceptron(cache *c, int set_index) {
    if (c == NULL) return;

    if (set_index < 0 || set_index >= c->num_sets) {
        printf("Set invalido: %d\n", set_index);
        return;
    }

    cache_set *set = &c->sets[set_index];

    printf("\n--- Debug Set %d / Perceptron ---\n", set_index);
    printf("GHR: 0x%02X\n", c->ghr);

    for (int i = 0; i < c->num_ways; i++) {
        if (set->ways[i].valid) {
            int u = perceptron_predict(c, set->ways[i].pc);

            printf(
                "Via %d | valid=%d | tag=%u | pc=0x%08X | score=%d\n",
                i,
                set->ways[i].valid,
                set->ways[i].tag,
                set->ways[i].pc,
                u
            );
        } else {
            printf("Via %d | invalid\n", i);
        }
    }

    printf("----------------------------------\n");
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
    
    if (c->weights_table != NULL) {
        for (int i = 0; i < PERCEPTRON_LINES; i++) {
            free(c->weights_table[i]);
        }
        free(c->weights_table);
    }
    
    free(c);
}
