/*
 * analise.ino — imprime no Monitor Serial (115200 baud) os registros da
 * análise com os dados da dupla: chave, mensagem com PKCS#7, texto cifrado,
 * chaves das rodadas 0 e 1, cinco estados do bloco 1 na rodada 1, efeito
 * avalanche e o experimento de blocos repetidos em ECB.
 *
 * O relatório é impresso ao ligar a placa e novamente sempre que algum
 * caractere for enviado pelo Monitor Serial (por exemplo, Enter).
 */
#include <AES128Core.h>

static const char *MATRICULA_A = "2022178440126";
static const char *MATRICULA_B = "2022178440142";

// Função de saída usada pelo relatório: uma linha por chamada.
static void escreverLinha(const char *linha) {
  Serial.println(linha);
}

static void imprimirRelatorio() {
  Serial.println();
  analise_relatorio(MATRICULA_A, MATRICULA_B, escreverLinha);
  Serial.println("(envie qualquer caractere para imprimir novamente)");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  imprimirRelatorio();
}

void loop() {
  if (Serial.available() > 0) {
    while (Serial.available() > 0) Serial.read();  // descarta o que foi digitado
    imprimirRelatorio();
  }
}
