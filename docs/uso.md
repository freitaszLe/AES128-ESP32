# Como usar

## 1. Instalação (macOS)

Com o [Homebrew](https://brew.sh):

```bash
brew install --cask arduino-ide
brew install arduino-cli
```

Núcleo ESP32 da Espressif (versão fixada para reprodutibilidade):

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12
```

Para os testes no computador bastam `clang` e `make` (Command Line Tools do Xcode:
`xcode-select --install`). O OpenSSL é opcional e só é usado por `tools/referencia.sh`.

## 2. Porta serial

Conecte a placa por USB e liste as portas:

```bash
arduino-cli board list
```

No macOS, use a porta **`/dev/cu.usbserial-*`** (não a `/dev/tty.*`). Placas com conversor CH340
usam o driver nativo do macOS.

## 3. Montagem das placas

Cada placa é alimentada pelo **próprio cabo USB**. Entre as placas ligam-se apenas três fios:

| Fio | Placa A (transmissor) | Placa B (receptor) |
|---|---|---|
| 1 | TX2 (GPIO17) | RX2 (GPIO16) |
| 2 | RX2 (GPIO16) | TX2 (GPIO17) |
| 3 | GND | GND |

```
   Placa A                    Placa B
  GPIO17 (TX2) ───────────────► GPIO16 (RX2)
  GPIO16 (RX2) ◄─────────────── GPIO17 (TX2)
  GND ───────────────────────── GND
```

- TX e RX são cruzados: o pino que transmite em uma placa vai no pino que recebe na outra.
- O GND comum dá às duas placas a mesma referência de 0 V; sem ele os níveis lógicos não são
  interpretados corretamente.
- **Não** ligar 5V, VIN ou 3V3 entre as placas.
- Algumas placas identificam esses pinos como `TX2`/`RX2`, outras como `D17`/`D16`.
- Fazer as ligações com os cabos USB desconectados.

### Teste da ligação (sem criptografia)

```bash
make upload-teste-uart PORTA=/dev/cu.usbserial-XXX   # gravar nas duas placas
```

Cada placa envia continuamente os bytes `0x00…0xFF` pela UART2 e confere a sequência recebida.
No Monitor Serial (115200 baud) aparece, a cada 2 s:

```
TX 22336 bytes | RX 22272 bytes, 0 erro(s) | ultimo 0xff | OK
```

Enviar `z` pelo Monitor Serial zera os contadores. "RX: nada recebido" indica fio solto, TX/RX
não cruzados ou outra placa desligada.

## 4. Testes no computador

```bash
make test
```

## 5. Compilar e gravar pelo terminal

```bash
make firmware                                # compila os dois sketches
make upload-tx PORTA=/dev/cu.usbserial-XXX   # grava o transmissor
make upload-rx PORTA=/dev/cu.usbserial-XXX   # grava o receptor
```

A gravação usa 460800 baud. Na velocidade padrão (921600), conversores CH340 ligados por hub USB
podem falhar com *"Unable to verify flash chip connection"*.

## 6. Validação na placa

Grava um sketch que executa os vetores do FIPS-197 (apêndices B e C.1) na placa e mede o tempo
de cifragem. O resultado é repetido a cada 10 segundos no Monitor Serial (115200 baud):

```bash
make upload-fips PORTA=/dev/cu.usbserial-XXX
```

Feche o Monitor Serial antes de gravar: dois programas abertos na mesma porta disputam os bytes,
o que causa falhas na gravação e texto embaralhado no monitor.

### Relatório de análise

```bash
make analise                                   # no computador
make upload-analise PORTA=/dev/cu.usbserial-XXX # na placa
```

Na placa, o relatório é impresso ao ligar e de novo a cada caractere enviado pelo Monitor Serial
(basta pressionar Enter).

## 7. Arduino IDE

Uma única vez, crie o link da biblioteca no sketchbook (na raiz do repositório):

```bash
mkdir -p ~/Documents/Arduino/libraries
ln -s "$PWD/libraries/AES128Core" ~/Documents/Arduino/libraries/AES128Core
```

Depois:

1. Abra `firmware/transmissor/transmissor.ino` ou `firmware/receptor/receptor.ino`.
2. *Ferramentas → Placa → esp32 → ESP32 Dev Module*.
3. *Ferramentas → Upload Speed → 460800*.
4. *Ferramentas → Porta → /dev/cu.usbserial-…*.
5. Carregar (→).
6. Monitor Serial em **115200 baud**.

> A velocidade de gravação (460800) não tem relação com a do Monitor Serial nem com a da UART2
> (ambas 115200).

## 8. Conferência com OpenSSL

```bash
tools/referencia.sh cifrar   <chave_hex> <blocos_hex>
tools/referencia.sh decifrar <chave_hex> <blocos_hex>
tools/referencia.sh texto    <chave_hex> "<texto com múltiplo de 16 bytes>"
```

Exemplo (FIPS-197, Apêndice B):

```bash
tools/referencia.sh cifrar 2b7e151628aed2a6abf7158809cf4f3c 3243f6a8885a308d313198a2e0370734
# 3925841d02dc09fbdc118597196a0b32
```
