/*
 * transmissor.ino — Placa A: cifra a mensagem da dupla e envia o quadro pela UART2.
 *
 * Versão inicial: apenas confirma que a biblioteca AES128Core é encontrada.
 */
#include <AES128Core.h>

void setup() {
  Serial.begin(115200);  // Monitor Serial (USB): apenas diagnóstico
  delay(500);
  Serial.println();
  Serial.printf("[TX] Transmissor - AES128Core %s\n", aes128core_versao());
}

void loop() {
}
