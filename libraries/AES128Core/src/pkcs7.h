/*
 * pkcs7.h — preenchimento PKCS#7 para blocos de 16 bytes.
 *
 * Acrescentam-se de 1 a 16 bytes, todos com valor igual à quantidade
 * acrescentada, até o tamanho ser múltiplo de 16. Se a mensagem já tiver
 * tamanho múltiplo de 16, acrescenta-se um bloco inteiro de 0x10: assim o
 * último byte SEMPRE indica quanto preenchimento existe, sem ambiguidade.
 */
#ifndef PKCS7_H
#define PKCS7_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PKCS7_BLOCO 16

/* Tamanho final após o preenchimento de uma mensagem de n bytes. */
size_t pkcs7_tamanho_preenchido(size_t n);

/* Copia os n bytes de 'dados' para 'saida' e acrescenta o preenchimento.
 * Retorna o tamanho final, ou 0 se não couber em 'capacidade' bytes.
 * 'dados' e 'saida' podem ser o mesmo vetor. */
size_t pkcs7_aplicar(const uint8_t *dados, size_t n, uint8_t *saida, size_t capacidade);

/* Valida o preenchimento de 'dados' (n bytes) e devolve em *n_original o
 * tamanho da mensagem sem ele. Retorna 0 se válido ou -1 se inválido
 * (n zero ou não múltiplo de 16, último byte 0 ou maior que 16, ou bytes de
 * preenchimento diferentes entre si). */
int pkcs7_remover(const uint8_t *dados, size_t n, size_t *n_original);

#ifdef __cplusplus
}
#endif

#endif /* PKCS7_H */
