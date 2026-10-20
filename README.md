# AES-128 em Dois ESP32 via UART

Implementação do **AES-128 do zero** (conforme o FIPS-197, sem bibliotecas criptográficas) em C,
executada em dois ESP32-WROOM-32 que se comunicam pela UART2.

- A **chave** e a **mensagem** são formadas a partir das matrículas da dupla.
- A placa **transmissora** cifra a mensagem em modo **ECB** (com preenchimento **PKCS#7**) e envia
  um quadro binário pela UART2.
- A placa **receptora** recebe o quadro, decifra, remove o preenchimento e exibe o texto original
  no Monitor Serial.

Trabalho prático da disciplina de Segurança de Sistemas de Computação.

## Estrutura do repositório

```
libraries/AES128Core/   núcleo AES-128 em C puro, compartilhado pelas duas placas e pelos testes
firmware/transmissor/   placa A: monta a mensagem, cifra e envia o quadro pela UART2
firmware/receptor/      placa B: recebe o quadro, decifra e exibe a mensagem
firmware/validacao_fips/ executa no ESP32 os vetores de teste do FIPS-197
firmware/analise/       imprime chaves de rodada, estados, avalanche e o experimento ECB
tests/host/             testes do núcleo executados no computador
tools/referencia.sh     conferência dos resultados com OpenSSL (não faz parte do firmware)
docs/                   arquitetura, desenvolvimento e instruções de uso
```

## Início rápido

```bash
make test                                   # testes do núcleo no computador
make analise                                # relatório de análise no computador
make firmware                               # compila transmissor e receptor
make upload-tx PORTA=/dev/cu.usbserial-XXX  # grava o transmissor
make upload-rx PORTA=/dev/cu.usbserial-XXX  # grava o receptor
make upload-fips PORTA=/dev/cu.usbserial-XXX # grava a validação FIPS-197
make upload-analise PORTA=/dev/cu.usbserial-XXX # grava o relatório de análise
```

Instalação das ferramentas, uso na Arduino IDE e montagem das placas: [docs/uso.md](docs/uso.md).

## Documentação

| Documento | Conteúdo |
|---|---|
| [docs/arquitetura.md](docs/arquitetura.md) | Organização do código, módulos do AES, protocolo do quadro UART |
| [docs/desenvolvimento.md](docs/desenvolvimento.md) | Requisitos, vetores de teste, estratégia de validação e resultados |
| [docs/uso.md](docs/uso.md) | Instalação, compilação, gravação, montagem e execução |

## Referências

- NIST. *FIPS PUB 197 — Advanced Encryption Standard (AES)*, 2001.
- STALLINGS, W.; BROWN, L. *Computer Security: Principles and Practice*. 5. ed.
