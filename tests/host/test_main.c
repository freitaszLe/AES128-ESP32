/*
 * test_main.c — testes do núcleo AES128Core executados no computador.
 *
 * Os valores esperados vêm do FIPS-197:
 *  - seção 4.2 / 4.2.1: multiplicação em GF(2^8) e xtime;
 *  - figura 7: S-box (recalculada a partir da definição matemática);
 *  - apêndice A.1: expansão da chave 2b7e1516...;
 *  - apêndice B: estados da rodada 1 e resultado da cifragem;
 *  - apêndice C.1: cifragem e decifragem ("INVERSE CIPHER") com a chave 000102...0f;
 *  - figura 14: S-box inversa.
 */
#include "teste.h"
#include "aes128.h"

/* ---------------------------------------------------------------------- */

static void teste_biblioteca_ligada(void)
{
    printf("[biblioteca] versão %s\n", aes128core_versao());
    CHECAR(aes128core_versao() != NULL);
}

/* ---------------------------------------------------------------------- */

static void teste_gf(void)
{
    printf("[GF(2^8)] xtime e multiplicação\n");

    /* FIPS-197 4.2.1: {57}·{02} = {ae}, ·{04} = {47}, ·{08} = {8e}, ·{10} = {07} */
    CHECAR(aes128_xtime(0x57) == 0xae);
    CHECAR(aes128_xtime(0xae) == 0x47);
    CHECAR(aes128_xtime(0x47) == 0x8e);
    CHECAR(aes128_xtime(0x8e) == 0x07);

    /* FIPS-197 4.2: {57}·{83} = {c1};  4.2.1: {57}·{13} = {fe} */
    CHECAR(aes128_gf_mult(0x57, 0x83) == 0xc1);
    CHECAR(aes128_gf_mult(0x57, 0x13) == 0xfe);

    /* Propriedades: elemento neutro e comutatividade */
    CHECAR(aes128_gf_mult(0x57, 0x01) == 0x57);
    CHECAR(aes128_gf_mult(0x83, 0x57) == 0xc1);
}

/* ---------------------------------------------------------------------- */

/* Inverso multiplicativo em GF(2^8) por busca exaustiva (0 -> 0). */
static uint8_t gf_inverso(uint8_t a)
{
    unsigned int b;
    if (a == 0)
        return 0;
    for (b = 1; b < 256; b++)
        if (aes128_gf_mult(a, (uint8_t)b) == 1)
            return (uint8_t)b;
    return 0; /* não ocorre: todo elemento não nulo tem inverso */
}

static uint8_t rotl8(uint8_t x, int n)
{
    return (uint8_t)((x << n) | (x >> (8 - n)));
}

static void teste_sbox(void)
{
    unsigned int b;
    int divergencias = 0;

    printf("[S-box] tabela == inverso em GF(2^8) + transformação afim\n");

    for (b = 0; b < 256; b++) {
        uint8_t inv = gf_inverso((uint8_t)b);
        uint8_t esperado = (uint8_t)(inv ^ rotl8(inv, 1) ^ rotl8(inv, 2) ^
                                     rotl8(inv, 3) ^ rotl8(inv, 4) ^ 0x63);
        if (aes128_sbox((uint8_t)b) != esperado) {
            printf("  S-box[%02x] = %02x, calculado %02x\n", b, aes128_sbox((uint8_t)b), esperado);
            divergencias++;
        }
    }
    CHECAR(divergencias == 0);

    /* Exemplo do FIPS-197 5.1.1: S-box({53}) = {ed} */
    CHECAR(aes128_sbox(0x53) == 0xed);
}

/* ---------------------------------------------------------------------- */

