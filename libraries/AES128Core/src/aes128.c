/*
 * aes128.c — AES-128 (FIPS-197) implementado do zero, sem bibliotecas
 * criptográficas.
 *
 * Estado: vetor de 16 bytes em que o byte da linha r, coluna c está em
 * estado[r + 4*c] (matriz 4x4 preenchida por colunas, FIPS-197 seção 3.4).
 */
#include "aes128.h"

#include <string.h>

const char *aes128core_versao(void)
{
    return "0.6.0";
}

/* ========================================================================
 * Aritmética em GF(2^8)
 *
 * Cada byte representa um polinômio de grau < 8 com coeficientes em {0,1}
 * (bit i = coeficiente de x^i). A soma é XOR. A multiplicação é feita
 * módulo o polinômio irredutível m(x) = x^8 + x^4 + x^3 + x + 1 (0x11B).
 * ======================================================================== */

uint8_t aes128_xtime(uint8_t a)
{
    /* Multiplicar por x desloca os coeficientes uma posição para a esquerda.
     * Se o bit 7 era 1, surge um termo x^8, que é reduzido somando (XOR)
     * x^4 + x^3 + x + 1 = 0x1B. */
    return (uint8_t)((a << 1) ^ ((a & 0x80) ? 0x1B : 0x00));
}

uint8_t aes128_gf_mult(uint8_t a, uint8_t b)
{
    /* Método "multiplicar e somar": percorre os bits de b. Para cada bit 1,
     * soma a potência correspondente a * x^i, obtida com xtime sucessivos. */
    uint8_t resultado = 0;
    while (b) {
        if (b & 1)
            resultado ^= a;
        a = aes128_xtime(a);
        b >>= 1;
    }
    return resultado;
}

/* ========================================================================
 * S-box (FIPS-197, figura 7)
 *
 * Origem: para cada byte b, calcula-se o inverso multiplicativo b^-1 em
 * GF(2^8) (0 é mapeado para 0) e aplica-se a transformação afim sobre GF(2):
 *     s = b' ^ rotl(b',1) ^ rotl(b',2) ^ rotl(b',3) ^ rotl(b',4) ^ 0x63
 * O inverso dá a não linearidade; a transformação afim elimina pontos fixos
 * e dificulta descrições algébricas simples. A tabela abaixo é o resultado
 * desse cálculo; os testes a recalculam e comparam com a tabela.
 * ======================================================================== */

