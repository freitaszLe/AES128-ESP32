/*
 * validacao_fips.ino — executa no ESP32 os vetores de teste do FIPS-197
 * (apêndices B e C.1) com o núcleo AES128Core e mede o tempo de cifragem.
 *
 * A validação é repetida a cada 10 segundos, para que o resultado possa ser
 * visto a qualquer momento no Monitor Serial (115200 baud).
 */
#include <AES128Core.h>

struct VetorFips {
  const char *nome;
  const char *chave;
  const char *claro;
  const char *cifrado;
};

static const VetorFips VETORES[] = {
  { "Apendice B",   "2b7e151628aed2a6abf7158809cf4f3c",
                    "3243f6a8885a308d313198a2e0370734",
                    "3925841d02dc09fbdc118597196a0b32" },
  { "Apendice C.1", "000102030405060708090a0b0c0d0e0f",
                    "00112233445566778899aabbccddeeff",
                    "69c4e0d86a7b0430d8cdb78070b4c55a" },
};

// Converte 32 dígitos hexadecimais em 16 bytes.
static void hexParaBytes(const char *hex, uint8_t *saida) {
  for (int i = 0; i < AES128_BLOCO; i++) {
    char par[3] = { hex[2 * i], hex[2 * i + 1], '\0' };
    saida[i] = (uint8_t)strtoul(par, nullptr, 16);
  }
}

static void imprimirHex(const char *rotulo, const uint8_t *dados) {
  Serial.print(rotulo);
  for (int i = 0; i < AES128_BLOCO; i++) Serial.printf("%02x", dados[i]);
  Serial.println();
}

// Executa um vetor (cifrar e decifrar). Retorna o número de falhas (0, 1 ou 2).
static int validarVetor(const VetorFips &v) {
  uint8_t chave[AES128_CHAVE], rk[AES128_CHAVES_RODADA_BYTES];
  uint8_t claro[AES128_BLOCO], esperado[AES128_BLOCO];
  uint8_t cifrado[AES128_BLOCO], decifrado[AES128_BLOCO];
  int falhas = 0;

  hexParaBytes(v.chave, chave);
  hexParaBytes(v.claro, claro);
  hexParaBytes(v.cifrado, esperado);

  aes128_expandir_chave(chave, rk);
  aes128_cifrar_bloco(rk, claro, cifrado);
  aes128_decifrar_bloco(rk, cifrado, decifrado);

  bool cifraOk = memcmp(cifrado, esperado, AES128_BLOCO) == 0;
  bool decifraOk = memcmp(decifrado, claro, AES128_BLOCO) == 0;

  Serial.printf("--- %s\n", v.nome);
  imprimirHex("  chave     : ", chave);
  imprimirHex("  claro     : ", claro);
  imprimirHex("  cifrado   : ", cifrado);
  imprimirHex("  esperado  : ", esperado);
  Serial.printf("  cifrar    : %s\n", cifraOk ? "OK" : "FALHOU");
  imprimirHex("  decifrado : ", decifrado);
  Serial.printf("  decifrar  : %s\n", decifraOk ? "OK" : "FALHOU");

  if (!cifraOk) falhas++;
  if (!decifraOk) falhas++;
  return falhas;
}

// Mede o tempo médio de expansão de chave, cifragem e decifragem de um bloco.
static void medirDesempenho() {
  const int N = 1000;
  uint8_t chave[AES128_CHAVE] = { 0 }, rk[AES128_CHAVES_RODADA_BYTES];
  uint8_t bloco[AES128_BLOCO] = { 0 };

  uint32_t t0 = micros();
  for (int i = 0; i < N; i++) aes128_expandir_chave(chave, rk);
  uint32_t t1 = micros();
  for (int i = 0; i < N; i++) aes128_cifrar_bloco(rk, bloco, bloco);
  uint32_t t2 = micros();
  for (int i = 0; i < N; i++) aes128_decifrar_bloco(rk, bloco, bloco);
  uint32_t t3 = micros();

  Serial.printf("--- Desempenho (media de %d execucoes, CPU a %lu MHz)\n", N,
                (unsigned long)getCpuFrequencyMhz());
  Serial.printf("  expansao de chave : %.2f us\n", (t1 - t0) / (float)N);
  Serial.printf("  cifrar 1 bloco    : %.2f us\n", (t2 - t1) / (float)N);
  Serial.printf("  decifrar 1 bloco  : %.2f us\n", (t3 - t2) / (float)N);
}

static void executarValidacao() {
  int falhas = 0;
  Serial.println();
  Serial.printf("===== Validacao FIPS-197 no ESP32 - AES128Core %s =====\n", aes128core_versao());
  for (const VetorFips &v : VETORES) falhas += validarVetor(v);
  medirDesempenho();
  Serial.printf("===== RESULTADO: %s (%d falha(s)) =====\n", falhas == 0 ? "OK" : "FALHOU", falhas);
}

void setup() {
  Serial.begin(115200);
  delay(500);
}

void loop() {
  executarValidacao();
  delay(10000);
}