static void teste_expansao_chave(void)
{
    /* FIPS-197 apêndice A.1 / apêndice B: chaves das rodadas 0 a 10 */
    static const char *esperadas[AES128_RODADAS + 1] = {
        "2b7e151628aed2a6abf7158809cf4f3c",
        "a0fafe1788542cb123a339392a6c7605",
        "f2c295f27a96b9435935807a7359f67f",
        "3d80477d4716fe3e1e237e446d7a883b",
        "ef44a541a8525b7fb671253bdb0bad00",
        "d4d1c6f87c839d87caf2b8bc11f915bc",
        "6d88a37a110b3efddbf98641ca0093fd",
        "4e54f70e5f5fc9f384a64fb24ea6dc4f",
        "ead27321b58dbad2312bf5607f8d292f",
        "ac7766f319fadc2128d12941575c006e",
        "d014f9a8c9ee2589e13f0cc8b6630ca6",
    };
    uint8_t chave[AES128_CHAVE], rk[AES128_CHAVES_RODADA_BYTES], esp[AES128_BLOCO];
    char nome[32];
    int r;

    printf("[expansão de chave] FIPS-197 apêndice A.1 (11 chaves de rodada)\n");

    hex_para_bytes("2b7e151628aed2a6abf7158809cf4f3c", chave);
    aes128_expandir_chave(chave, rk);

    for (r = 0; r <= AES128_RODADAS; r++) {
        hex_para_bytes(esperadas[r], esp);
        snprintf(nome, sizeof nome, "chave da rodada %d", r);
        CHECAR_BYTES(&rk[AES128_BLOCO * r], esp, AES128_BLOCO, nome);
    }
}

/* ---------------------------------------------------------------------- */

/* Aplica uma transformação a 'antes' e compara com 'depois' (hex). */
static void checar_transformacao(void (*f)(uint8_t *), const char *antes,
                                 const char *depois, const char *nome)
{
    uint8_t s[AES128_BLOCO], esp[AES128_BLOCO];
    hex_para_bytes(antes, s);
    hex_para_bytes(depois, esp);
    f(s);
    CHECAR_BYTES(s, esp, AES128_BLOCO, nome);
}

static void teste_transformacoes(void)
{
    /* FIPS-197 apêndice B, rodada 1: cada transformação aplicada isoladamente */
    uint8_t s[AES128_BLOCO], k[AES128_BLOCO], esp[AES128_BLOCO];

    printf("[transformações] FIPS-197 apêndice B, rodada 1\n");

    checar_transformacao(aes128_sub_bytes,
                         "193de3bea0f4e22b9ac68d2ae9f84808",
                         "d42711aee0bf98f1b8b45de51e415230", "SubBytes");
    checar_transformacao(aes128_shift_rows,
                         "d42711aee0bf98f1b8b45de51e415230",
                         "d4bf5d30e0b452aeb84111f11e2798e5", "ShiftRows");
    checar_transformacao(aes128_mix_columns,
                         "d4bf5d30e0b452aeb84111f11e2798e5",
                         "046681e5e0cb199a48f8d37a2806264c", "MixColumns");

    hex_para_bytes("046681e5e0cb199a48f8d37a2806264c", s);
    hex_para_bytes("a0fafe1788542cb123a339392a6c7605", k);
    hex_para_bytes("a49c7ff2689f352b6b5bea43026a5049", esp);
    aes128_add_round_key(s, k);
    CHECAR_BYTES(s, esp, AES128_BLOCO, "AddRoundKey (chave 1)");
}

/* ---------------------------------------------------------------------- */

/* Reproduz a rodada 1 a partir do texto claro, como será registrado na análise. */
static void teste_rodada1_apendice_b(void)
{
    static const char *estados[5] = {
        "193de3bea0f4e22b9ac68d2ae9f84808", /* (1) AddRoundKey inicial (chave 0) */
        "d42711aee0bf98f1b8b45de51e415230", /* (2) SubBytes                      */
        "d4bf5d30e0b452aeb84111f11e2798e5", /* (3) ShiftRows                     */
        "046681e5e0cb199a48f8d37a2806264c", /* (4) MixColumns                    */
        "a49c7ff2689f352b6b5bea43026a5049", /* (5) AddRoundKey (chave 1)         */
    };
    static const char *nomes[5] = {
        "(1) AddRoundKey chave 0", "(2) SubBytes", "(3) ShiftRows",
        "(4) MixColumns", "(5) AddRoundKey chave 1",
    };
    uint8_t chave[AES128_CHAVE], rk[AES128_CHAVES_RODADA_BYTES];
    uint8_t s[AES128_BLOCO], esp[AES128_BLOCO];
    int i;

    printf("[rodada 1] cinco estados a partir do texto claro (apêndice B)\n");

    hex_para_bytes("2b7e151628aed2a6abf7158809cf4f3c", chave);
    hex_para_bytes("3243f6a8885a308d313198a2e0370734", s);
    aes128_expandir_chave(chave, rk);

    for (i = 0; i < 5; i++) {
        switch (i) {
        case 0: aes128_add_round_key(s, &rk[0]);            break;
        case 1: aes128_sub_bytes(s);                         break;
        case 2: aes128_shift_rows(s);                        break;
        case 3: aes128_mix_columns(s);                       break;
        case 4: aes128_add_round_key(s, &rk[AES128_BLOCO]);  break;
        }
        hex_para_bytes(estados[i], esp);
        CHECAR_BYTES(s, esp, AES128_BLOCO, nomes[i]);
    }
}

