/*
 * main.c
 * Simulador de cache com política LRU e Perceptron.
 */
#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "config.h"
#include "benchmarks.h"

float run_simulation(int scenario, int policy)
{
  // 1. Inicializa a cache L1 com a política escolhida
  cache *L1 = cache_init(L1_NUM_SETS, L1_WAYS, L1_BLOCK_SIZE, policy);

  // 2. Dispara o cenário de teste selecionado
  if (scenario == 1)
  {
    simulate_streaming(L1);
  }
  else if (scenario == 2)
  {
    simulate_matrix_conv(L1);
  }

  // 3. Calcula matematicamente a taxa de acerto (Hit Rate) final
  int total_accesses = L1->hit_count + L1->miss_count;
  float hit_rate = (total_accesses > 0) ? ((float)L1->hit_count / total_accesses) * 100.0f : 0.0f;

  // 4. Libera a memória da cache utilizada no benchamark (evita vazamento de memória)
  cache_free(L1);

  return hit_rate;
}

int main()
{

  int option = 0;

  do
  {

    printf("=== SIMULADOR DE CACHE  ===\n");
    printf("Escolha o cenario de teste:\n");
    printf("1. Streaming + HotSet (Antagonista ao LRU)\n");
    printf("2. Matrix Convolution(Reuso Temporal)\n");
    printf("0. Sair\n");
    printf("Digite sua opcao: ");
    scanf("%d", &option);

    const char *scenario_name = (option == 1) ? "Streaming + HotSet " : "Matrix Convolution";

    // Executa as duas simulações e guarda apenas as taxas de acerto
    float hit_lru = run_simulation(option, 0);  // 0 = LRU Baseline
    float hit_aira = run_simulation(option, 1); // 1 = AIRA Perceptro

    if (option == 1 || option == 2)
    {
      printf(" BENCHMARK: %s\n", scenario_name);
      printf("\n===================================================\n");
      printf("===================================================\n");
      printf(" Algoritmo de Substituicao | Taxa de Acerto (Hit Rate) \n");
      printf("---------------------------|-----------------------\n");
      printf(" LRU (Baseline)            |        %6.2f%% \n", hit_lru);
      printf(" AIRA (Perceptron)         |        %6.2f%% \n", hit_aira);
      printf("---------------------------|-----------------------\n");
      printf(" IMPACTO DO AIRA           |        %+6.2f%% \n", hit_aira - hit_lru);
      printf("===================================================\n\n");
    }

  } while (option != 0);

  return 0;
}