static const uint8_t SBOX[256] = {
    /*        0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
    /*0*/ 0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    /*1*/ 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    /*2*/ 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    /*3*/ 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    /*4*/ 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    /*5*/ 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    /*6*/ 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    /*7*/ 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    /*8*/ 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    /*9*/ 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    /*a*/ 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    /*b*/ 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    /*c*/ 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    /*d*/ 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    /*e*/ 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    /*f*/ 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

uint8_t aes128_sbox(uint8_t b)
{
    return SBOX[b];
}

/* S-box inversa (FIPS-197, figura 14): INV_SBOX[SBOX[b]] = b.
 * Obtida invertendo a tabela acima; os testes verificam essa relação. */
static const uint8_t INV_SBOX[256] = {
    /*        0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
    /*0*/ 0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    /*1*/ 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    /*2*/ 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    /*3*/ 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    /*4*/ 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    /*5*/ 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    /*6*/ 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    /*7*/ 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    /*8*/ 0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    /*9*/ 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    /*a*/ 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    /*b*/ 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    /*c*/ 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    /*d*/ 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    /*e*/ 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    /*f*/ 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

uint8_t aes128_inv_sbox(uint8_t b)
{
    return INV_SBOX[b];
}

/* ========================================================================
 * Transformações da cifragem
 * ======================================================================== */

/* SubBytes: substitui cada byte do estado pelo valor da S-box.
 * É a única etapa não linear do AES (confusão). */
void aes128_sub_bytes(uint8_t estado[AES128_BLOCO])
{
    int i;
    for (i = 0; i < AES128_BLOCO; i++)
        estado[i] = SBOX[estado[i]];
}

/* ShiftRows: a linha r é deslocada ciclicamente r posições para a esquerda
 * (linha 0 fica igual). Espalha os bytes de cada coluna por colunas
 * diferentes, para que o MixColumns seguinte misture bytes de origens
 * distintas (difusão entre colunas). */
void aes128_shift_rows(uint8_t estado[AES128_BLOCO])
{
    uint8_t tmp[AES128_BLOCO];
    int r, c;
    for (c = 0; c < 4; c++)
        for (r = 0; r < 4; r++)
            tmp[r + 4 * c] = estado[r + 4 * ((c + r) % 4)];
    memcpy(estado, tmp, AES128_BLOCO);
}

/* MixColumns: cada coluna é tratada como um polinômio e multiplicada por
 * a(x) = {03}x^3 + {01}x^2 + {01}x + {02}, o que equivale à matriz:
 *     | 02 03 01 01 |
 *     | 01 02 03 01 |
 *     | 01 01 02 03 |
 *     | 03 01 01 02 |
 * Cada byte de saída depende dos 4 bytes da coluna (difusão dentro da coluna).
 * Multiplicar por {02} é xtime; por {03} é xtime(a) ^ a. */
void aes128_mix_columns(uint8_t estado[AES128_BLOCO])
{
    int c;
    for (c = 0; c < 4; c++) {
        uint8_t *col = &estado[4 * c];
        uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];

        col[0] = (uint8_t)(aes128_xtime(a0) ^ (aes128_xtime(a1) ^ a1) ^ a2 ^ a3);
        col[1] = (uint8_t)(a0 ^ aes128_xtime(a1) ^ (aes128_xtime(a2) ^ a2) ^ a3);
        col[2] = (uint8_t)(a0 ^ a1 ^ aes128_xtime(a2) ^ (aes128_xtime(a3) ^ a3));
        col[3] = (uint8_t)((aes128_xtime(a0) ^ a0) ^ a1 ^ a2 ^ aes128_xtime(a3));
    }
}

/* AddRoundKey: XOR do estado com a chave da rodada. É a única etapa que
 * usa a chave; como XOR é sua própria inversa, também serve para decifrar. */
void aes128_add_round_key(uint8_t estado[AES128_BLOCO],
                          const uint8_t chave_rodada[AES128_BLOCO])
{
    int i;
    for (i = 0; i < AES128_BLOCO; i++)
        estado[i] ^= chave_rodada[i];
}

/* ========================================================================
 * Transformações inversas (FIPS-197, seção 5.3)
 * ======================================================================== */

/* InvSubBytes: substitui cada byte pelo valor da S-box inversa. */
void aes128_inv_sub_bytes(uint8_t estado[AES128_BLOCO])
{
    int i;
    for (i = 0; i < AES128_BLOCO; i++)
        estado[i] = INV_SBOX[estado[i]];
}

/* InvShiftRows: a linha r é deslocada ciclicamente r posições para a
 * DIREITA, desfazendo o ShiftRows. */
void aes128_inv_shift_rows(uint8_t estado[AES128_BLOCO])
{
    uint8_t tmp[AES128_BLOCO];
    int r, c;
    for (c = 0; c < 4; c++)
        for (r = 0; r < 4; r++)
            tmp[r + 4 * c] = estado[r + 4 * ((c - r + 4) % 4)];
    memcpy(estado, tmp, AES128_BLOCO);
}

/* InvMixColumns: multiplica cada coluna pelo polinômio inverso
 * a^-1(x) = {0b}x^3 + {0d}x^2 + {09}x + {0e}, ou seja, pela matriz:
 *     | 0e 0b 0d 09 |
 *     | 09 0e 0b 0d |
 *     | 0d 09 0e 0b |
 *     | 0b 0d 09 0e |
 * Essa matriz é a inversa, em GF(2^8), da matriz usada no MixColumns. */
void aes128_inv_mix_columns(uint8_t estado[AES128_BLOCO])
{
    int c;
    for (c = 0; c < 4; c++) {
        uint8_t *col = &estado[4 * c];
        uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];

        col[0] = (uint8_t)(aes128_gf_mult(a0, 0x0e) ^ aes128_gf_mult(a1, 0x0b) ^
                           aes128_gf_mult(a2, 0x0d) ^ aes128_gf_mult(a3, 0x09));
        col[1] = (uint8_t)(aes128_gf_mult(a0, 0x09) ^ aes128_gf_mult(a1, 0x0e) ^
                           aes128_gf_mult(a2, 0x0b) ^ aes128_gf_mult(a3, 0x0d));
        col[2] = (uint8_t)(aes128_gf_mult(a0, 0x0d) ^ aes128_gf_mult(a1, 0x09) ^
                           aes128_gf_mult(a2, 0x0e) ^ aes128_gf_mult(a3, 0x0b));
        col[3] = (uint8_t)(aes128_gf_mult(a0, 0x0b) ^ aes128_gf_mult(a1, 0x0d) ^
                           aes128_gf_mult(a2, 0x09) ^ aes128_gf_mult(a3, 0x0e));
    }
}

