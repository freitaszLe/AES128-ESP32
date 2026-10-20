/*
 * aes128.h — interface do AES-128 (FIPS-197), implementado do zero.
 *
 * Convenções:
 *  - Bloco e estado: 16 bytes. O estado é a matriz 4x4 do FIPS-197 preenchida
 *    por colunas: o byte da linha r, coluna c fica em estado[r + 4*c].
 *  - Chaves de rodada: 11 chaves de 16 bytes (176 bytes) geradas pela expansão
 *    de chave. A chave da rodada r ocupa chaves_rodada[16*r .. 16*r + 15].
 *
 * As transformações são públicas para permitir testá-las isoladamente e
 * registrar os estados intermediários de cada rodada.
 */
#ifndef AES128_H
#define AES128_H

#include <stdint.h>

/* Os sketches são compilados como C++; o núcleo, como C. O bloco extern "C"
 * evita que o compilador C++ altere os nomes das funções (name mangling). */
#ifdef __cplusplus
extern "C" {
#endif

#define AES128_BLOCO               16   /* bytes por bloco                        */
#define AES128_CHAVE               16   /* bytes da chave (128 bits)              */
#define AES128_RODADAS             10   /* Nr para chave de 128 bits              */
#define AES128_CHAVES_RODADA_BYTES (AES128_BLOCO * (AES128_RODADAS + 1)) /* 176 */

/* Versão do núcleo, usada para confirmar que a biblioteca foi ligada ao programa. */
const char *aes128core_versao(void);

/* ---- Aritmética no corpo GF(2^8) ---------------------------------------- */

/* Multiplica a por x (isto é, por {02}) módulo m(x) = x^8 + x^4 + x^3 + x + 1. */
uint8_t aes128_xtime(uint8_t a);

/* Multiplicação genérica a * b em GF(2^8), construída com xtime. */
uint8_t aes128_gf_mult(uint8_t a, uint8_t b);

/* Valor da S-box e da S-box inversa para o byte b. */
uint8_t aes128_sbox(uint8_t b);
uint8_t aes128_inv_sbox(uint8_t b);

/* ---- Transformações da cifragem (alteram o estado no lugar) -------------- */

void aes128_sub_bytes(uint8_t estado[AES128_BLOCO]);
void aes128_shift_rows(uint8_t estado[AES128_BLOCO]);
void aes128_mix_columns(uint8_t estado[AES128_BLOCO]);
void aes128_add_round_key(uint8_t estado[AES128_BLOCO],
                          const uint8_t chave_rodada[AES128_BLOCO]);

/* ---- Transformações inversas, usadas na decifragem ----------------------- */

void aes128_inv_sub_bytes(uint8_t estado[AES128_BLOCO]);
void aes128_inv_shift_rows(uint8_t estado[AES128_BLOCO]);
void aes128_inv_mix_columns(uint8_t estado[AES128_BLOCO]);

/* ---- Expansão de chave, cifragem e decifragem de um bloco ----------------- */

/* Gera as 11 chaves de rodada (44 palavras de 32 bits) a partir da chave. */
void aes128_expandir_chave(const uint8_t chave[AES128_CHAVE],
                           uint8_t chaves_rodada[AES128_CHAVES_RODADA_BYTES]);

/* Cifra um bloco de 16 bytes. entrada e saida podem ser o mesmo vetor. */
void aes128_cifrar_bloco(const uint8_t chaves_rodada[AES128_CHAVES_RODADA_BYTES],
                         const uint8_t entrada[AES128_BLOCO],
                         uint8_t saida[AES128_BLOCO]);

/* Decifra um bloco de 16 bytes, usando as mesmas chaves de rodada da cifragem
 * em ordem inversa. entrada e saida podem ser o mesmo vetor. */
void aes128_decifrar_bloco(const uint8_t chaves_rodada[AES128_CHAVES_RODADA_BYTES],
                           const uint8_t entrada[AES128_BLOCO],
                           uint8_t saida[AES128_BLOCO]);

#ifdef __cplusplus
}
#endif

#endif /* AES128_H */