/* ---------------------------------------------------------------------- */

static void checar_cifragem(const char *chave_hex, const char *claro_hex,
                            const char *cifrado_hex, const char *nome)
{
    uint8_t chave[AES128_CHAVE], rk[AES128_CHAVES_RODADA_BYTES];
    uint8_t claro[AES128_BLOCO], esp[AES128_BLOCO], saida[AES128_BLOCO];

    hex_para_bytes(chave_hex, chave);
    hex_para_bytes(claro_hex, claro);
    hex_para_bytes(cifrado_hex, esp);

    aes128_expandir_chave(chave, rk);
    aes128_cifrar_bloco(rk, claro, saida);
    CHECAR_BYTES(saida, esp, AES128_BLOCO, nome);

    /* Cifragem no lugar (entrada == saída) deve dar o mesmo resultado */
    aes128_cifrar_bloco(rk, claro, claro);
    CHECAR_BYTES(claro, esp, AES128_BLOCO, "cifragem no lugar");
}

static void teste_cifragem_fips(void)
{
    printf("[cifragem] FIPS-197 apêndices B e C.1\n");
    checar_cifragem("2b7e151628aed2a6abf7158809cf4f3c",
                    "3243f6a8885a308d313198a2e0370734",
                    "3925841d02dc09fbdc118597196a0b32", "apêndice B");
    checar_cifragem("000102030405060708090a0b0c0d0e0f",
                    "00112233445566778899aabbccddeeff",
                    "69c4e0d86a7b0430d8cdb78070b4c55a", "apêndice C.1");
}

/* ---------------------------------------------------------------------- */

static void teste_inv_sbox(void)
{
    unsigned int b;
    int divergencias = 0;

    printf("[S-box inversa] InvSbox(Sbox(b)) == b para os 256 bytes\n");

    for (b = 0; b < 256; b++)
        if (aes128_inv_sbox(aes128_sbox((uint8_t)b)) != b ||
            aes128_sbox(aes128_inv_sbox((uint8_t)b)) != b)
            divergencias++;
    CHECAR(divergencias == 0);

    /* FIPS-197 figura 14: InvSbox({00}) = {52}, InvSbox({63}) = {00} */
    CHECAR(aes128_inv_sbox(0x00) == 0x52);
    CHECAR(aes128_inv_sbox(0x63) == 0x00);
}

/* ---------------------------------------------------------------------- */

static void teste_transformacoes_inversas(void)
{
    printf("[transformações inversas] desfazem as diretas e conferem com o apêndice C.1\n");

    /* Cada inversa desfaz a transformação direta da rodada 1 do apêndice B */
    checar_transformacao(aes128_inv_sub_bytes,
                         "d42711aee0bf98f1b8b45de51e415230",
                         "193de3bea0f4e22b9ac68d2ae9f84808", "InvSubBytes");
    checar_transformacao(aes128_inv_shift_rows,
                         "d4bf5d30e0b452aeb84111f11e2798e5",
                         "d42711aee0bf98f1b8b45de51e415230", "InvShiftRows");
    checar_transformacao(aes128_inv_mix_columns,
                         "046681e5e0cb199a48f8d37a2806264c",
                         "d4bf5d30e0b452aeb84111f11e2798e5", "InvMixColumns");

    /* Apêndice C.1, INVERSE CIPHER, rodada 1: istart -> is_row -> is_box */
    checar_transformacao(aes128_inv_shift_rows,
                         "7ad5fda789ef4e272bca100b3d9ff59f",
                         "7a9f102789d5f50b2beffd9f3dca4ea7", "C.1 InvShiftRows");
    checar_transformacao(aes128_inv_sub_bytes,
                         "7a9f102789d5f50b2beffd9f3dca4ea7",
                         "bd6e7c3df2b5779e0b61216e8b10b689", "C.1 InvSubBytes");
}

