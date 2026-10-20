/*
 * analise.h — registros e experimentos sobre o AES-128 com os dados da dupla:
 * chaves de rodada, estados intermediários da rodada 1, efeito avalanche e
 * o experimento de blocos repetidos em ECB.
 *
 * O relatório é gerado como texto e entregue linha a linha a uma função de
 * saída. Assim o mesmo código imprime no Monitor Serial (ESP32) e no
 * terminal do computador.
 */
#ifndef ANALISE_H
#define ANALISE_H

#include <stddef.h>
#include <stdint.h>

#include "aes128.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Recebe cada linha do relatório (sem '\n' no final). */
typedef void (*analise_saida_t)(const char *linha);

/* Número de estados registrados na rodada 1. */
#define ANALISE_ESTADOS_RODADA1 5

/* Aplica ao bloco a transformação inicial e a rodada 1, guardando o estado
 * após cada fase:
 *   [0] AddRoundKey inicial (chave 0)   [1] SubBytes   [2] ShiftRows
 *   [3] MixColumns                       [4] AddRoundKey (chave 1) */
void analise_estados_rodada1(const uint8_t chaves_rodada[AES128_CHAVES_RODADA_BYTES],
                             const uint8_t bloco[AES128_BLOCO],
                             uint8_t estados[ANALISE_ESTADOS_RODADA1][AES128_BLOCO]);

/* Conta quantos bits diferem entre a e b (n bytes). */
unsigned analise_bits_diferentes(const uint8_t *a, const uint8_t *b, size_t n);

/* Inverte o bit de índice 'bit'. Numeração da esquerda para a direita, como
 * na escrita hexadecimal: bit 0 é o mais significativo do byte 0 e bit 7 é o
 * menos significativo do byte 0. */
void analise_inverter_bit(uint8_t *dados, size_t bit);

/* Gera o relatório completo com os dados das matrículas informadas. */
void analise_relatorio(const char *matricula_a, const char *matricula_b,
                       analise_saida_t saida);

#ifdef __cplusplus
}
#endif

#endif /* ANALISE_H */