/* ========================================================================
 * Expansão de chave (FIPS-197, seção 5.2)
 *
 * Gera 44 palavras w[0..43] de 4 bytes (11 chaves de rodada de 16 bytes):
 *   - w[0..3] = a própria chave;
 *   - para i >= 4: temp = w[i-1];
 *       se i for múltiplo de 4: temp = SubWord(RotWord(temp)) ^ Rcon[i/4];
 *       w[i] = w[i-4] ^ temp.
 * RotWord gira os 4 bytes uma posição para a esquerda; SubWord aplica a
 * S-box a cada byte; Rcon[j] = (x^(j-1), 0, 0, 0) em GF(2^8), obtido
 * aplicando xtime sucessivamente a partir de {01}.
 * ======================================================================== */

void aes128_expandir_chave(const uint8_t chave[AES128_CHAVE],
                           uint8_t chaves_rodada[AES128_CHAVES_RODADA_BYTES])
{
    uint8_t *w = chaves_rodada;   /* w[i] ocupa os bytes w[4*i .. 4*i+3] */
    uint8_t rcon = 0x01;
    uint8_t temp[4];
    int i, k;

    memcpy(w, chave, AES128_CHAVE);

    for (i = 4; i < 4 * (AES128_RODADAS + 1); i++) {
        memcpy(temp, &w[4 * (i - 1)], 4);

        if (i % 4 == 0) {
            /* RotWord: [a0 a1 a2 a3] -> [a1 a2 a3 a0] */
            uint8_t t = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = t;

            /* SubWord */
            for (k = 0; k < 4; k++)
                temp[k] = SBOX[temp[k]];

            /* Rcon: só o primeiro byte é diferente de zero */
            temp[0] ^= rcon;
            rcon = aes128_xtime(rcon);
        }

        for (k = 0; k < 4; k++)
            w[4 * i + k] = (uint8_t)(w[4 * (i - 4) + k] ^ temp[k]);
    }
}

/* ========================================================================
 * Cifragem de um bloco (FIPS-197, seção 5.1)
 *
 *   AddRoundKey(chave 0)
 *   rodadas 1..9: SubBytes, ShiftRows, MixColumns, AddRoundKey(chave r)
 *   rodada 10:    SubBytes, ShiftRows,             AddRoundKey(chave 10)
 *
 * A última rodada omite o MixColumns: isso torna a decifragem simétrica
 * à cifragem sem alterar a segurança.
 * ======================================================================== */

