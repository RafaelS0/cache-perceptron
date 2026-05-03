/*
 * cache.c
 * Implementação das funções da cache.
 */

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------
 * Função auxiliar: Calcula quantidade de bits necessários para representar cada campo do endereço
    * - offset_bits: log2(block_size)
   * - index_bits: log2(num_sets)
   * - tag_bits: restante dos bits do endereço (não calculado aqui, mas seria: 32 - offset_bits - index_bits) 
 
 * --------------------------------------------------------------------- */
static int log2_int(int n) {
    int bits = 0; // conta quantos bits são necessários para representar n
    while (n > 1) { // enquanto n for maior que 1, continua dividindo por 2
        n >>= 1; // equivalente a n = n / 2, 
        bits++; // incrementa o contador de bits
    }
    return bits;  // retorna o número de bits necessários para representar n (log2(n))
}

/* ---------------------------------------------------------------------
 * cache_init
 * Aloca e inicializa a cache com os parâmetros fornecidos.
 * Retorna ponteiro para a cache criada, ou NULL em caso de erro.
 * --------------------------------------------------------------------- */
cache* cache_init(int num_sets, int num_ways, int block_size, int replacement_policy) {

    /* 1. Aloca a struct principal */
    cache *c = (cache*) malloc(sizeof(cache)); // aloca a estrutura da cache na memória
    if (c == NULL) {  // verifica se a alocação foi bem-sucedida
        printf("Erro: falha ao alocar cache\n");
        return NULL;
    }

    /* 2. Preenche os parâmetros */
    c->num_sets          = num_sets;  // número de conjuntos (sets) na cache
    c->num_ways          = num_ways; // número de vias por conjunto (associatividade)
    c->block_size        = block_size; // tamanho do bloco em bytes
    c->offset_bits       = log2_int(block_size); // número de bits para o deslocamento dentro do bloco (log2(block_size))
    c->index_bits        = log2_int(num_sets); // número de bits para o índice do conjunto (log2(num_sets))
    c->hit_count         = 0; // contaor de hits inicializado em 0
    c->miss_count        = 0; // contador de misses inicializado em 0
    c->replacement_policy = replacement_policy;  // política de substituição (0 para LRU, 1 para Perceptron)

    /* 3. Aloca o array de conjuntos (sets) */
    c->sets = (cache_set*) malloc(num_sets * sizeof(cache_set)); // aloca um array de conjuntos (sets) na memória, onde cada conjunto é do tipo cache_set
    if (c->sets == NULL) { 
        printf("Erro: falha ao alocar sets\n");
        free(c);
        return NULL;
    }

    /* 4. Para cada conjunto, aloca o array de vias e inicializa os blocos */
    
    for (int i = 0; i < num_sets; i++) { // para cada conjunto (set) na cache
        c->sets[i].num_ways = num_ways; // define o número de vias para o conjunto atual
        c->sets[i].ways = (cache_block*) malloc(num_ways * sizeof(cache_block)); // aloca um array de vias (cache_block) para o conjunto atual, onde cada via pode conter um bloco de cache
       
        if (c->sets[i].ways == NULL) { 
            printf("Erro: falha ao alocar vias do conjunto %d\n", i);
            
            /* Libera o que já foi alocado antes de sair */
            for (int j = 0; j < i; j++) free(c->sets[j].ways); 
            free(c->sets); 
            free(c); 
            
            return NULL;
        }

        /* 5. Inicializa cada bloco do conjunto */
        for (int j = 0; j < num_ways; j++) { // para cada via no conjunto atual
            c->sets[i].ways[j].valid       = 0; // inicializa o bit de validade do bloco como 0 (inválido)
            c->sets[i].ways[j].tag         = 0; // inicializa a tag do bloco como 0 
            c->sets[i].ways[j].lru_counter = 0; // inicializa o contador LRU do bloco como 0 
        }
    }

    return c; // retorna o ponteiro para a cache inicializada
}

/* ---------------------------------------------------------------------
 * cache_access
 * Processa um acesso ao endereço fornecido e aplica a política de substituição.
 * Retorna 1 (hit) ou 0 (miss).
 * --------------------------------------------------------------------- */
