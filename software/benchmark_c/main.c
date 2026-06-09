/*
 * main.c
 * Simulador de cache com política LRU e Perceptron.
 */
#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "config.h"
#include "benchmarks.h"

float run_simulation(int scenario, int policy, int ghr, int threshold)
{
	// 1. Inicializa a cache L1 com a política escolhida
	cache *L1 = cache_init(L1_NUM_SETS, L1_WAYS, L1_BLOCK_SIZE, policy, ghr, threshold);

	// 2. Dispara o cenário de teste selecionado
	if (scenario == 1) simulate_streaming(L1);
	else if (scenario == 2) simulate_matrix_conv(L1);
    else if (scenario == 3) simulate_zigzag_access(L1);
    else if (scenario == 4) simulate_hash_table_noise(L1);

	// 3. Calcula matematicamente a taxa de acerto (Hit Rate) final
	int total_accesses = L1->hit_count + L1->miss_count;
	float hit_rate = (total_accesses > 0) ? ((float)L1->hit_count / total_accesses) * 100.0f : 0.0f;

	// 4. Libera a memória da cache utilizada no benchamark (evita vazamento de memória)
	cache_free(L1);

	return hit_rate;
}

int main(){

	int option = 0;
	//valores padrão se for rodar interativo
	int default_ghr = 8;
    int default_t = 29;

	do{
		printf("=== SIMULADOR DE CACHE  ===\n");
		printf("Escolha o cenario de teste:\n");
		printf("1. Streaming + HotSet (Antagonista ao LRU)\n");
		printf("2. Matrix Convolution(Reuso Temporal)\n");
		printf("3. ZigZag Access (Acesso Correlacionado)\n");
        printf("4. Hash Table Noise (Ruidos)\n");
        
		printf("5. Validacao funcional LRU\n");
		printf("6. Validacao funcional Perceptron\n");
		
		printf("10. Rodar Grid Tests Automatizado (Gera CSV)\n");
		printf("0. Sair\n");
		printf("Digite sua opcao: ");
		scanf("%d", &option);

		const char *scenario_name = (option == 1) ? "Streaming + HotSet " : "Matrix Convolution";

		// Executa as duas simulações e guarda apenas as taxas de acerto
		float hit_lru = run_simulation(option, 0, default_ghr, default_t);  // 0 = LRU Baseline
		float hit_aira = run_simulation(option, 1, default_ghr, default_t); // 1 = AIRA Perceptro

		if (option >= 1 && option <= 4)
		{
		    const char *nomes[] = {"", "Streaming", "Matrix", "ZigZag", "Hash Noise"};
		    
		    float hit_lru = run_simulation(option, 0, default_ghr, default_t);  
            float hit_aira = run_simulation(option, 1, default_ghr, default_t);
      
			printf(" BENCHMARK: %s\n", nomes[option]);
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

		if (option == 5) {
			cache *L1 = cache_init(L1_NUM_SETS, L1_WAYS, L1_BLOCK_SIZE, 0, default_ghr, default_t);

			simulate_validation_lru(L1);

			cache_free(L1);
			continue;
		}

		if (option == 6) {
			cache *L1 = cache_init(L1_NUM_SETS, L1_WAYS, L1_BLOCK_SIZE, 1, default_ghr, default_t);

			simulate_validation_perceptron(L1);

			cache_free(L1);
			continue;
		}
		
		
		
		if (option == 10) {
        int ghr_testes[] = {4, 8, 12, 16, 24, 32, 48, 64, 80, 96, 112, 128};
        int t_testes[]   = {21, 29, 37, 44, 60, 75, 106, 137, 168, 199, 230, 261}; //t = floor(1.93 * GHR + 14)
        int num_testes = sizeof(ghr_testes) / sizeof(ghr_testes[0]);

        printf("\nGHR,Threshold,B1_LRU,B1_AIRA,B2_LRU,B2_AIRA,B3_LRU,B3_AIRA,B4_LRU,B4_AIRA\n");

        for (int i = 0; i < num_testes; i++) {
            int ghr = ghr_testes[i];
            int t = t_testes[i];

            float b1_lru = run_simulation(1, 0, ghr, t);
            float b1_aira= run_simulation(1, 1, ghr, t);
            float b2_lru = run_simulation(2, 0, ghr, t);
            float b2_aira= run_simulation(2, 1, ghr, t);
            float b3_lru = run_simulation(3, 0, ghr, t);
            float b3_aira= run_simulation(3, 1, ghr, t);
            float b4_lru = run_simulation(4, 0, ghr, t);
            float b4_aira= run_simulation(4, 1, ghr, t);

            printf("%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",ghr, t, b1_lru, b1_aira, b2_lru, b2_aira, b3_lru, b3_aira, b4_lru, b4_aira);
        }
    }
		

	} while (option != 0);

	return 0;
}
