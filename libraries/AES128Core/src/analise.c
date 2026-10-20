/*
 * analise.c — registros e experimentos sobre o AES-128 (ver analise.h).
 *
 * O texto do relatório usa apenas ASCII, para ser exibido corretamente em
 * qualquer monitor serial.
 */
#include "analise.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "dupla.h"
#include "pkcs7.h"

#define MAX_MENSAGEM 128                         /* bytes da mensagem sem preenchimento */
#define MAX_DADOS    (MAX_MENSAGEM + PKCS7_BLOCO) /* com o preenchimento               */

/* ========================================================================
 * Funções básicas
 * ======================================================================== */

void analise_estados_rodada1(const uint8_t chaves_rodada[AES128_CHAVES_RODADA_BYTES],
                             const uint8_t bloco[AES128_BLOCO],
                             uint8_t estados[ANALISE_ESTADOS_RODADA1][AES128_BLOCO])
{
    uint8_t s[AES128_BLOCO];
    memcpy(s, bloco, AES128_BLOCO);

    aes128_add_round_key(s, &chaves_rodada[0]);             memcpy(estados[0], s, AES128_BLOCO);
    aes128_sub_bytes(s);                                    memcpy(estados[1], s, AES128_BLOCO);
    aes128_shift_rows(s);                                   memcpy(estados[2], s, AES128_BLOCO);
    aes128_mix_columns(s);                                  memcpy(estados[3], s, AES128_BLOCO);
    aes128_add_round_key(s, &chaves_rodada[AES128_BLOCO]);  memcpy(estados[4], s, AES128_BLOCO);
}

unsigned analise_bits_diferentes(const uint8_t *a, const uint8_t *b, size_t n)
{
    unsigned total = 0;
    size_t i;
    for (i = 0; i < n; i++) {
        uint8_t x = (uint8_t)(a[i] ^ b[i]);   /* bits 1 = posições diferentes */
        while (x) {
            total += x & 1u;
            x >>= 1;
        }
    }
    return total;
}

void analise_inverter_bit(uint8_t *dados, size_t bit)
{
    dados[bit / 8] ^= (uint8_t)(0x80u >> (bit % 8));
}

/* ========================================================================
 * Formatação das linhas do relatório
 * ======================================================================== */

static void linha(analise_saida_t saida, const char *formato, ...)
{
    char buf[160];
    va_list args;
    va_start(args, formato);
    vsnprintf(buf, sizeof buf, formato, args);
    va_end(args);
    saida(buf);
}

/* Escreve "rotulo: <16 bytes em hex>" e, se houver mais blocos, uma linha
 * por bloco alinhada abaixo da primeira. */
static void linha_hex(analise_saida_t saida, const char *rotulo,
                      const uint8_t *p, size_t n)
{
    char buf[160];
    size_t i, bloco;
    for (bloco = 0; bloco * AES128_BLOCO < n; bloco++) {
        int pos = snprintf(buf, sizeof buf, "  %-24s: ", bloco == 0 ? rotulo : "");
        for (i = bloco * AES128_BLOCO; i < n && i < (bloco + 1) * AES128_BLOCO; i++)
            pos += snprintf(buf + pos, sizeof buf - (size_t)pos, "%02x", p[i]);
        saida(buf);
    }
}

static double porcentagem(unsigned parte, unsigned total)
{
    return total ? 100.0 * parte / total : 0.0;
}

/* Linha com bits alterados por bloco e no total. */
static void linha_bits(analise_saida_t saida, const uint8_t *a, const uint8_t *b, size_t n)
{
    size_t bloco;
    unsigned total = 0;
    for (bloco = 0; bloco * AES128_BLOCO < n; bloco++) {
        unsigned d = analise_bits_diferentes(&a[bloco * AES128_BLOCO],
                                             &b[bloco * AES128_BLOCO], AES128_BLOCO);
        total += d;
        linha(saida, "    bloco %-16u: %3u de 128 bits (%.2f%%)", (unsigned)(bloco + 1), d,
              porcentagem(d, 128));
    }
    linha(saida, "    %-22s: %3u de %u bits (%.2f%%)", "total", total,
          (unsigned)(8 * n), porcentagem(total, (unsigned)(8 * n)));
}

/* ========================================================================
 * Relatório
 * ======================================================================== */

/* Cifra 'claro' (n bytes, múltiplo de 16) com 'chave' em ECB. */
static void cifrar_ecb(const uint8_t chave[AES128_CHAVE], const uint8_t *claro,
                       uint8_t *cifrado, size_t n)
{
    uint8_t rk[AES128_CHAVES_RODADA_BYTES];
    aes128_expandir_chave(chave, rk);
    aes128_ecb_cifrar(rk, claro, cifrado, n / AES128_BLOCO);
}

