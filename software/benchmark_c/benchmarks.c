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