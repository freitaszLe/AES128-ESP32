/*
 * receptor.ino — Placa B: recebe o quadro pela UART2, decifra e exibe a mensagem.
 *
 * Versão inicial: apenas confirma que a biblioteca AES128Core é encontrada.
 */
#include <AES128Core.h>

void setup() {
  Serial.begin(115200);  // Monitor Serial (USB): apenas diagnóstico
  delay(500);
  Serial.println();
  Serial.printf("[RX] Receptor - AES128Core %s\n", aes128core_versao());
}

void loop() {
}
