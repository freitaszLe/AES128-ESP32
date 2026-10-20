/*
 * dupla.h — formação da chave e da mensagem a partir das matrículas da dupla.
 *
 * Chave (16 bytes): os 8 últimos dígitos da matrícula A (completados com '0'
 * à esquerda se houver menos de 8), seguidos dos 8 últimos dígitos da
 * matrícula B, cada dígito convertido para seu código ASCII ('2' = 0x32).
 *
 * Mensagem: o texto "A:<matrícula A>;B:<matrícula B>" com as matrículas
 * completas (sem o preenchimento, que é aplicado depois com PKCS#7).
 */
#ifndef DUPLA_H
#define DUPLA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DUPLA_DIGITOS 8   /* dígitos de cada matrícula usados na chave */

/* Monta a chave de 16 bytes. Caracteres que não são dígitos são ignorados. */
void dupla_montar_chave(const char *matricula_a, const char *matricula_b,
                        uint8_t chave[2 * DUPLA_DIGITOS]);

/* Escreve "A:<matrícula A>;B:<matrícula B>" em saida (com '\0' no final).
 * Retorna o tamanho da mensagem em bytes (sem o '\0'), ou 0 se não couber
 * em 'capacidade' bytes. */
size_t dupla_montar_mensagem(const char *matricula_a, const char *matricula_b,
                             char *saida, size_t capacidade);

#ifdef __cplusplus
}
#endif

#endif /* DUPLA_H */
