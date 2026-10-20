/*
 * quadro.h — quadro de transmissão pela UART e recepção com máquina de estados.
 *
 * Formato do quadro (bytes brutos):
 *
 *   | 0xAA (marcador) | n (nº de blocos) | 16 x n bytes cifrados |
 *
 * O receptor processa um byte por vez:
 *   ESPERANDO_MARCADOR --0xAA--> LENDO_N --n válido--> LENDO_BLOCOS --16n bytes--> COMPLETO
 * Bytes diferentes de 0xAA antes do marcador são descartados. Se o quadro não
 * terminar dentro do tempo limite (contado a partir do marcador), ele é
 * descartado e o receptor volta a esperar um novo marcador.
 *
 * O tempo é recebido como parâmetro (em ms) para que a lógica seja testável no
 * computador; no ESP32 usa-se millis().
 */
#ifndef QUADRO_H
#define QUADRO_H

#include <stddef.h>
#include <stdint.h>

#include "aes128.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QUADRO_MARCADOR     0xAA
#define QUADRO_MAX_BLOCOS   16                                    /* até 255 bytes de mensagem */
#define QUADRO_MAX_DADOS    (QUADRO_MAX_BLOCOS * AES128_BLOCO)    /* 256 bytes cifrados        */
#define QUADRO_MAX_BYTES    (2 + QUADRO_MAX_DADOS)                /* marcador + n + dados      */
#define QUADRO_MAX_MENSAGEM (QUADRO_MAX_DADOS - 1)                /* PKCS#7 acrescenta >= 1    */

/* ---- Transmissor -------------------------------------------------------- */

/* Aplica PKCS#7 à mensagem, cifra em ECB com a chave e monta o quadro.
 * Retorna o tamanho do quadro em bytes, ou 0 se a mensagem for longa demais
 * ou não couber em 'capacidade'. */
size_t quadro_cifrar_mensagem(const uint8_t chave[AES128_CHAVE],
                              const uint8_t *mensagem, size_t n,
                              uint8_t *quadro, size_t capacidade);

/* ---- Receptor ----------------------------------------------------------- */

typedef enum {
    QUADRO_ESPERANDO_MARCADOR,
    QUADRO_LENDO_N,
    QUADRO_LENDO_BLOCOS
} quadro_estado_t;

typedef enum {
    QUADRO_EM_ANDAMENTO,      /* nada a fazer ainda                               */
    QUADRO_COMPLETO,          /* quadro inteiro recebido: usar n_blocos e dados   */
    QUADRO_ERRO_N_INVALIDO,   /* n = 0 ou maior que QUADRO_MAX_BLOCOS             */
    QUADRO_ERRO_TIMEOUT       /* quadro incompleto dentro do tempo limite         */
} quadro_resultado_t;

typedef struct {
    quadro_estado_t estado;
    uint32_t timeout_ms;
    uint32_t inicio_ms;            /* instante em que o marcador foi recebido */
    uint8_t  n_blocos;
    size_t   recebidos;            /* bytes cifrados já recebidos             */
    uint32_t descartados;          /* bytes ignorados fora de um quadro       */
    uint8_t  dados[QUADRO_MAX_DADOS];
} quadro_receptor_t;

void quadro_receptor_iniciar(quadro_receptor_t *r, uint32_t timeout_ms);

/* Processa um byte recebido no instante 'agora_ms'. Quando retorna
 * QUADRO_COMPLETO, r->n_blocos e r->dados contêm o quadro; o receptor já
 * fica pronto para o próximo. */
quadro_resultado_t quadro_receptor_byte(quadro_receptor_t *r, uint8_t byte, uint32_t agora_ms);

/* Deve ser chamada periodicamente: descarta um quadro incompleto cujo tempo
 * limite tenha expirado (retorna QUADRO_ERRO_TIMEOUT nesse caso). */
quadro_resultado_t quadro_receptor_verificar_timeout(quadro_receptor_t *r, uint32_t agora_ms);

/* Decifra os blocos recebidos e remove o PKCS#7. Escreve a mensagem em
 * 'mensagem' (capacidade mínima 16 x n_blocos) e o tamanho em *n.
 * Retorna 0, ou -1 se o preenchimento for inválido (chave errada ou dados
 * corrompidos). */
int quadro_decifrar(const uint8_t chave[AES128_CHAVE],
                    const uint8_t *dados, uint8_t n_blocos,
                    uint8_t *mensagem, size_t *n);

#ifdef __cplusplus
}
#endif

#endif /* QUADRO_H */
