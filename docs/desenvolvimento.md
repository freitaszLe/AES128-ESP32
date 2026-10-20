# Desenvolvimento

## Requisitos

### Chave (16 bytes)

1. Usar somente os dígitos das matrículas.
2. Tomar os **8 últimos dígitos** da matrícula A, completando com `0` à esquerda se houver menos de 8.
3. Idem para a matrícula B.
4. Converter cada caractere para ASCII (`'2'` = `0x32`) e concatenar **A seguido de B**.

### Mensagem

- Texto `A:<matrícula A>;B:<matrícula B>` com as matrículas completas.
- Preenchimento **PKCS#7**: acrescentar de 1 a 16 bytes, todos com valor igual à quantidade
  acrescentada, até completar um múltiplo de 16. Se o tamanho já for múltiplo de 16, acrescenta-se
  um bloco inteiro de `0x10`.
- Modo **ECB**: cada bloco de 16 bytes é cifrado de forma independente.

### Comunicação

- UART2, 115200 bps, 8N1, ligação TX→RX, RX→TX e GND.
- Transmissão do quadro em bytes brutos (ver [arquitetura.md](arquitetura.md)).
- O receptor sincroniza em `0xAA`, lê `n` e exatamente `16 × n` bytes com tempo limite, decifra,
  remove o PKCS#7 e exibe a mensagem.

### Análises

- Registros em hexadecimal: chave, mensagem com PKCS#7, texto cifrado, chaves das rodadas 0 e 1.
- Cinco estados do bloco 1 na rodada 1: AddRoundKey inicial, SubBytes, ShiftRows, MixColumns,
  AddRoundKey (chave 1).
- Efeito avalanche: alterar 1 bit da chave ou da mensagem e contar os bits alterados no texto cifrado.
- Experimento de blocos repetidos em ECB.

## Vetores de teste

### FIPS-197

| | Apêndice B | Apêndice C.1 |
|---|---|---|
| Chave | `2b7e151628aed2a6abf7158809cf4f3c` | `000102030405060708090a0b0c0d0e0f` |
| Texto claro | `3243f6a8885a308d313198a2e0370734` | `00112233445566778899aabbccddeeff` |
| Cifrado | `3925841d02dc09fbdc118597196a0b32` | `69c4e0d86a7b0430d8cdb78070b4c55a` |

### Exemplo de matrículas (fictícias)

| Item | Valor |
|---|---|
| Matrículas | A = `20231001`, B = `20231002` |
| Chave (hex) | `32303233313030313230323331303032` |
| Mensagem | `A:20231001;B:20231002` (21 bytes) |
| Bloco 1 com PKCS#7 | `413a32303233313030313b423a323032` |
| Bloco 2 com PKCS#7 | `33313030320b0b0b0b0b0b0b0b0b0b0b` |
| Cifrado, bloco 1 | `3a3784991ab0acaa88d5fd94cb7b74ad` |
| Cifrado, bloco 2 | `cbea1d9e3e1f39da47111cf69064b166` |

### Blocos repetidos em ECB (chave do exemplo)

| Bloco | Texto claro (16 bytes) | Cifrado |
|---|---|---|
| 1 | `"LIGAR LED VERDE."` | `e0c22a6f02664f487dc9528642203dec` |
| 2 | `"DESLIGAR LED    "` (4 espaços no final) | `2b4d065ea65478b60696616244cd5070` |
| 3 | `"LIGAR LED VERDE."` | `e0c22a6f02664f487dc9528642203dec` |

## Estratégia de validação

1. **Testes no computador** (`make test`): cada transformação do AES é testada isoladamente contra
   os valores intermediários do FIPS-197; depois, a cifra completa contra os Apêndices B e C.1.
2. **Conferência externa** (`tools/referencia.sh`): os mesmos dados são cifrados com OpenSSL, usado
   somente como gabarito e nunca no firmware.
3. **Execução no ESP32**: os mesmos vetores são verificados na placa.
4. **Teste integrado**: transmissão entre as duas placas com os dados da dupla.

## Ambiente utilizado

| Item | Versão |
|---|---|
| Computador | MacBook Air Apple M5, macOS 26.6 |
| Arduino IDE | 2.3.10 |
| arduino-cli | 1.5.1 |
| Núcleo ESP32 (Espressif) | `esp32:esp32@3.3.12` (ESP-IDF v5.5.5) |
| Compilador para testes | Apple clang 21 |
| Referência externa | OpenSSL 4.0.3 |
| Placa | ESP32-D0WD-V3 (rev. 3.0), flash 4 MB, sem PSRAM (módulo WROOM-32), USB-serial CH340 |

## Registro de resultados

| Verificação | Resultado |
|---|---|
| Vetores do enunciado (FIPS B e C.1, exemplo de matrículas, blocos repetidos) conferidos com OpenSSL | ✅ todos coincidem |
| Estrutura: `make test` e compilação dos dois firmwares | ✅ |