void aes128_cifrar_bloco(const uint8_t chaves_rodada[AES128_CHAVES_RODADA_BYTES],
                         const uint8_t entrada[AES128_BLOCO],
                         uint8_t saida[AES128_BLOCO])
{
    uint8_t estado[AES128_BLOCO];
    int rodada;

    memcpy(estado, entrada, AES128_BLOCO);
    aes128_add_round_key(estado, &chaves_rodada[0]);

    for (rodada = 1; rodada < AES128_RODADAS; rodada++) {
        aes128_sub_bytes(estado);
        aes128_shift_rows(estado);
        aes128_mix_columns(estado);
        aes128_add_round_key(estado, &chaves_rodada[AES128_BLOCO * rodada]);
    }

    aes128_sub_bytes(estado);
    aes128_shift_rows(estado);
    aes128_add_round_key(estado, &chaves_rodada[AES128_BLOCO * AES128_RODADAS]);

    memcpy(saida, estado, AES128_BLOCO);
}

/* ========================================================================
 * Decifragem de um bloco (FIPS-197, seção 5.3, "InvCipher")
 *
 * Percorre as rodadas de trás para frente, aplicando as inversas e as
 * mesmas chaves de rodada em ordem decrescente (10, 9, ..., 0):
 *
 *   AddRoundKey(chave 10)
 *   rodadas 9..1: InvShiftRows, InvSubBytes, AddRoundKey(chave r), InvMixColumns
 *   final:        InvShiftRows, InvSubBytes, AddRoundKey(chave 0)
 *
 * AddRoundKey não precisa de inversa: XOR com a mesma chave desfaz o XOR.
 * ======================================================================== */

void aes128_decifrar_bloco(const uint8_t chaves_rodada[AES128_CHAVES_RODADA_BYTES],
                           const uint8_t entrada[AES128_BLOCO],
                           uint8_t saida[AES128_BLOCO])
{
    uint8_t estado[AES128_BLOCO];
    int rodada;

    memcpy(estado, entrada, AES128_BLOCO);
    aes128_add_round_key(estado, &chaves_rodada[AES128_BLOCO * AES128_RODADAS]);

    for (rodada = AES128_RODADAS - 1; rodada >= 1; rodada--) {
        aes128_inv_shift_rows(estado);
        aes128_inv_sub_bytes(estado);
        aes128_add_round_key(estado, &chaves_rodada[AES128_BLOCO * rodada]);
        aes128_inv_mix_columns(estado);
    }

    aes128_inv_shift_rows(estado);
    aes128_inv_sub_bytes(estado);
    aes128_add_round_key(estado, &chaves_rodada[0]);

    memcpy(saida, estado, AES128_BLOCO);
}

/* ========================================================================
 * Modo ECB (Electronic Codebook)
 *
 * Cada bloco de 16 bytes é cifrado de forma independente, com a mesma chave.
 * Consequência: blocos de texto claro iguais geram blocos cifrados iguais,
 * o que revela padrões da mensagem e permite reordenar ou repetir blocos
 * sem que o receptor perceba.
 * ======================================================================== */

void aes128_ecb_cifrar(const uint8_t chaves_rodada[AES128_CHAVES_RODADA_BYTES],
                       const uint8_t *entrada, uint8_t *saida, size_t n_blocos)
{
    size_t i;
    for (i = 0; i < n_blocos; i++)
        aes128_cifrar_bloco(chaves_rodada, &entrada[AES128_BLOCO * i], &saida[AES128_BLOCO * i]);
}

void aes128_ecb_decifrar(const uint8_t chaves_rodada[AES128_CHAVES_RODADA_BYTES],
                         const uint8_t *entrada, uint8_t *saida, size_t n_blocos)
{
    size_t i;
    for (i = 0; i < n_blocos; i++)
        aes128_decifrar_bloco(chaves_rodada, &entrada[AES128_BLOCO * i], &saida[AES128_BLOCO * i]);
}