static void secao_avalanche(analise_saida_t saida, const uint8_t chave[AES128_CHAVE],
                            const uint8_t *claro, const uint8_t *cifrado, size_t n)
{
    uint8_t chave2[AES128_CHAVE], claro2[MAX_DADOS], cifrado2[MAX_DADOS];
    const size_t BIT_CHAVE = 127;   /* último bit da chave: muda o último dígito de B */
    const size_t BIT_MSG = 7;       /* último bit do 1o byte: 'A' (0x41) -> '@' (0x40) */
    unsigned soma, minimo, maximo, d;
    size_t bit;

    linha(saida, "");
    linha(saida, "[4] Efeito avalanche");

    /* 4.1: um bit da chave */
    memcpy(chave2, chave, AES128_CHAVE);
    analise_inverter_bit(chave2, BIT_CHAVE);
    cifrar_ecb(chave2, claro, cifrado2, n);

    linha(saida, "4.1 Bit %u da chave invertido (ultimo bit)", (unsigned)BIT_CHAVE);
    linha(saida, "  %-24s: %.16s", "Chave original (ASCII)", (const char *)chave);
    linha(saida, "  %-24s: %.16s", "Chave alterada (ASCII)", (const char *)chave2);
    linha_hex(saida, "Chave alterada (hex)", chave2, AES128_CHAVE);
    linha_hex(saida, "Cifrado original", cifrado, n);
    linha_hex(saida, "Cifrado alterado", cifrado2, n);
    linha(saida, "  Bits alterados no texto cifrado:");
    linha_bits(saida, cifrado, cifrado2, n);

    /* 4.2: um bit da mensagem */
    memcpy(claro2, claro, n);
    analise_inverter_bit(claro2, BIT_MSG);
    cifrar_ecb(chave, claro2, cifrado2, n);

    linha(saida, "4.2 Bit %u da mensagem invertido ('%c' -> '%c')", (unsigned)BIT_MSG,
          claro[BIT_MSG / 8], claro2[BIT_MSG / 8]);
    linha_hex(saida, "Mensagem alterada", claro2, n);
    linha_hex(saida, "Cifrado alterado", cifrado2, n);
    linha(saida, "  Bits alterados no texto cifrado:");
    linha_bits(saida, cifrado, cifrado2, n);
    if (n > AES128_BLOCO)
        linha(saida, "  Em ECB a alteracao no bloco 1 nao afeta os demais blocos.");

    /* 4.3: média ao inverter, uma de cada vez, todas as posições */
    linha(saida, "4.3 Media invertendo cada bit, um de cada vez");

    soma = 0; minimo = 8u * (unsigned)n; maximo = 0;
    for (bit = 0; bit < 8 * AES128_CHAVE; bit++) {
        memcpy(chave2, chave, AES128_CHAVE);
        analise_inverter_bit(chave2, bit);
        cifrar_ecb(chave2, claro, cifrado2, n);
        d = analise_bits_diferentes(cifrado, cifrado2, n);
        soma += d;
        if (d < minimo) minimo = d;
        if (d > maximo) maximo = d;
    }
    linha(saida, "  128 bits da chave       : media %.2f%% de %u bits (min %.2f%%, max %.2f%%)",
          porcentagem(soma, 128u * 8u * (unsigned)n), (unsigned)(8 * n),
          porcentagem(minimo, 8u * (unsigned)n), porcentagem(maximo, 8u * (unsigned)n));

    soma = 0; minimo = 128; maximo = 0;
    for (bit = 0; bit < 8 * AES128_BLOCO; bit++) {
        memcpy(claro2, claro, n);
        analise_inverter_bit(claro2, bit);
        cifrar_ecb(chave, claro2, cifrado2, AES128_BLOCO);   /* só o bloco 1 muda */
        d = analise_bits_diferentes(cifrado, cifrado2, AES128_BLOCO);
        soma += d;
        if (d < minimo) minimo = d;
        if (d > maximo) maximo = d;
    }
    linha(saida, "  128 bits do bloco 1     : media %.2f%% de 128 bits (min %.2f%%, max %.2f%%)",
          porcentagem(soma, 128u * 128u), porcentagem(minimo, 128), porcentagem(maximo, 128));
}

