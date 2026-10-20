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

### Dados da dupla

| Item | Valor |
|---|---|
| Matrículas | A = `2022178440126`, B = `2022178440142` |
| 8 últimos dígitos | A = `78440126`, B = `78440142` |
| Chave (ASCII) | `7844012678440142` |
| Chave (hex) | `37383434303132363738343430313432` |
| Mensagem (31 bytes) | `A:2022178440126;B:2022178440142` |
| Bloco 1 com PKCS#7 | `413a323032323137383434303132363b` |
| Bloco 2 com PKCS#7 | `423a3230323231373834343031343201` (1 byte de preenchimento `0x01`) |
| Cifrado, bloco 1 | `907e9c613af4c51b5962ab262c545ec0` |
| Cifrado, bloco 2 | `39c740f27e00a499c0e34ea9fd06076b` |

Quadro UART correspondente (34 bytes): `AA 02 907e9c61…5ec0 39c740f2…076b`.

### Blocos repetidos em ECB (chave do exemplo)

| Bloco | Texto claro (16 bytes) | Cifrado |
|---|---|---|
| 1 | `"LIGAR LED VERDE."` | `e0c22a6f02664f487dc9528642203dec` |
| 2 | `"DESLIGAR LED    "` (4 espaços no final) | `2b4d065ea65478b60696616244cd5070` |
| 3 | `"LIGAR LED VERDE."` | `e0c22a6f02664f487dc9528642203dec` |

## Registros da análise (dados da dupla)

Gerados por `firmware/analise` na placa e por `make analise` no computador (saídas idênticas).

### Chaves de rodada

| Rodada | Chave |
|---|---|
| 0 | `37383434303132363738343430313432` |
| 1 | `f1201730c1112506f6291132c6182500` |

### Estados do bloco 1 na rodada 1

| Fase | Estado |
|---|---|
| Entrada (bloco 1) | `413a323032323137383434303132363b` |
| (1) AddRoundKey (chave 0) | `76020604020303010f0c000401030209` |
| (2) SubBytes | `38776ff2777b7b7c76fe63f27c7b7701` |
| (3) ShiftRows | `387b630177fe77f2767b6f7c7c777bf2` |
| (4) MixColumns | `9f6a865272fb6aef724d5776e8edf077` |
| (5) AddRoundKey (chave 1) | `6e4a9162b3ea4fe9846446442ef5d577` |

### Efeito avalanche

| Alteração | Texto cifrado resultante | Bits alterados |
|---|---|---|
| Nenhuma (original) | `907e9c613af4c51b5962ab262c545ec0` `39c740f27e00a499c0e34ea9fd06076b` | — |
| Bit 127 da chave (`…0142` → `…0143`) | `913c69859192ec1d814f7a32eee0e559` `30fe9563db8bb9a96de30694fec74b52` | bloco 1: 58/128 (45,31%) · bloco 2: 52/128 (40,62%) · total: 110/256 (42,97%) |
| Bit 7 da mensagem (`'A'` → `'@'`) | `da03aeb3304e881a9a1f2bdb55c18783` `39c740f27e00a499c0e34ea9fd06076b` | bloco 1: 63/128 (49,22%) · bloco 2: 0/128 (0%) · total: 63/256 (24,61%) |

Média invertendo cada bit, um de cada vez:

| Posições | Média | Mínimo | Máximo |
|---|---|---|---|
| 128 bits da chave (sobre os 256 bits cifrados) | 49,90% | 41,80% | 60,16% |
| 128 bits do bloco 1 da mensagem (sobre o bloco 1 cifrado) | 50,57% | 40,62% | 63,28% |

Uma única alteração varia em torno de 50% (o valor exato depende do bit escolhido); a média sobre
todas as posições fica muito próxima dos 50% esperados. Alterar 1 bit da mensagem muda apenas o
bloco onde ele está: em ECB o bloco 2 não é afetado.

### Experimento ECB

