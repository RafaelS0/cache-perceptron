/*
 * main.c
 * Simulador de cache com política LRU e Perceptron.
 */
#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "config.h"

#define NUM_ACCESSES 800
#define LOOP_SIZE    130
#define SCAN_SIZE    64

void generate_sequence(unsigned int *sequence, int num_accesses, int type_acess)
{
    switch (type_acess) {
        case 1: // Loop — repete os mesmos endereços
            for (int i = 0; i < num_accesses; i++)
                sequence[i] = (i % LOOP_SIZE) * L1_BLOCK_SIZE;
            break;
        case 2: // Scan — acessa endereços sequenciais sem repetir
            for (int i = 0; i < num_accesses; i++)
                sequence[i] = (i % SCAN_SIZE) * L1_BLOCK_SIZE;
            break;
        case 3: // Aleatório — endereços aleatórios numa janela de 32 blocos
            srand(42); // semente fixa para reprodutibilidade
            for (int i = 0; i < num_accesses; i++)
                sequence[i] = (rand() % 32) * L1_BLOCK_SIZE;
            break;
    }
}

int main()
{
    printf("===========================================\n");
    printf("  Simulador de Cache — Baseline LRU\n");
    printf("  L1: %d conjuntos | %d vias | bloco %d bytes\n",
           L1_NUM_SETS, L1_WAYS, L1_BLOCK_SIZE);
    printf("===========================================\n");

    int pattern;
    printf("Escolha o padrão de acesso:\n");
    printf("  1) Loop\n  2) Scan\n  3) Aleatório\n");
    printf("Digite 1, 2 ou 3: ");
    scanf("%d", &pattern);
    if (pattern < 1 || pattern > 3) {
        printf("Entrada inválida. Usando padrão aleatório.\n");
        pattern = 3;
    }

    // inicializa a cache L1 com política LRU (0)
    cache *c = cache_init(L1_NUM_SETS, L1_WAYS, L1_BLOCK_SIZE, 0);
    if (c == NULL) {
        fprintf(stderr, "Erro ao inicializar a cache.\n");
        return 1;
    }

    unsigned int *sequence = (unsigned int *) malloc(NUM_ACCESSES * sizeof(unsigned int));
    if (sequence == NULL) {
        fprintf(stderr, "Erro ao alocar sequência.\n");
        cache_free(c);
        return 1;
    }

    generate_sequence(sequence, NUM_ACCESSES, pattern);

    for (int i = 0; i < NUM_ACCESSES; i++)
        cache_access(c, sequence[i]);

    cache_print_stats(c);

    free(sequence);
    cache_free(c);
    return 0;
}
