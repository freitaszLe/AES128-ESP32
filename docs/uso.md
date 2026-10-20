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

## 3. Testes no computador

```bash
make test
```

## 4. Compilar e gravar pelo terminal

```bash
make firmware                                # compila os dois sketches
make upload-tx PORTA=/dev/cu.usbserial-XXX   # grava o transmissor
make upload-rx PORTA=/dev/cu.usbserial-XXX   # grava o receptor
```

A gravação usa 460800 baud. Na velocidade padrão (921600), conversores CH340 ligados por hub USB
podem falhar com *"Unable to verify flash chip connection"*.

## 5. Validação na placa

Grava um sketch que executa os vetores do FIPS-197 (apêndices B e C.1) na placa e mede o tempo
de cifragem. O resultado é repetido a cada 10 segundos no Monitor Serial (115200 baud):

```bash
make upload-fips PORTA=/dev/cu.usbserial-XXX
```

Feche o Monitor Serial antes de gravar: dois programas abertos na mesma porta disputam os bytes,
o que causa falhas na gravação e texto embaralhado no monitor.

## 6. Arduino IDE

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

## 7. Conferência com OpenSSL

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