| Bloco | Texto claro | Cifrado |
|---|---|---|
| 1 | `"LIGAR LED VERDE."` | `e0c22a6f02664f487dc9528642203dec` |
| 2 | `"DESLIGAR LED    "` | `2b4d065ea65478b60696616244cd5070` |
| 3 | `"LIGAR LED VERDE."` | `e0c22a6f02664f487dc9528642203dec` |

O bloco 3 cifrado é idêntico ao bloco 1: em ECB, textos claros iguais sob a mesma chave sempre geram
textos cifrados iguais.

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
| GF(2⁸): `xtime` e multiplicação (FIPS-197 seção 4.2) | ✅ |
| S-box: 256 valores da tabela iguais aos recalculados (inverso + transformação afim) | ✅ |
| Expansão de chave: 11 chaves de rodada iguais ao apêndice A.1 | ✅ |
| SubBytes, ShiftRows, MixColumns e AddRoundKey isolados (apêndice B, rodada 1) | ✅ |
| Cinco estados da rodada 1 a partir do texto claro (apêndice B) | ✅ |
| Cifragem: apêndices B e C.1 | ✅ |
| Sensibilidade dos testes: defeitos introduzidos de propósito (byte da S-box, sentido do ShiftRows, Rcon) foram todos detectados | ✅ |
| S-box inversa: `InvSbox(Sbox(b)) = b` para os 256 bytes | ✅ |
| InvSubBytes, InvShiftRows e InvMixColumns desfazem as diretas; conferem com o apêndice C.1 (rodada 1 inversa) | ✅ |
| Decifragem: apêndices B e C.1 recuperam o texto claro | ✅ |
| Ida e volta: `Decifrar(Cifrar(x)) = x` em 1000 chaves e blocos pseudoaleatórios | ✅ |
| Sensibilidade dos testes da decifragem: coeficiente do InvMixColumns, sentido do InvShiftRows e ordem das chaves, todos detectados | ✅ |
| ESP32: apêndices B e C.1 cifrados e decifrados corretamente na placa (`validacao_fips`) | ✅ |
| Matrículas: chave e mensagem do exemplo do enunciado; matrículas com menos e mais de 8 dígitos e com caracteres não numéricos | ✅ |
| PKCS#7: mensagens de 0, 15, 16, 17 e 21 bytes; rejeição de preenchimentos inválidos | ✅ |
| ECB: exemplo do enunciado reproduzido byte a byte (mensagem com PKCS#7, 2 blocos cifrados, decifragem e remoção do preenchimento) | ✅ |
| Sensibilidade dos testes: PKCS#7 sem bloco extra, remoção sem conferir todos os bytes, uso dos 8 primeiros dígitos — todos detectados | ✅ |
| Dados da dupla: chave, mensagem com PKCS#7 e texto cifrado iguais aos obtidos com OpenSSL (inclusive com o PKCS#7 automático do OpenSSL) | ✅ |
| Análise: estados da rodada 1 conferidos pelo mesmo código com o apêndice B do FIPS-197; contagem e inversão de bits testadas | ✅ |
| Avalanche: textos cifrados com chave e mensagem alteradas e contagens de bits conferidos com OpenSSL | ✅ |
| Experimento ECB: 3 blocos iguais à tabela do enunciado | ✅ |
| Relatório gerado na placa idêntico, byte a byte, ao gerado no computador (62 linhas) | ✅ |

### Desempenho no ESP32 (240 MHz, média de 1000 execuções)

| Operação | Tempo |
|---|---|
| Expansão de chave | 24,06 µs |
| Cifrar 1 bloco | 55,95 µs |
| Decifrar 1 bloco | 404,13 µs |

A decifragem é mais lenta porque o InvMixColumns usa a multiplicação genérica em GF(2⁸)
(laço bit a bit) com os coeficientes {0e, 0b, 0d, 09}, enquanto o MixColumns usa apenas `xtime`
e XOR para {02} e {03}. Para este trabalho (poucos blocos por mensagem) a diferença é irrelevante.