static void secao_ecb(analise_saida_t saida)
{
    static const char *BLOCOS[3] = { "LIGAR LED VERDE.", "DESLIGAR LED    ", "LIGAR LED VERDE." };
    uint8_t chave[AES128_CHAVE], claro[3 * AES128_BLOCO], cifrado[3 * AES128_BLOCO];
    char rotulo[32];
    int i;

    dupla_montar_chave("20231001", "20231002", chave);
    for (i = 0; i < 3; i++)
        memcpy(&claro[AES128_BLOCO * i], BLOCOS[i], AES128_BLOCO);
    cifrar_ecb(chave, claro, cifrado, sizeof claro);

    linha(saida, "");
    linha(saida, "[5] Experimento ECB: blocos repetidos (chave do exemplo do enunciado)");
    linha_hex(saida, "Chave", chave, AES128_CHAVE);
    for (i = 0; i < 3; i++) {
        snprintf(rotulo, sizeof rotulo, "Bloco %d \"%s\"", i + 1, BLOCOS[i]);
        linha_hex(saida, rotulo, &cifrado[AES128_BLOCO * i], AES128_BLOCO);
    }
    linha(saida, "  Cifrado do bloco 3 igual ao do bloco 1: %s",
          memcmp(&cifrado[0], &cifrado[2 * AES128_BLOCO], AES128_BLOCO) == 0 ? "SIM" : "NAO");
}

void analise_relatorio(const char *matricula_a, const char *matricula_b,
                       analise_saida_t saida)
{
    static const char *FASES[ANALISE_ESTADOS_RODADA1] = {
        "(1) AddRoundKey chave 0", "(2) SubBytes", "(3) ShiftRows",
        "(4) MixColumns", "(5) AddRoundKey chave 1",
    };
    uint8_t chave[AES128_CHAVE], rk[AES128_CHAVES_RODADA_BYTES];
    uint8_t claro[MAX_DADOS], cifrado[MAX_DADOS];
    uint8_t estados[ANALISE_ESTADOS_RODADA1][AES128_BLOCO];
    char mensagem[MAX_MENSAGEM + 1];
    size_t n, total;
    int i;

    linha(saida, "===== ANALISE AES-128 - DADOS DA DUPLA =====");

    n = dupla_montar_mensagem(matricula_a, matricula_b, mensagem, sizeof mensagem);
    total = pkcs7_aplicar((const uint8_t *)mensagem, n, claro, sizeof claro);
    if (n == 0 || total == 0) {
        linha(saida, "ERRO: mensagem maior que %d bytes", MAX_MENSAGEM);
        return;
    }

    /* [1] Dados */
    dupla_montar_chave(matricula_a, matricula_b, chave);
    aes128_expandir_chave(chave, rk);
    aes128_ecb_cifrar(rk, claro, cifrado, total / AES128_BLOCO);

    linha(saida, "[1] Dados");
    linha(saida, "  %-24s: %s", "Matricula A", matricula_a);
    linha(saida, "  %-24s: %s", "Matricula B", matricula_b);
    linha(saida, "  %-24s: %.16s", "Chave (ASCII)", (const char *)chave);
    linha_hex(saida, "Chave (hex)", chave, AES128_CHAVE);
    linha(saida, "  %-24s: %s (%u bytes)", "Mensagem", mensagem, (unsigned)n);
    linha_hex(saida, "Mensagem (hex)", (const uint8_t *)mensagem, n);
    linha(saida, "  PKCS#7: %u byte(s) de valor 0x%02x -> %u bytes, %u bloco(s)",
          (unsigned)(total - n), (unsigned)(total - n), (unsigned)total,
          (unsigned)(total / AES128_BLOCO));
    linha_hex(saida, "Com PKCS#7", claro, total);
    linha_hex(saida, "Texto cifrado (ECB)", cifrado, total);
    linha(saida, "  %-24s: AA %02x + %u bytes cifrados (%u bytes)", "Quadro UART",
          (unsigned)(total / AES128_BLOCO), (unsigned)total, (unsigned)(total + 2));

    /* [2] Chaves de rodada */
    linha(saida, "");
    linha(saida, "[2] Chaves de rodada");
    linha_hex(saida, "Rodada 0", &rk[0], AES128_BLOCO);
    linha_hex(saida, "Rodada 1", &rk[AES128_BLOCO], AES128_BLOCO);

    /* [3] Estados do bloco 1 na rodada 1 */
    analise_estados_rodada1(rk, claro, estados);
    linha(saida, "");
    linha(saida, "[3] Estados do bloco 1 na rodada 1");
    linha_hex(saida, "Entrada (bloco 1)", claro, AES128_BLOCO);
    for (i = 0; i < ANALISE_ESTADOS_RODADA1; i++)
        linha_hex(saida, FASES[i], estados[i], AES128_BLOCO);

    secao_avalanche(saida, chave, claro, cifrado, total);
    secao_ecb(saida);

    linha(saida, "===== FIM DA ANALISE =====");
}
