/*
 * main.c
 * Simulador de cache com política LRU e Perceptron.
 * Gerador de sequências sintéticas mistas para validação do hit rate.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cache.h"
#include "config.h"

/* ---------------------------------------------------------------------
 * Parâmetros do gerador de sequências
 * --------------------------------------------------------------------- */
#define NUM_ACESSOS     1000   /* numero total de acessos            */
#define TAMANHO_LOOP    8      /* quantos endereços distintos no loop     */
#define TAMANHO_SCAN    64     /* quantos endereços distintos no scan     */

/* ---------------------------------------------------------------------
 * Gerador de sequências sintéticas
 * Combina 3 padrões em sequência:
 *   1. Loop    — reutiliza os mesmos endereços (favorece hit)
 *   2. Scan    — acessa endereços sequenciais sem repetir (causa miss)
 *   3. Aleatório — endereços aleatórios dentro de uma janela
 *   
 *
 * Parâmetro: array de endereços a preencher, tamanho do array
 * --------------------------------------------------------------------- */
void gerar_sequencia_mista(unsigned int *sequencia, int num_acessos) {
    int i = 0;
    int quarto = num_acessos / 4;

    /* Padrão 1: Loop — repete endereços 0 a TAMANHO_LOOP */
    for (; i < quarto; i++) {
        sequencia[i] = (i % TAMANHO_LOOP) * L1_TAMANHO_BLOCO;
    }

    /* Padrão 2: Scan — acessa sequencialmente sem repetir */
    for (; i < 2 * quarto; i++) {
        sequencia[i] = (i % TAMANHO_SCAN) * L1_TAMANHO_BLOCO;
    }

    /* Padrão 3: Aleatório — endereços aleatórios numa janela de 32 blocos */
    srand(42); /* semente fixa para reprodutibilidade */
    for (; i < 3 * quarto; i++) {
        sequencia[i] = (rand() % 32) * L1_TAMANHO_BLOCO;
    }

}

/* ---------------------------------------------------------------------
 * Função auxiliar: roda a sequência na cache e imprime hit rate
 * --------------------------------------------------------------------- */
void rodar_simulacao(cache *c, unsigned int *sequencia, int tamanho, const char *nome) {
    printf("\n--- %s ---\n", nome);
    for (int i = 0; i < tamanho; i++) {
        cache_access(c, sequencia[i]);
    }
    cache_print_stats(c);
}

/* ---------------------------------------------------------------------
 * main
 * --------------------------------------------------------------------- */
int main() {

    printf("===========================================\n");
    printf("  Simulador de Cache — Baseline LRU\n");
    printf("  L1: %d conjuntos | %d vias | bloco %d bytes\n",
           L1_NUM_CONJUNTOS, L1_ASSOCIATIVIDADE, L1_TAMANHO_BLOCO);
    printf("===========================================\n");

    /* 1. Inicializa a cache L1 com política LRU (0) */
    cache *c = cache_init(L1_NUM_CONJUNTOS, L1_ASSOCIATIVIDADE, L1_TAMANHO_BLOCO, 0);
    if (c == NULL) {
        fprintf(stderr, "Erro ao inicializar a cache.\n");
        return 1;
    }

    /* 2. Gera sequência mista */
    unsigned int *sequencia = (unsigned int*) malloc(NUM_ACESSOS * sizeof(unsigned int));
    if (sequencia == NULL) {
        fprintf(stderr, "Erro ao alocar sequência.\n");
        cache_free(c);
        return 1;
    }
    gerar_sequencia_mista(sequencia, NUM_ACESSOS);

    /* 3. Roda simulação e imprime resultados */
    rodar_simulacao(c, sequencia, NUM_ACESSOS, "Sequência Mista (Loop + Scan + Aleatório + Loop)");

    /* 4. Imprime detalhes por padrão */
    int quarto = NUM_ACESSOS / 4;
    printf("\n--- Detalhamento por padrão ---\n");

    /* Reinicia a cache pra cada padrão */
    const char *nomes[] = {"Loop", "Scan", "Aleatório"};
    
    for (int p = 0; p < 3; p++) {
        cache_free(c);
        c = cache_init(L1_NUM_CONJUNTOS, L1_ASSOCIATIVIDADE, L1_TAMANHO_BLOCO, 0);
        printf("\nPadrão %d: %s\n", p + 1, nomes[p]);
        for (int i = p * quarto; i < (p + 1) * quarto; i++) {
            cache_access(c, sequencia[i]);
        }
       for (int i = 0; i < NUM_ACESSOS; i++) {
    printf("[%d] = %u\n", i, sequencia[i]);
}
     
     
        cache_print_stats(c);
    }

    /* 5. Libera memória */
    free(sequencia);
    cache_free(c);

    return 0;
}