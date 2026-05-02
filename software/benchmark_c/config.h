/*Parâmetros da cache*/

#define CONFIG_H

/* ===================================================================== 
    Utilizando a lógica de acesso do RISC-V - Endereço de 32 bits
   O endereço é dividido em 3 campos: tag, index e offset
 * O campo offset é usado para identificar o byte dentro do bloco
 * O campo index é usado para identificar o conjunto na cache
 * O campo tag é são os bits restantes para comparação de identidade
 ===================================================================== */
 



/* =====================================================================
 * CACHE L1 (Dados)
 * Capacidade: 4KB a 8KB
 * Tamanho do bloco: 32 bytes
 * Associatividade: 2-way ou 4-way
 *
 * Número de conjuntos = Capacidade / (Bloco x Associatividade)
 * Exemplo: 4096 / (32 x 2) = 64 conjuntos
 * ===================================================================== */

#define L1_CAPACIDADE       4096   /* bytes — trocar para 8192 para testar 8KB */
#define L1_TAMANHO_BLOCO    32     /* bytes */
#define L1_ASSOCIATIVIDADE  2      /* vias — trocar para 4 para testar 4-way   */
#define L1_NUM_CONJUNTOS    (L1_CAPACIDADE / (L1_TAMANHO_BLOCO * L1_ASSOCIATIVIDADE))
 
/* =====================================================================
 * CACHE L2 (Unificada)
 * Capacidade: 32KB a 128KB
 * Tamanho do bloco: 64 bytes
 * Associatividade: 8-way ou 16-way
 *
 * Exemplo: 32768 / (64 x 8) = 64 conjuntos
 * ===================================================================== */
#define L2_CAPACIDADE       32768  /* bytes — trocar para 131072 para testar 128KB */
#define L2_TAMANHO_BLOCO    64     /* bytes */
#define L2_ASSOCIATIVIDADE  8      /* vias — trocar para 16 para testar 16-way     */
#define L2_NUM_CONJUNTOS    (L2_CAPACIDADE / (L2_TAMANHO_BLOCO * L2_ASSOCIATIVIDADE))
 
