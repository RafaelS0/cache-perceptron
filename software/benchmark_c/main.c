/*
 * main.c
 * Simulador de cache com política LRU e Perceptron.
 */
#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "config.h"
#include "benchmarks.h"



// --- VARIÁVEIS GLOBAIS DE CONFIGURAÇÃO DINÂMICA ---
// Inicializam com os valores padrão definidos no config.h (por padrão não usa L2)
int cfg_use_l2   = USE_L2; 
int cfg_l1_sets  = L1_NUM_SETS;
int cfg_l1_ways  = L1_WAYS;
int cfg_l1_block = L1_BLOCK_SIZE;

int cfg_l2_sets  = L2_NUM_SETS;
int cfg_l2_ways  = L2_WAYS;
int cfg_l2_block = L2_BLOCK_SIZE;

// Função interativa para alterar os parâmetros em tempo de execução
void cache_config() {
    int sub_option = 0;
    
    printf("\n===================================================\n");
    printf("        MENU DE CONFIGURAÇÃO DA CACHE\n");
    printf("===================================================\n");
    printf("1. Alternar Cache L2 (Atual: %s)\n", cfg_use_l2 ? "LIGADA" : "DESLIGADA");
    printf("2. Alterar Vias da L1 (Atual: %d vias)\n", cfg_l1_ways);
    printf("3. Alterar Vias da L2 (Atual: %d vias)\n", cfg_l2_ways);
    printf("6. Resetar para Padrões do config.h\n");
    printf("0. Voltar ao Menu Principal\n");
    printf("---------------------------------------------------\n");
    printf("Digite sua opção: ");
    scanf("%d", &sub_option);

    if (sub_option == 1) {
        cfg_use_l2 = !cfg_use_l2; // Inverte entre 0 e 1
        printf("\n[Sucesso] Cache L2 alterada para: %s\n", cfg_use_l2 ? "LIGADA" : "DESLIGADA");
    } 
    else if (sub_option == 2) {
        printf("Digite o novo número de vias para L1: ");
        scanf("%d", &cfg_l1_ways);
		if(cfg_l1_ways != 4 && cfg_l1_ways != 8){
		cfg_l1_ways = L1_WAYS; // Restaura para o valor padrão
		printf("\n[Erro] Número de vias para L1 inválido. Deve ser 4 ou 8. Revertendo para o valor padrão (%d vias).\n", cfg_l1_ways);
		} else {
        printf("\n[Sucesso] L1 configurada para %d vias.\n", cfg_l1_ways);
 	   } 
}
    else if (sub_option == 3) {
        printf("Digite o novo número de vias para L2: ");
        scanf("%d", &cfg_l2_ways);
		if(cfg_l2_ways != 8 && cfg_l2_ways != 16){
		cfg_l2_ways = L2_WAYS; // Restaura para o valor padrão
		printf("\n[Erro] Número de vias para L2 inválido. Deve ser 8 ou 16. Revertendo para o valor padrão (%d vias).\n", cfg_l2_ways);
		
		} else {
		printf("\n[Sucesso] L2 configurada para %d vias.\n", cfg_l2_ways);
		}   
    }
    else if (sub_option == 5) {
        cfg_use_l2   = USE_L2;
        cfg_l1_ways  = L1_WAYS;
        cfg_l2_ways  = L2_WAYS;
        printf("\n[Sucesso] Configurações resetadas para o padrão de fábrica.\n");
    }
    printf("===================================================\n\n");
}



float run_simulation(int scenario, int policy, int ghr, int threshold, int cap, int ways)
{
    // 1. Inicializa a cache L1 usando as variáveis globais configuráveis
    int num_sets = cap / (L1_BLOCK_SIZE * ways);
    cache *L1 = cache_init(num_sets, ways, L1_BLOCK_SIZE, policy, ghr, threshold);
	
    cache *L2 = NULL;

    // Se o usuário ativou a L2 na configuração, instancia e encadeia ela
    if (cfg_use_l2) {
        L2 = cache_init(cfg_l2_sets, cfg_l2_ways, cfg_l2_block, policy, ghr, threshold);
        L1->l2 = L2; 
    } else {
        L1->l2 = NULL; // Garante que está desativada
    }

    // 2. Dispara o cenário de teste selecionado
    if (scenario == 1) simulate_streaming(L1);
	else if (scenario == 2) simulate_matrix_conv(L1);
    else if (scenario == 3) simulate_zigzag_access(L1);
    else if (scenario == 4) simulate_hash_table_noise(L1);
    else if (scenario == 5) simulate_cyclic_thrashing(L1);
    else if (scenario == 6) simulate_conditional_pattern(L1);

    // 3. Calcula matematicamente a taxa de acerto (Hit Rate) final da L1
    int total_accesses = L1->hit_count + L1->miss_count;
    float hit_rate = (total_accesses > 0) ? ((float)L1->hit_count / total_accesses) * 100.0f : 0.0f;

    // 4. Se a L2 estiver ligada, exibe suas métricas de forma segura
    if (cfg_use_l2 && L2 != NULL) {
        int total_l2 = L2->hit_count + L2->miss_count;
        float hit_rate_l2 = (total_l2 > 0) ? ((float)L2->hit_count / total_l2) * 100.0f : 0.0f;
        printf("     [Info Interna] -> Hit Rate L1: %.2f%% | Hit Rate L2: %.2f%%\n", hit_rate, hit_rate_l2);
        cache_free(L2); // Libera a L2
    } else {
        printf("     [Info Interna] -> Hit Rate L1: %.2f%% | L2 DESATIVADA\n", hit_rate);
    }

    // 5. Libera a memória da L1
    cache_free(L1);

    return hit_rate;
}

