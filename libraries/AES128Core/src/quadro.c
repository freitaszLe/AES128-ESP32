/*
 * quadro.c — quadro de transmissão e máquina de estados do receptor.
 */
#include "quadro.h"

#include <string.h>

#include "pkcs7.h"

/* ========================================================================
 * Transmissor
 * ======================================================================== */

size_t quadro_cifrar_mensagem(const uint8_t chave[AES128_CHAVE],
                              const uint8_t *mensagem, size_t n,
                              uint8_t *quadro, size_t capacidade)
{
    uint8_t rk[AES128_CHAVES_RODADA_BYTES];
    size_t total;

    if (n > QUADRO_MAX_MENSAGEM || capacidade < 2 + pkcs7_tamanho_preenchido(n))
        return 0;

    /* Mensagem + PKCS#7 diretamente na área de dados do quadro */
    total = pkcs7_aplicar(mensagem, n, &quadro[2], capacidade - 2);

    aes128_expandir_chave(chave, rk);
    aes128_ecb_cifrar(rk, &quadro[2], &quadro[2], total / AES128_BLOCO);

    quadro[0] = QUADRO_MARCADOR;
    quadro[1] = (uint8_t)(total / AES128_BLOCO);
    return 2 + total;
}

/* ========================================================================
 * Receptor
 * ======================================================================== */

void quadro_receptor_iniciar(quadro_receptor_t *r, uint32_t timeout_ms)
{
    memset(r, 0, sizeof *r);
    r->estado = QUADRO_ESPERANDO_MARCADOR;
    r->timeout_ms = timeout_ms;
}

/* Volta a esperar um marcador. r->n_blocos e r->recebidos são mantidos para
 * que o chamador possa informar quanto do quadro descartado chegou; eles são
 * zerados quando o próximo marcador é recebido. */
static void reiniciar(quadro_receptor_t *r)
{
    r->estado = QUADRO_ESPERANDO_MARCADOR;
}

quadro_resultado_t quadro_receptor_byte(quadro_receptor_t *r, uint8_t byte, uint32_t agora_ms)
{
    /* Um quadro que já estourou o tempo é descartado antes de usar o byte novo */
    if (quadro_receptor_verificar_timeout(r, agora_ms) == QUADRO_ERRO_TIMEOUT) {
        quadro_receptor_byte(r, byte, agora_ms);   /* o byte pode iniciar outro quadro */
        return QUADRO_ERRO_TIMEOUT;
    }

    switch (r->estado) {
    case QUADRO_ESPERANDO_MARCADOR:
        if (byte == QUADRO_MARCADOR) {
            r->estado = QUADRO_LENDO_N;
            r->inicio_ms = agora_ms;
            r->n_blocos = 0;
            r->recebidos = 0;
        } else {
            r->descartados++;
        }
        return QUADRO_EM_ANDAMENTO;

    case QUADRO_LENDO_N:
        if (byte == 0 || byte > QUADRO_MAX_BLOCOS) {
            reiniciar(r);
            return QUADRO_ERRO_N_INVALIDO;
        }
        r->n_blocos = byte;
        r->recebidos = 0;
        r->estado = QUADRO_LENDO_BLOCOS;
        return QUADRO_EM_ANDAMENTO;

    case QUADRO_LENDO_BLOCOS:
        r->dados[r->recebidos++] = byte;
        if (r->recebidos == (size_t)r->n_blocos * AES128_BLOCO) {
            reiniciar(r);
            return QUADRO_COMPLETO;
        }
        return QUADRO_EM_ANDAMENTO;
    }
    return QUADRO_EM_ANDAMENTO;
}

quadro_resultado_t quadro_receptor_verificar_timeout(quadro_receptor_t *r, uint32_t agora_ms)
{
    /* A subtração em uint32_t continua correta quando millis() dá a volta */
    if (r->estado != QUADRO_ESPERANDO_MARCADOR &&
        (uint32_t)(agora_ms - r->inicio_ms) > r->timeout_ms) {
        reiniciar(r);
        return QUADRO_ERRO_TIMEOUT;
    }
    return QUADRO_EM_ANDAMENTO;
}

int quadro_decifrar(const uint8_t chave[AES128_CHAVE],
                    const uint8_t *dados, uint8_t n_blocos,
                    uint8_t *mensagem, size_t *n)
{
    uint8_t rk[AES128_CHAVES_RODADA_BYTES];

    aes128_expandir_chave(chave, rk);
    aes128_ecb_decifrar(rk, dados, mensagem, n_blocos);
    return pkcs7_remover(mensagem, (size_t)n_blocos * AES128_BLOCO, n);
}
