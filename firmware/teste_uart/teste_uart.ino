/*
 * teste_uart.ino — teste da UART2 sem criptografia.
 *
 * A mesma versão é gravada nas duas placas. Cada placa:
 *  - envia continuamente pela UART2 a sequência de bytes 0x00, 0x01, ..., 0xFF,
 *    0x00, ... (todos os valores possíveis, inclusive 0xAA);
 *  - lê a UART2 e confere se cada byte recebido é o sucessor do anterior;
 *  - a cada 2 segundos mostra no Monitor Serial quantos bytes enviou, quantos
 *    recebeu e quantos chegaram fora de sequência (erros).
 *
 * Ligações:
 *  - teste em uma placa (loopback): GPIO17 (TX2) ligado ao GPIO16 (RX2) da mesma placa;
 *  - teste entre placas: TX2 de uma no RX2 da outra, nos dois sentidos, e GND com GND.
 *
 * Enviar 'z' pelo Monitor Serial zera os contadores (útil depois de mexer nos fios).
 */
#include <Arduino.h>

static const int PINO_RX2 = 16;      // GPIO16: recebe
static const int PINO_TX2 = 17;      // GPIO17: transmite
static const int PINO_LED = 2;       // LED da placa, quando existir
static const uint32_t BAUD = 115200; // 8N1

static uint8_t proximoEnvio = 0;
static uint32_t enviados = 0, recebidos = 0, erros = 0;
static bool sincronizado = false;
static uint8_t esperado = 0;
static uint8_t ultimoRecebido = 0;

// MAC de fábrica (eFuse), usado para identificar qual placa está imprimindo.
static void imprimirMac() {
  uint64_t mac = ESP.getEfuseMac();
  for (int i = 0; i < 6; i++)
    Serial.printf(i ? ":%02x" : "%02x", (unsigned)((mac >> (8 * i)) & 0xFF));
}

static void zerarContadores() {
  enviados = recebidos = erros = 0;
  sincronizado = false;
}

void setup() {
  Serial.begin(115200);                                     // Monitor Serial (USB)
  Serial2.begin(BAUD, SERIAL_8N1, PINO_RX2, PINO_TX2);      // UART2
  pinMode(PINO_LED, OUTPUT);
  delay(500);
  Serial.println();
  Serial.print("[teste UART2] MAC ");
  imprimirMac();
  Serial.printf(" | RX2=GPIO%d TX2=GPIO%d | %lu bps 8N1\n", PINO_RX2, PINO_TX2,
                (unsigned long)BAUD);
}

void loop() {
  static uint32_t ultimoEnvio = 0, ultimoRelatorio = 0;
  uint32_t agora = millis();

  // Envia um lote de 64 bytes a cada 10 ms (cerca de 55% da capacidade do canal)
  if (agora - ultimoEnvio >= 10) {
    ultimoEnvio = agora;
    uint8_t lote[64];
    for (int i = 0; i < 64; i++) lote[i] = proximoEnvio++;
    enviados += Serial2.write(lote, sizeof lote);
  }

  // Confere tudo o que chegou
  while (Serial2.available() > 0) {
    uint8_t b = (uint8_t)Serial2.read();
    recebidos++;
    if (sincronizado && b != esperado) erros++;
    sincronizado = true;
    esperado = (uint8_t)(b + 1);
    ultimoRecebido = b;
  }

  // Comandos pelo Monitor Serial
  while (Serial.available() > 0) {
    if (Serial.read() == 'z') {
      zerarContadores();
      Serial.println("[teste UART2] contadores zerados");
    }
  }

  // Relatório a cada 2 s; o LED pisca para mostrar que a placa está ativa
  if (agora - ultimoRelatorio >= 2000) {
    ultimoRelatorio = agora;
    digitalWrite(PINO_LED, !digitalRead(PINO_LED));
    if (recebidos == 0)
      Serial.printf("TX %lu bytes | RX: nada recebido no GPIO%d\n",
                    (unsigned long)enviados, PINO_RX2);
    else
      Serial.printf("TX %lu bytes | RX %lu bytes, %lu erro(s) | ultimo 0x%02x | %s\n",
                    (unsigned long)enviados, (unsigned long)recebidos, (unsigned long)erros,
                    ultimoRecebido, erros == 0 ? "OK" : "COM ERROS");
  }
}