int main(){

    int option = 0;
    int default_ghr = 8;
    int default_t = 29;

    do{
        printf("=== SIMULADOR DE CACHE ===\n");
        printf("Escolha o cenario de teste:\n");
        printf("1. Streaming + HotSet (Antagonista ao LRU)\n");
        printf("2. Matrix Convolution (Reuso Temporal)\n");
        printf("3. ZigZag Access (Acesso Correlacionado)\n");
        printf("4. Hash Table Noise (Ruidos)\n");
        printf("5. Cyclic Thrashing\n");
		printf("6. Conditional Pattern\n");
		printf("7. Validacao funcional LRU\n");
		printf("8. Validacao funcional Perceptron\n");
        printf("10. Rodar Grid Tests Automatizado (Gera CSV)\n");
        printf("11. Configurar Parametros da Cache (L2 / Vias)\n");
        printf("0. Sair\n");
        printf("Digite sua opcao: ");
        scanf("%d", &option);

        // CORREÇÃO DE BUG: As duas chamadas soltas de run_simulation que ficavam aqui 
        // foram removidas, pois elas executavam indevidamente para qualquer opção digitada.

        if (option >= 1 && option <= 6)
        {
            const char *nomes[] = {"", "Streaming", "Matrix", "ZigZag", "Hash Noise", "Cyclic", "Conditional"};
            
            float hit_lru = run_simulation(option, 0, default_ghr, default_t, L1_CAPACITY, L1_WAYS);  
            float hit_aira = run_simulation(option, 1, default_ghr, default_t, L1_CAPACITY, L1_WAYS);
      
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

        if (option == 7) {
            // Adaptado para também usar os parâmetros configurados dinamicamente
            cache *L1 = cache_init(cfg_l1_sets, cfg_l1_ways, cfg_l1_block, 0, default_ghr, default_t);
            simulate_validation_lru(L1);
            cache_free(L1);
            continue;
        }

        if (option == 8) {
            // Adaptado para também usar os parâmetros configurados dinamicamente
            cache *L1 = cache_init(cfg_l1_sets, cfg_l1_ways, cfg_l1_block, 1, default_ghr, default_t);
            simulate_validation_perceptron(L1);
            cache_free(L1);
            continue;
        }
        
        if (option == 10) {
            int ghr_testes[] = {4, 8, 12, 16, 24, 32, 48, 64, 80, 96, 112, 128};
            int t_testes[]   = {21, 29, 37, 44, 60, 75, 106, 137, 168, 199, 230, 261}; 
            int num_testes = sizeof(ghr_testes) / sizeof(ghr_testes[0]);

			int cap_testes[] = {2048, 4096, 16384}; 
	        int way_testes[] = {1, 2, 4};
	        int num_arqs = 3;

            printf("\nL1_Cap(KB),L1_Ways,GHR,Threshold,B1_LRU,B1_AIRA,B2_LRU,B2_AIRA,B3_LRU,B3_AIRA,B5_LRU,B5_AIRA,B6_LRU,B6_AIRA\n");

            // varia a Arquitetura da Cache
	        for (int a = 0; a < num_arqs; a++) {
	            int cap = cap_testes[a];
	            int ways = way_testes[a];
	            
	            //varia a IA (GHR e Threshold)
	            for (int i = 0; i < num_testes; i++) {
	                int ghr = ghr_testes[i];
	                int t = t_testes[i];
	    
	                float b1_lru = run_simulation(1, 0, ghr, t, cap, ways);
	                float b1_aira= run_simulation(1, 1, ghr, t, cap, ways);
	                float b2_lru = run_simulation(2, 0, ghr, t, cap, ways);
	                float b2_aira= run_simulation(2, 1, ghr, t, cap, ways);
	                float b3_lru = run_simulation(3, 0, ghr, t, cap, ways);
	                float b3_aira= run_simulation(3, 1, ghr, t, cap, ways);
	                float b4_lru = run_simulation(4, 0, ghr, t, cap, ways);
	                float b4_aira= run_simulation(4, 1, ghr, t, cap, ways);
	                float b5_lru = run_simulation(5, 0, ghr, t, cap, ways);
	                float b5_aira= run_simulation(5, 1, ghr, t, cap, ways);
	                float b6_lru = run_simulation(6, 0, ghr, t, cap, ways);
	                float b6_aira= run_simulation(6, 1, ghr, t, cap, ways);
	    
	                printf("%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",cap/1024, ways, ghr, t,
	                        b1_lru, b1_aira, b2_lru, b2_aira, b3_lru, b3_aira, b5_lru, b5_aira, b6_lru, b6_aira);
	            }
	        }
        }

        if (option == 11) {
            cache_config(); // Chama o menu de configuração dinâmica
        }
        
    } while (option != 0);

    return 0;
}
