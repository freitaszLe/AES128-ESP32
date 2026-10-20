/*
 * dupla.c — formação da chave e da mensagem a partir das matrículas.
 */
#include "dupla.h"

#include <string.h>

/* Escreve em destino[0..7] os 8 últimos dígitos de 'matricula', em ASCII,
 * completando com '0' à esquerda quando a matrícula tem menos de 8 dígitos. */
static void ultimos_digitos(const char *matricula, uint8_t destino[DUPLA_DIGITOS])
{
    int pos = DUPLA_DIGITOS - 1;
    size_t i = strlen(matricula);

    /* Percorre a matrícula do fim para o início, copiando só os dígitos. */
    while (i > 0 && pos >= 0) {
        char c = matricula[--i];
        if (c >= '0' && c <= '9')
            destino[pos--] = (uint8_t)c;   /* o próprio caractere já é o código ASCII */
    }

    /* Posições que sobraram à esquerda recebem '0' (0x30). */
    while (pos >= 0)
        destino[pos--] = (uint8_t)'0';
}

void dupla_montar_chave(const char *matricula_a, const char *matricula_b,
                        uint8_t chave[2 * DUPLA_DIGITOS])
{
    ultimos_digitos(matricula_a, &chave[0]);              /* bytes 0..7:  aluno A */
    ultimos_digitos(matricula_b, &chave[DUPLA_DIGITOS]);  /* bytes 8..15: aluno B */
}

size_t dupla_montar_mensagem(const char *matricula_a, const char *matricula_b,
                             char *saida, size_t capacidade)
{
    size_t na = strlen(matricula_a), nb = strlen(matricula_b);
    size_t total = 2 + na + 3 + nb;   /* "A:" + A + ";B:" + B */

    if (total + 1 > capacidade)       /* +1 para o '\0' */
        return 0;

    memcpy(saida, "A:", 2);
    memcpy(saida + 2, matricula_a, na);
    memcpy(saida + 2 + na, ";B:", 3);
    memcpy(saida + 5 + na, matricula_b, nb);
    saida[total] = '\0';
    return total;
}
