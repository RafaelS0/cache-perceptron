#include "cache.h"
#include "config.h"
#include <stdio.h>
#define ARRAY_SIZE ((L1_CAPACITY * 2) / 4) // Tamanho do array para o benchmark 1 (2x a capacidade da cache, considerando blocos de 4 bytes)

// Benchmark 1
void simulate_streaming(cache *c)
{
    unsigned int base_array = 0x10000;
    unsigned int base_hot = 0x20000;

    // PCs fakes
    unsigned int pc_array = 0x00400000; // Instrução que lê o array
    unsigned int pc_hot = 0x00400004;   // Instrução que lê o dado importante

    for (int it = 0; it < 10; it++)
    {
        for (int i = 0; i < ARRAY_SIZE; i++)
        {
            // Acessa o dado passando o PC daquela instrução
            cache_access(c, base_array + (i * 4), pc_array);

            if (i % 64 == 0)
            {
                // Acessa o hot_data passando o PC dele
                cache_access(c, base_hot, pc_hot);
            }
        }
    }
}

// Benchmark 2
void simulate_matrix_conv(cache *c)
{
    unsigned int base_img = 0x30000;
    unsigned int base_out = 0x80000;
    int width = 128;
    int height = ARRAY_SIZE / width;

    unsigned int pc_img = 0x00400008; // Instrução que lê a imagem
    unsigned int pc_out = 0x0040000C; // Instrução que escreve a saída

    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            cache_access(c, base_img + (((y - 1) * width + x) * 4), pc_img);
            cache_access(c, base_img + ((y * width + x) * 4), pc_img);
            cache_access(c, base_img + (((y + 1) * width + x) * 4), pc_img);
            cache_access(c, base_out + ((y * width + x) * 4), pc_out);
        }
    }
}

// Benchmark 3 (Acesso Zigue-Zague Largo)
// processador lendo um bd gigante, alternando entre uma tabela principal (A) e uma tabela de índices (B)
void simulate_zigzag_access(cache *c) {
    unsigned int base_A = 0x100000; 
    unsigned int base_B = 0x200000; 
    
    // dois PCs separados, pq são duas partes diferentes do código
    unsigned int pc_read_A = 0x00400010; 
    unsigned int pc_read_B = 0x00400014; 
    
    // 20 repetições para forçar o histórico a aprender
    for (int rep = 0; rep < 20; rep++) {
        // Varre 8.000 blocos
        for (int i = 0; i < 8000; i++) {
            if (i % 2 == 0) {
                cache_access(c, base_A + (i * 4), pc_read_A);
            } else {
                cache_access(c, base_B + (i * 4), pc_read_B);
            }
        }
    }
}

// Benchmark 4 (Apenas Ruído)
//simula busca em Tabela Hash em uma área de 1mb de RAM.
void simulate_hash_table_noise(cache *c) {
    unsigned int base_hash = 0x500000;
    unsigned int pc_hash   = 0x00400020;
    
    srand(12345); 
    
    for (int i = 0; i < 100000; i++) {//100000 acessos
        //gera offset de até 256.000 posições inteiras (1MB)
        int random_offset = (rand() % 256000) * 4; 
        cache_access(c, base_hash + random_offset, pc_hash);
    }
}

// Benchmark 5(Thrashing Cíclico)
//varrer um array grande de forma contínua
void simulate_cyclic_thrashing(cache *c) {
    unsigned int base_addr = 0x300000;
    unsigned int pc_thrash = 0x00400030;
    
    int blocks_to_read = 1000; 
    
    for (int rep = 0; rep < 100; rep++) {
        for (int i = 0; i < blocks_to_read; i++) {
            cache_access(c, base_addr + (i * 32), pc_thrash);
        }
    }
}

// Benchmark 6(Padrão Condicional Longo)
//simula acessos guiados por lógicas de if/else rítmicas
void simulate_conditional_pattern(cache *c) {
    unsigned int base_A = 0x400000;
    unsigned int base_B = 0x401000;
    unsigned int base_C = 0x402000;
    unsigned int pc_cond = 0x00400034;
    
    for (int i = 0; i < 50000; i++) {
        int state = i % 4; 
        
        //Acesso aos mesmos 3 blocos em padrão rítmico
        if (state == 0)      cache_access(c, base_A, pc_cond);
        else if (state == 1) cache_access(c, base_B, pc_cond);
        else if (state == 2) cache_access(c, base_A, pc_cond);
        else                 cache_access(c, base_C, pc_cond);
    }
}