/* ---------------------------------------------------------------------- */

static void checar_decifragem(const char *chave_hex, const char *cifrado_hex,
                              const char *claro_hex, const char *nome)
{
    uint8_t chave[AES128_CHAVE], rk[AES128_CHAVES_RODADA_BYTES];
    uint8_t cifrado[AES128_BLOCO], esp[AES128_BLOCO], saida[AES128_BLOCO];

    hex_para_bytes(chave_hex, chave);
    hex_para_bytes(cifrado_hex, cifrado);
    hex_para_bytes(claro_hex, esp);

    aes128_expandir_chave(chave, rk);
    aes128_decifrar_bloco(rk, cifrado, saida);
    CHECAR_BYTES(saida, esp, AES128_BLOCO, nome);

    /* Decifragem no lugar (entrada == saída) deve dar o mesmo resultado */
    aes128_decifrar_bloco(rk, cifrado, cifrado);
    CHECAR_BYTES(cifrado, esp, AES128_BLOCO, "decifragem no lugar");
}

static void teste_decifragem_fips(void)
{
    printf("[decifragem] FIPS-197 apêndices B e C.1\n");
    checar_decifragem("2b7e151628aed2a6abf7158809cf4f3c",
                      "3925841d02dc09fbdc118597196a0b32",
                      "3243f6a8885a308d313198a2e0370734", "apêndice B (inverso)");
    checar_decifragem("000102030405060708090a0b0c0d0e0f",
                      "69c4e0d86a7b0430d8cdb78070b4c55a",
                      "00112233445566778899aabbccddeeff", "apêndice C.1 (inverso)");
}

/* ---------------------------------------------------------------------- */

/* Gerador pseudoaleatório simples (xorshift32), apenas para gerar dados de
 * teste reprodutíveis. Não tem uso criptográfico. */
static uint32_t semente = 0x2545F491u;

static uint8_t byte_teste(void)
{
    semente ^= semente << 13;
    semente ^= semente >> 17;
    semente ^= semente << 5;
    return (uint8_t)semente;
}

static void teste_ida_e_volta(void)
{
    enum { CASOS = 1000 };
    uint8_t chave[AES128_CHAVE], rk[AES128_CHAVES_RODADA_BYTES];
    uint8_t claro[AES128_BLOCO], cifrado[AES128_BLOCO], volta[AES128_BLOCO];
    uint8_t s[AES128_BLOCO], orig[AES128_BLOCO];
    int caso, i, erros = 0, inversas_erradas = 0, iguais = 0;

    printf("[ida e volta] Decifrar(Cifrar(x)) == x em %d chaves/blocos pseudoaleatórios\n", CASOS);

    for (caso = 0; caso < CASOS; caso++) {
        for (i = 0; i < AES128_BLOCO; i++) {
            chave[i] = byte_teste();
            claro[i] = byte_teste();
        }
        aes128_expandir_chave(chave, rk);
        aes128_cifrar_bloco(rk, claro, cifrado);
        aes128_decifrar_bloco(rk, cifrado, volta);
        if (memcmp(volta, claro, AES128_BLOCO) != 0)
            erros++;
        if (memcmp(cifrado, claro, AES128_BLOCO) == 0)
            iguais++;

        /* Cada inversa desfaz sua transformação para um estado qualquer */
        memcpy(orig, claro, AES128_BLOCO);
        memcpy(s, claro, AES128_BLOCO);
        aes128_sub_bytes(s);    aes128_inv_sub_bytes(s);
        aes128_shift_rows(s);   aes128_inv_shift_rows(s);
        aes128_mix_columns(s);  aes128_inv_mix_columns(s);
        if (memcmp(s, orig, AES128_BLOCO) != 0)
            inversas_erradas++;
    }
    CHECAR(erros == 0);
    CHECAR(inversas_erradas == 0);
    CHECAR(iguais == 0);  /* o texto cifrado nunca deve sair igual ao claro */
}

/* ---------------------------------------------------------------------- */

int main(void)
{
    teste_biblioteca_ligada();
    teste_gf();
    teste_sbox();
    teste_expansao_chave();
    teste_transformacoes();
    teste_rodada1_apendice_b();
    teste_cifragem_fips();
    teste_inv_sbox();
    teste_transformacoes_inversas();
    teste_decifragem_fips();
    teste_ida_e_volta();
    return teste_resumo();
}
