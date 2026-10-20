/*
 * teste.h — mini-framework de testes (sem dependências externas).
 *
 * CHECAR(cond)              verifica uma condição.
 * CHECAR_BYTES(a, b, n, m)  compara n bytes e, se diferirem, mostra os dois em hexadecimal.
 * Ao final, main() chama teste_resumo(), que retorna 0 se tudo passou.
 */
#ifndef TESTE_H
#define TESTE_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>

static int teste_total = 0;
static int teste_falhas = 0;

#if defined(__GNUC__)
__attribute__((unused))
#endif
static void teste_hex(const char *rotulo, const unsigned char *p, size_t n)
{
    size_t i;
    printf("      %s ", rotulo);
    for (i = 0; i < n; i++) printf("%02x", p[i]);
    printf("\n");
}

#define CHECAR(cond)                                                        \
    do {                                                                    \
        teste_total++;                                                      \
        if (!(cond)) {                                                      \
            teste_falhas++;                                                 \
            printf("  FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                   \
    } while (0)

#define CHECAR_BYTES(obtido, esperado, n, nome)                             \
    do {                                                                    \
        teste_total++;                                                      \
        if (memcmp((obtido), (esperado), (n)) != 0) {                       \
            teste_falhas++;                                                 \
            printf("  FALHOU %s:%d: %s\n", __FILE__, __LINE__, (nome));     \
            teste_hex("obtido:  ", (const unsigned char *)(obtido), (n));   \
            teste_hex("esperado:", (const unsigned char *)(esperado), (n)); \
        }                                                                   \
    } while (0)

static int teste_resumo(void)
{
    printf("\n%d verificações, %d falha(s) -> %s\n", teste_total, teste_falhas,
           teste_falhas == 0 ? "OK" : "FALHOU");
    return teste_falhas == 0 ? 0 : 1;
}

#endif /* TESTE_H */
