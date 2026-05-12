/*
 * main.c
 * Simulador de cache com política LRU e Perceptron.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "config.h"


typedef struct {
    double hit_rate;
    double area_les;
    double fmax_mhz;
    int decision_latency_cycles;
} Metrics;

typedef struct {
    const char *label;
    Metrics metrics;
} PolicyResult;

static void print_benchmark_table_flexible(const char *benchmark_name, const char *config_label, PolicyResult *results, int count) {
    if (count < 1) return;

    int col_width = 14;

    printf("\n");

    // cabeçalho
    printf("%-22s| %-14s", "Métrica", results[0].label);
    for (int i = 1; i < count; i++) {
        printf(" | %-12s | %-10s", results[i].label, "Impacto");
    }
    printf("\n-----------------------------------------------------------------------------------------\n");

    // linha: Taxa de Acerto (%). Impacto da taxa de acerto é calculado como a diferença
    double base_hit = results[0].metrics.hit_rate;
    printf("%-22s| %6.2f%%       ", "Taxa de Acerto (%)", base_hit);
    for (int i = 1; i < count; i++) {
        double hit = results[i].metrics.hit_rate;
        double impact = hit - base_hit;
        printf(" | %6.2f%%     | %s%.2f%%   ", hit, (impact >= 0 ? "+" : ""), impact);
    }
    printf("\n\n");

    // Linha: Área (LEs). Impacto da área é relativo
    double base_area = results[0].metrics.area_les;
    printf("%-22s| %6.1f un.    ", "Área (LEs)", base_area);
    for (int i = 1; i < count; i++) {
        double area = results[i].metrics.area_les;
        double impact = (base_area > 0) ? ((area - base_area) / base_area) * 100.0 : 0.0;
        printf(" | %6.1f un.  | %s%.2f%%   ", area, (impact >= 0 ? "+" : ""), impact);
    }
    printf("\n\n");

    // Linha: Frequência (Fmax). Impacto da frequência é relativo
    double base_fmax = results[0].metrics.fmax_mhz;
    printf("%-22s| %6.1f MHz    ", "Frequência (Fmax)", base_fmax);
    for (int i = 1; i < count; i++) {
        double fmax = results[i].metrics.fmax_mhz;
        double impact = (base_fmax > 0) ? ((fmax - base_fmax) / base_fmax) * 100.0 : 0.0;
        printf(" | %6.1f MHz  | %s%.2f%%   ", fmax, (impact >= 0 ? "+" : ""), impact);
    }
    printf("\n\n");

    // Linha: Latência de Decisão. Impacto da latência é em ciclos absolutos
    int base_lat = results[0].metrics.decision_latency_cycles;
    printf("%-22s| %d ciclo%s      ", "Latência de Decisão", base_lat, (base_lat == 1 ? " " : "s"));
    for (int i = 1; i < count; i++) {
        int lat = results[i].metrics.decision_latency_cycles;
        int impact = lat - base_lat;
        printf(" | %d ciclo%s   | %s%d ciclo%s", 
                lat, (lat == 1 ? " " : "s"), 
                (impact >= 0 ? "+" : ""), impact, (impact == 1 || impact == -1 ? " " : "s"));
    }
    printf("\n=========================================================================================\n");
}

static Metrics run_policy(int policy, unsigned int *sequence, int n) {
    cache *c = cache_init(L1_NUM_SETS, L1_WAYS, L1_BLOCK_SIZE, policy);
    for (int i = 0; i < n; i++) cache_access(c, sequence[i]);

    int total = c->hit_count + c->miss_count;
    Metrics m;
    m.hit_rate = (total > 0) ? (100.0 * c->hit_count / total) : 0.0;
    
    // valores fixos temporariamente para síntese de hardware (Quartus)
    if (policy == 0) { // LRU
        m.area_les = 0; 
        m.fmax_mhz = 0;
        m.decision_latency_cycles = 0;
    } else if (policy == 1) { // AIRA 1
        m.area_les = 0; 
        m.fmax_mhz = 0;
        m.decision_latency_cycles = 0;
    } else { // AIRA 2
        m.area_les = 0;
        m.fmax_mhz = 0;
        m.decision_latency_cycles = 0;
    }

    cache_free(c);
    return m;
}


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

    unsigned int *sequence = (unsigned int *) malloc(NUM_ACCESSES * sizeof(unsigned int));
    if (sequence == NULL) {
        fprintf(stderr, "Erro ao alocar sequência.\n");
        return 1;
    }

    generate_sequence(sequence, NUM_ACCESSES, pattern);



    // 1. Roda sempre o Baseline (LRU)
    Metrics lru_metrics = run_policy(0, sequence, NUM_ACCESSES);
    
    // 2. Controla quantas políticas queremos comparar na tabela
    int num_policies_to_compare = 1; 
    PolicyResult results[3]; // Suporta até 3 colunas: LRU, AIRA1, AIRA2
    
    // Insere o Baseline na primeira posição
    results[0] = (PolicyResult){ "Baseline (LRU)", lru_metrics };
    
    /*
    Metrics aira1_metrics = run_policy(1, sequence, NUM_ACCESSES);
    results[1] = (PolicyResult){ "AIRA1", aira1_metrics };
    num_policies_to_compare = 2; 
    */
    
    /*
    Metrics aira2_metrics = run_policy(2, sequence, NUM_ACCESSES);
    results[2] = (bPolicyResult){ "AIRA2", aira2_metrics };
    num_policies_to_compare = 3;
    */

    // imprime a tabela de acordo com a quantidade de políticas ativadas
    print_benchmark_table_flexible("BENCHMARK A", "L1 4KB e 2 vias, L2 32KB e 8 vias", results, num_policies_to_compare);



    free(sequence);
    return 0;
}
