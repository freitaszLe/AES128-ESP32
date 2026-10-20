/*
 * pkcs7.c — preenchimento PKCS#7 para blocos de 16 bytes.
 */
#include "pkcs7.h"

#include <string.h>

size_t pkcs7_tamanho_preenchido(size_t n)
{
    return n + (PKCS7_BLOCO - n % PKCS7_BLOCO);   /* sempre acrescenta de 1 a 16 */
}

size_t pkcs7_aplicar(const uint8_t *dados, size_t n, uint8_t *saida, size_t capacidade)
{
    size_t total = pkcs7_tamanho_preenchido(n);
    uint8_t valor = (uint8_t)(total - n);         /* quantidade acrescentada */

    if (total > capacidade)
        return 0;

    memmove(saida, dados, n);
    memset(saida + n, valor, valor);
    return total;
}

int pkcs7_remover(const uint8_t *dados, size_t n, size_t *n_original)
{
    uint8_t valor;
    size_t i;

    if (n == 0 || n % PKCS7_BLOCO != 0)
        return -1;

    valor = dados[n - 1];
    if (valor == 0 || valor > PKCS7_BLOCO)
        return -1;

    for (i = n - valor; i < n; i++)
        if (dados[i] != valor)
            return -1;

    *n_original = n - valor;
    return 0;
}
