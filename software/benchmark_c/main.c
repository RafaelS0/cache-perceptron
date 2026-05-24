/*
 * main.c
 * Simulador de cache com política LRU e Perceptron.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "config.h"

// configs para banchmarks
#define L2_SIZE_BYTES (128 * 1024)
#define ARRAY_SIZE    (L2_SIZE_BYTES * 2 / 4)

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

    //int col_width = 14;

    printf("\n");
    printf("-----------------------------------------------------------------------------------------\n");
    printf("                                  %s\n", benchmark_name);
    printf("                         (%s)\n", config_label);
    printf("-----------------------------------------------------------------------------------------\n");

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
    printf("\n-----------------------------------------------------------------------------------------\n");
}


// Benchmark 1
void simulate_streaming(cache *c) {
    unsigned int base_array = 0x10000; 
    unsigned int base_hot   = 0x20000; 
    
    // PCs fakes
    unsigned int pc_array = 0x00400000; // Instrução que lê o array
    unsigned int pc_hot   = 0x00400004; // Instrução que lê o dado importante
    
    for (int it = 0; it < 10; it++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            // Acessa o dado passando o PC daquela instrução
            cache_access(c, base_array + (i * 4), pc_array);
            
            if (i % 64 == 0) {
                // Acessa o hot_data passando o PC dele
                cache_access(c, base_hot, pc_hot);
            }
        }
    }
}

// Benchmark 2
void simulate_matrix_conv(cache *c) {
    unsigned int base_img = 0x30000;
    unsigned int base_out = 0x80000;
    int width = 128;
    int height = ARRAY_SIZE / width;
    
    unsigned int pc_img = 0x00400008; // Instrução que lê a imagem
    unsigned int pc_out = 0x0040000C; // Instrução que escreve a saída
    
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            cache_access(c, base_img + (((y-1)*width+x) * 4), pc_img);
            cache_access(c, base_img + ((y*width+x) * 4), pc_img);
            cache_access(c, base_img + (((y+1)*width+x) * 4), pc_img);
            cache_access(c, base_out + ((y*width+x) * 4), pc_out);
        }
    }
}

void print_menu() {
    printf("1. Streaming + HotSet (Antagonista ao LRU)\n");
    printf("2. Matrix Convolution (Reuso Temporal)\n");
    printf("0. Sair\n");
    printf("Escolha uma opção: ");
}


static Metrics run_policy(int policy, int benchmark_id, int cap, int ways, int b_size) {
    int num_sets = cap / (b_size * ways);
    cache *c = cache_init(num_sets, ways, b_size, policy);
    //for (int i = 0; i < n; i++) cache_access(c, sequence[i]);
    
    if (benchmark_id == 1) simulate_streaming(c);
    else if (benchmark_id == 2) simulate_matrix_conv(c);

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

int main(){
    int choice = 1;
    
    while (choice != 0) {
        print_menu();
        if (scanf("%d", &choice) != 1) break;
        
        if (choice == 1 || choice == 2) {
            
            // 1. Roda o Baseline
            Metrics lru = run_policy(0, choice, L1_CAPACITY, L1_WAYS, L1_BLOCK_SIZE);
            PolicyResult results[2];
            results[0] = (PolicyResult){ "Baseline (LRU)", lru };
            int count = 1;
            
            // 2. Roda o Perceptron (AIRA1)
            Metrics aira1 = run_policy(1, choice, L1_CAPACITY, L1_WAYS, L1_BLOCK_SIZE);
            results[1] = (PolicyResult){ "AIRA1", aira1 };
            count = 2;
            
            /*Metrics aira2 = run_policy(2, choice, L1_CAPACITY, L1_WAYS, L1_BLOCK_SIZE);
            results[2] = (PolicyResult){ "AIRA2", aira2 };
            count = 3;*/
        
            char nome_bench[50];
            sprintf(nome_bench, "BENCHMARK %d", choice);
            print_benchmark_table_flexible(nome_bench, "L1 cache simulada", results, count);
        }
    }
    
    printf("Encerrando...\n");
    return 0;
}
