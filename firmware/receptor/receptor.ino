/*
 * receptor.ino — Placa B: recebe pela UART2 o quadro [0xAA][n][16 x n bytes],
 * decifra com AES-128 (ECB), valida e remove o PKCS#7 e exibe a mensagem
 * original no Monitor Serial.
 *
 * Ligações: GPIO16 (RX2) <- TX2 da placa A, GPIO17 (TX2) -> RX2 da placa A, GND comum.
 *
 * Recepção (máquina de estados em quadro.c):
 *   1. descarta bytes até encontrar o marcador 0xAA;
 *   2. lê n e rejeita n = 0 ou n > QUADRO_MAX_BLOCOS;
 *   3. lê exatamente 16 x n bytes; se não chegarem em TIMEOUT_MS, descarta o quadro.
 *
 * Comandos pelo Monitor Serial (terminar com Enter):
 *   b <n>   inverte o bit n (0 a 127) da chave do receptor
 *   k       restaura a chave original
 *   ?       mostra os comandos
 */
#include <AES128Core.h>

static const int PINO_RX2 = 16;
static const int PINO_TX2 = 17;
static const uint32_t BAUD_UART2 = 115200;   // 8N1
static const uint32_t TIMEOUT_MS = 500;      // o quadro máximo (258 bytes) leva ~23 ms

static uint8_t chaveOriginal[AES128_CHAVE];
static uint8_t chave[AES128_CHAVE];
static quadro_receptor_t receptor;
static uint32_t quadrosOk = 0, quadrosRejeitados = 0;

// ---- Impressão ---------------------------------------------------------------

static void imprimirHex(const char *rotulo, const uint8_t *dados, size_t n) {
  Serial.printf("  %-14s: ", rotulo);
  for (size_t i = 0; i < n; i++) {
    if (i > 0 && i % AES128_BLOCO == 0) Serial.printf("\n  %-14s  ", "");
    Serial.printf("%02x", dados[i]);
  }
  Serial.println();
}

static void imprimirAjuda() {
  Serial.println("Comandos: b <0-127> (inverte bit da chave) | k (chave original) | ?");
}

static void imprimirChave() {
  bool alterada = memcmp(chave, chaveOriginal, AES128_CHAVE) != 0;
  Serial.printf("  Chave (ASCII) : %.16s%s\n", (const char *)chave, alterada ? "   (ALTERADA)" : "");
}

// ---- Tratamento de um quadro completo ----------------------------------------

static void processarQuadro() {
  uint8_t mensagem[QUADRO_MAX_DADOS + 1];
  size_t n = 0;
  size_t tamDados = (size_t)receptor.n_blocos * AES128_BLOCO;

  Serial.println();
  Serial.printf("[RX] Quadro recebido pela UART2: AA %02x + %u bytes cifrados\n",
                receptor.n_blocos, (unsigned)tamDados);
  imprimirChave();
  imprimirHex("Cifrado", receptor.dados, tamDados);

  int ok = quadro_decifrar(chave, receptor.dados, receptor.n_blocos, mensagem, &n);
  imprimirHex("Decifrado", mensagem, tamDados);

  if (ok == 0) {
    mensagem[n] = '\0';
    quadrosOk++;
    Serial.printf("  PKCS#7        : valido (%u byte(s) de preenchimento removidos)\n",
                  (unsigned)(tamDados - n));
    Serial.printf("  MENSAGEM      : %s\n", (const char *)mensagem);
  } else {
    quadrosRejeitados++;
    Serial.println("  PKCS#7        : INVALIDO -> quadro rejeitado (chave diferente ou dados corrompidos)");
  }
  Serial.printf("  Quadros aceitos: %lu | rejeitados: %lu | bytes descartados fora de quadro: %lu\n",
                (unsigned long)quadrosOk, (unsigned long)quadrosRejeitados,
                (unsigned long)receptor.descartados);
}

static void tratarResultado(quadro_resultado_t res) {
  switch (res) {
    case QUADRO_COMPLETO:
      processarQuadro();
      break;
    case QUADRO_ERRO_N_INVALIDO:
      quadrosRejeitados++;
      Serial.printf("[RX] Quadro rejeitado: numero de blocos invalido (1 a %d)\n", QUADRO_MAX_BLOCOS);
      break;
    case QUADRO_ERRO_TIMEOUT:
      quadrosRejeitados++;
      Serial.printf("[RX] Quadro descartado: incompleto apos %lu ms (recebidos %u de %u bytes)\n",
                    (unsigned long)TIMEOUT_MS, (unsigned)receptor.recebidos,
                    (unsigned)(receptor.n_blocos * AES128_BLOCO));
      break;
    default:
      break;
  }
}

// ---- Comandos ----------------------------------------------------------------

static bool lerLinha(char *buf, size_t cap) {
  static size_t pos = 0;
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      buf[pos] = '\0';
      pos = 0;
      return true;
    }
    if (pos < cap - 1) buf[pos++] = c;
  }
  return false;
}

static void executarComando(const char *linha) {
  if (linha[0] == 'b' && linha[1] == ' ') {
    int bit = atoi(linha + 2);
    if (bit < 0 || bit > 127) {
      Serial.println("[RX] bit deve estar entre 0 e 127");
      return;
    }
    analise_inverter_bit(chave, (size_t)bit);
    Serial.printf("[RX] bit %d da chave invertido\n", bit);
    imprimirChave();
  } else if (strcmp(linha, "k") == 0) {
    memcpy(chave, chaveOriginal, AES128_CHAVE);
    Serial.println("[RX] chave original restaurada");
    imprimirChave();
  } else {
    imprimirAjuda();
  }
}

// ---- Programa ----------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD_UART2, SERIAL_8N1, PINO_RX2, PINO_TX2);
  delay(500);

  dupla_montar_chave(DUPLA_MATRICULA_A, DUPLA_MATRICULA_B, chaveOriginal);
  memcpy(chave, chaveOriginal, AES128_CHAVE);
  quadro_receptor_iniciar(&receptor, TIMEOUT_MS);

  Serial.println();
  Serial.printf("===== RECEPTOR AES-128 (AES128Core %s) - UART2 %lu bps 8N1, timeout %lu ms =====\n",
                aes128core_versao(), (unsigned long)BAUD_UART2, (unsigned long)TIMEOUT_MS);
  imprimirAjuda();
  Serial.println("[RX] Aguardando quadro (marcador 0xAA)...");
}

void loop() {
  static char linha[32];

  // Bytes brutos da UART2 entram na máquina de estados um a um
  while (Serial2.available() > 0)
    tratarResultado(quadro_receptor_byte(&receptor, (uint8_t)Serial2.read(), millis()));

  // Quadro incompleto há mais de TIMEOUT_MS é descartado
  tratarResultado(quadro_receptor_verificar_timeout(&receptor, millis()));

  if (lerLinha(linha, sizeof linha)) executarComando(linha);
}