int cache_access(cache *c, unsigned int address) { // recebe um ponteiro para a cache e um endereço de memória a ser acessado, e retorna 1 se for um hit ou 0 se for um miss

    /* --- Passo 1: fatiar o endereço em offset, índice e tag --- */
   
    unsigned int offset = address & ((1 << c->offset_bits) - 1); // nao utilizado ainda
    unsigned int index  = (address >> c->offset_bits) & ((1 << c->index_bits) - 1); // índice do conjunto (set)
    unsigned int tag    = address >> (c->offset_bits + c->index_bits); // tag do bloco

    cache_set *conjunto = &c->sets[index]; // obtém o ponteiro para o conjunto (set) correspondente ao índice calculado a partir do endereço

    /* --- Passo 2: verificar hit --- */
    for (int i = 0; i < c->num_ways; i++) { // para cada via no conjunto, verifica se há um bloco válido com a mesma tag do endereço acessado
        if (conjunto->ways[i].valid && conjunto->ways[i].tag == tag) { // se encontrar um bloco válido com a mesma tag, é um hit

            /* HIT: atualiza contadores LRU */
            int contador_atual = conjunto->ways[i].lru_counter; // armazena o valor atual do contador LRU do bloco que foi acessado (hit)
            for (int j = 0; j < c->num_ways; j++) { // para cada via no conjunto, incrementa o contador LRU dos blocos válidos que são mais antigos que o bloco acessado 

                /* blocos mais recentes que o acessado ficam ainda mais antigos */
                if (conjunto->ways[j].valid && conjunto->ways[j].lru_counter < contador_atual) { // se o bloco j for válido e tiver um contador LRU menor que o do bloco acessado, significa que ele é mais recente que o bloco acessado, então incrementa seu contador LRU para indicar que ele se tornou mais antigo em relação ao bloco acessado
                    conjunto->ways[j].lru_counter++;
                }
            }
            conjunto->ways[i].lru_counter = 0; /* bloco acessado = mais recente */

            c->hit_count++; // incrementa contagem de acertos
            return 1; // HIT
        }
    }

    /* --- Passo 3: MISS — Substituir bloco (LRU) --- */
    int need_space = -1; // Indica a via onde o novo bloco será inserido. Inicialmente, -1 significa que ainda não foi encontrada uma via para inserção.

    /* Primeiro tenta encontrar uma via vazia */
    for (int i = 0; i < c->num_ways; i++) { // percorre as vias do conjunto para encontrar uma via vazia (inválida) onde o novo bloco possa ser inserido sem precisar substituir um bloco existente
        if (!conjunto->ways[i].valid) { // se encontrar uma via onde o bloco não é válido (inválida), significa que essa via está vazia e pode ser usada para inserir o novo bloco sem precisar substituir um bloco existente
            need_space = i; // armazena o índice da via vazia encontrada para inserir o novo bloco
            break; 
        }
    }

    /* Se não houver via vazia, escolhe a de maior lru_counter (mais antiga) */
    if (need_space == -1) { // se não encontrou nenhuma via vazia, significa que todas as vias estão ocupadas por blocos válidos
        int maior_contador = -1; // variável para armazenar o maior valor do contador LRU encontrado
        
        /*Percorre todas as vias para encontrar a mais antiga (maior lru_counter) */
        for (int i = 0; i < c->num_ways; i++) { 
            if (conjunto->ways[i].lru_counter > maior_contador) { 
                maior_contador = conjunto->ways[i].lru_counter; // atualiza o maior contador LRU encontrado até agora com o contador do bloco na via i, que é o mais antigo encontrado até agora
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

    conjunto->ways[need_space].valid       = 1; // marca a via escolhida como válida, indicando que agora contém um bloco válido
    conjunto->ways[need_space].tag         = tag; // atualiza a tag do bloco na via escolhida com a tag do endereço acessado, para que possa ser comparada em futuros acessos e determinar se é um hit ou miss
    conjunto->ways[need_space].lru_counter = 0; /* recém inserido → mais recente */

    c->miss_count++;
    return 0; // retorna 0 para indicar que é miss
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