void simulate_validation_lru(cache *c) {
    unsigned int important_pc = 0x100;
    unsigned int pc_2 = 0x200;

    printf("\n========================================\n");
    printf(" VALIDACAO FUNCIONAL DO LRU\n");
    printf("========================================\n");

    printf("\nConfiguracao esperada: cache 2-way.\n");
    printf("A ideia e forcar tres blocos diferentes no mesmo set.\n");

    printf("\n[Acesso 1] Endereco: 0 | PC: 0x100\n");
    cache_access(c, 0, important_pc);
    cache_print_set(c, 0);

    printf("\n[Acesso 2] Endereco: 2048 | PC: 0x200\n");
    cache_access(c, 2048, pc_2);
    cache_print_set(c, 0);

    printf("\n[Acesso 3] Endereco: 0 | PC: 0x100\n");
    cache_access(c, 0, important_pc);
    cache_print_set(c, 0);

    printf("\n[Acesso 4] Endereco: 4096 | PC: 0x200\n");
    cache_access(c, 4096, pc_2);
    cache_print_set(c, 0);

    printf("\nResultado final:\n");
    cache_print_stats(c);

    printf("\nEsperado:\n");
    printf("- 3 misses\n");
    printf("- 1 hit\n");
    printf("- O bloco do endereco 0 deve permanecer na cache\n");
    printf("- O bloco do endereco 2048 deve ser substituido\n");
}

void simulate_validation_perceptron(cache *c) {
    unsigned int stride = c->num_sets * c->block_size;

    unsigned int pc_hot = 0x00400004;
    unsigned int pc_stream = 0x00400000;

    unsigned int addr_hot = 0;
    unsigned int addr_stream1 = stride;
    unsigned int addr_stream2 = stride * 2;

    printf("\n========================================\n");
    printf(" VALIDACAO FUNCIONAL DO PERCEPTRON\n");
    printf("========================================\n");

    printf("\nObjetivo:\n");
    printf("- Ensinar que pc_hot deve ser preservado\n");
    printf("- Ensinar que pc_stream deve ser menos confiavel\n");
    printf("- Verificar se a vitima escolhida e a de menor score\n");

    printf("\nEnderecos usados:\n");
    printf("addr_hot     = %u\n", addr_hot);
    printf("addr_stream1 = %u\n", addr_stream1);
    printf("addr_stream2 = %u\n", addr_stream2);
    printf("Todos devem mapear para o set 0.\n");

    /*
     * Fase 1: aquecimento.
     * Coloca dois blocos no mesmo set.
     */
    printf("\n[Fase 1] Preenchendo o set\n");

    cache_access(c, addr_hot, pc_hot);
    cache_access(c, addr_stream1, pc_stream);

    cache_debug_set_perceptron(c, 0);
    cache_debug_perceptron_pc(c, pc_hot);
    cache_debug_perceptron_pc(c, pc_stream);

    /*
     * Fase 2: reforçar o hot block.
     * Repetir acessos ao addr_hot gera HITs e treina d = 1.
     */
    printf("\n[Fase 2] Reforcando o pc_hot com varios HITs\n");

    for (int i = 0; i < 10; i++) {
        cache_access(c, addr_hot, pc_hot);
        cache_debug_perceptron_pc(c, pc_hot);
    }

    /*
     * Fase 3: gerar conflito.
     * Como addr_stream2 mapeia para o mesmo set, a cache precisa escolher uma vitima.
     * O esperado e preservar addr_hot e expulsar o bloco com pc_stream.
     */
    printf("\n[Fase 3] Gerando conflito no mesmo set\n");

    printf("\nAntes do conflito:\n");
    cache_debug_set_perceptron(c, 0);

    cache_access(c, addr_stream2, pc_stream);

    printf("\nDepois do conflito:\n");
    cache_debug_set_perceptron(c, 0);

    /*
     * Fase 4: verificar se o hot continua na cache.
     * Se der HIT, o Perceptron preservou o bloco correto.
     */
    printf("\n[Fase 4] Verificando se o hot block foi preservado\n");

    int result = cache_access(c, addr_hot, pc_hot);

    if (result == 1) {
        printf("\nPASSOU: addr_hot ainda estava na cache.\n");
    } else {
        printf("\nFALHOU: addr_hot foi expulso da cache.\n");
    }

    printf("\nEstatisticas finais:\n");
    cache_print_stats(c);
}
