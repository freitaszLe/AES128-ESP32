/*
 * transmissor.ino — Placa A: cifra a mensagem com AES-128 (ECB + PKCS#7) e
 * envia o quadro [0xAA][n][16 x n bytes] em bytes brutos pela UART2.
 *
 * Ligações: GPIO17 (TX2) -> RX2 da placa B, GPIO16 (RX2) <- TX2 da placa B, GND comum.
 * O Monitor Serial (USB, 115200 baud) mostra apenas diagnóstico; os dados
 * cifrados trafegam somente pela UART2.
 *
 * Comandos pelo Monitor Serial (terminar com Enter):
 *   (vazio)     reenvia a mensagem atual
 *   m <texto>   cifra e envia uma mensagem nova
 *   d           volta para a mensagem da dupla e envia
 *   b <n>       inverte o bit n (0 a 127) da chave e envia
 *   k           restaura a chave original e envia
 *   t           envia só a metade do quadro (simula falha; o receptor deve descartar)
 *   ?           mostra os comandos
 */
#include <AES128Core.h>

static const int PINO_RX2 = 16;
static const int PINO_TX2 = 17;
static const uint32_t BAUD_UART2 = 115200;   // 8N1

static uint8_t chaveOriginal[AES128_CHAVE];
static uint8_t chave[AES128_CHAVE];
static char mensagem[QUADRO_MAX_MENSAGEM + 1];
static size_t tamMensagem = 0;
static uint32_t quadrosEnviados = 0;

// ---- Impressão ---------------------------------------------------------------

static void imprimirHex(const char *rotulo, const uint8_t *dados, size_t n) {
  Serial.printf("  %-14s: ", rotulo);
  for (size_t i = 0; i < n; i++) {
    if (i > 0 && i % AES128_BLOCO == 0) Serial.printf("\n  %-14s  ", "");
    Serial.printf("%02x", dados[i]);
  }
  Serial.println();
}

static void imprimirChave() {
  Serial.print("  Chave (ASCII) : ");
  for (int i = 0; i < AES128_CHAVE; i++)
    Serial.print(isprint(chave[i]) ? (char)chave[i] : '.');
  bool alterada = memcmp(chave, chaveOriginal, AES128_CHAVE) != 0;
  Serial.println(alterada ? "   (ALTERADA)" : "");
  imprimirHex("Chave (hex)", chave, AES128_CHAVE);
}

static void imprimirAjuda() {
  Serial.println("Comandos: [Enter] reenviar | m <texto> | d (dupla) | b <0-127> | k (chave original) | t (quadro truncado) | ?");
}

// ---- Envio -------------------------------------------------------------------

static void enviar() {
  uint8_t quadro[QUADRO_MAX_BYTES];
  uint8_t preenchida[QUADRO_MAX_DADOS];

  size_t tam = quadro_cifrar_mensagem(chave, (const uint8_t *)mensagem, tamMensagem,
                                      quadro, sizeof quadro);
  if (tam == 0) {
    Serial.println("[TX] ERRO: mensagem longa demais");
    return;
  }
  size_t tamPreenchida = pkcs7_aplicar((const uint8_t *)mensagem, tamMensagem,
                                       preenchida, sizeof preenchida);

  // Envio dos bytes brutos do quadro pela UART2
  size_t escritos = Serial2.write(quadro, tam);
  Serial2.flush();   // aguarda o último byte sair do pino TX2
  quadrosEnviados++;

  Serial.println();
  Serial.printf("[TX] Quadro #%lu enviado pela UART2: %u bytes (%u bloco(s))\n",
                (unsigned long)quadrosEnviados, (unsigned)escritos, (unsigned)quadro[1]);
  imprimirChave();
  Serial.printf("  Mensagem      : %s (%u bytes)\n", mensagem, (unsigned)tamMensagem);
  imprimirHex("Com PKCS#7", preenchida, tamPreenchida);
  imprimirHex("Cifrado (ECB)", &quadro[2], tam - 2);
  Serial.printf("  Quadro        : %02x %02x + %u bytes cifrados\n", quadro[0], quadro[1],
                (unsigned)(tam - 2));
}

// Envia apenas a metade inicial do quadro, para demonstrar o tempo limite do receptor.
static void enviarTruncado() {
  uint8_t quadro[QUADRO_MAX_BYTES];
  size_t tam = quadro_cifrar_mensagem(chave, (const uint8_t *)mensagem, tamMensagem,
                                      quadro, sizeof quadro);
  if (tam == 0) return;
  Serial2.write(quadro, tam / 2);
  Serial2.flush();
  Serial.printf("\n[TX] Quadro TRUNCADO enviado: %u de %u bytes (teste de timeout)\n",
                (unsigned)(tam / 2), (unsigned)tam);
}

static void usarMensagemDaDupla() {
  tamMensagem = dupla_montar_mensagem(DUPLA_MATRICULA_A, DUPLA_MATRICULA_B,
                                      mensagem, sizeof mensagem);
}

// ---- Comandos ----------------------------------------------------------------

static void executarComando(char *linha) {
  if (linha[0] == '\0') {
    enviar();
  } else if (linha[0] == 'm' && linha[1] == ' ') {
    strncpy(mensagem, linha + 2, QUADRO_MAX_MENSAGEM);
    mensagem[QUADRO_MAX_MENSAGEM] = '\0';
    tamMensagem = strlen(mensagem);
    enviar();
  } else if (strcmp(linha, "d") == 0) {
    usarMensagemDaDupla();
    enviar();
  } else if (linha[0] == 'b' && linha[1] == ' ') {
    int bit = atoi(linha + 2);
    if (bit < 0 || bit > 127) {
      Serial.println("[TX] bit deve estar entre 0 e 127");
      return;
    }
    analise_inverter_bit(chave, (size_t)bit);
    Serial.printf("[TX] bit %d da chave invertido\n", bit);
    enviar();
  } else if (strcmp(linha, "t") == 0) {
    enviarTruncado();
  } else if (strcmp(linha, "k") == 0) {
    memcpy(chave, chaveOriginal, AES128_CHAVE);
    Serial.println("[TX] chave original restaurada");
    enviar();
  } else {
    imprimirAjuda();
  }
}

// Lê uma linha do Monitor Serial sem bloquear; retorna true quando completa.
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

// ---- Programa ----------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD_UART2, SERIAL_8N1, PINO_RX2, PINO_TX2);
  delay(1000);

  dupla_montar_chave(DUPLA_MATRICULA_A, DUPLA_MATRICULA_B, chaveOriginal);
  memcpy(chave, chaveOriginal, AES128_CHAVE);
  usarMensagemDaDupla();

  Serial.println();
  Serial.printf("===== TRANSMISSOR AES-128 (AES128Core %s) - UART2 %lu bps 8N1 =====\n",
                aes128core_versao(), (unsigned long)BAUD_UART2);
  imprimirAjuda();
  enviar();
}

void loop() {
  static char linha[QUADRO_MAX_MENSAGEM + 4];
  if (lerLinha(linha, sizeof linha)) executarComando(linha);
}
