#include "cache.h"
#include "config.h"

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