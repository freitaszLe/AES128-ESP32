# Arquitetura

## Visão geral

```
            Placa A (transmissor)                          Placa B (receptor)
  matrículas → chave + mensagem                     ┌→ sincroniza em 0xAA
  mensagem → PKCS#7 → AES-128 ECB                   │  lê n e 16·n bytes (com timeout)
  quadro [AA][n][blocos cifrados] ── UART2 (TX→RX) ─┘  AES-128 ECB inverso → remove PKCS#7
                                                        exibe o texto no Monitor Serial
```

A UART2 (115200 bps, 8N1) transporta **somente bytes brutos** do quadro. Mensagens de
diagnóstico vão para a `Serial` (USB), exibidas no Monitor Serial.

## Organização do código

```
libraries/AES128Core/
├── library.properties     metadados da biblioteca (Arduino IDE / arduino-cli)
└── src/
    ├── AES128Core.h       cabeçalho único incluído pelos sketches
    └── aes128.h / .c      cifra de bloco AES-128 (FIPS-197)
firmware/
├── transmissor/transmissor.ino
└── receptor/receptor.ino
tests/host/
├── teste.h                mini-framework de testes
└── test_main.c            testes do núcleo
```

### Por que um núcleo em C puro

O núcleo não depende de nenhuma API do Arduino. Assim, **o mesmo código** é:

- compilado para o ESP32 pelos dois sketches (transmissor e receptor), sem duplicação;
- compilado no computador com `clang`, permitindo validar o AES contra os vetores do FIPS-197
  antes de gravar qualquer placa.

Os sketches `.ino` são compilados como C++ e o núcleo como C. Os cabeçalhos usam
`extern "C"` para que o compilador C++ não altere os nomes das funções (*name mangling*).

### Como a Arduino IDE encontra a biblioteca

A IDE só procura bibliotecas no *sketchbook* (`~/Documents/Arduino/libraries`). Um link simbólico
aponta para `libraries/AES128Core` deste repositório, de modo que a IDE e o `arduino-cli`
usam exatamente os mesmos arquivos (ver [uso.md](uso.md)).

## Núcleo AES-128

O AES-128 opera sobre blocos de 16 bytes com uma chave de 16 bytes, em 10 rodadas.

### Estado

O bloco é tratado como uma matriz 4×4 de bytes, **preenchida por colunas** (FIPS-197, seção 3.4):

```
entrada: in[0] in[1] ... in[15]

        c=0    c=1    c=2    c=3
r=0   in[0]  in[4]  in[8]  in[12]
r=1   in[1]  in[5]  in[9]  in[13]
r=2   in[2]  in[6]  in[10] in[14]
r=3   in[3]  in[7]  in[11] in[15]
```

No código, o estado é um vetor `uint8_t[16]` nessa mesma ordem: o byte da linha `r`, coluna `c`
está em `s[r + 4*c]`.

### Transformações

| Transformação | Função |
|---|---|
| SubBytes | Substitui cada byte pela S-box (não linearidade) |
| ShiftRows | Desloca a linha `r` em `r` posições para a esquerda (espalha bytes entre colunas) |
| MixColumns | Multiplica cada coluna por uma matriz fixa em GF(2⁸) (mistura bytes dentro da coluna) |
| AddRoundKey | XOR do estado com a chave da rodada |
| Expansão de chave | Gera 11 chaves de rodada (44 palavras de 32 bits) a partir da chave |

### Sequência da cifragem

```
AddRoundKey(chave 0)
rodadas 1 a 9:  SubBytes → ShiftRows → MixColumns → AddRoundKey(chave r)
rodada 10:      SubBytes → ShiftRows →              AddRoundKey(chave 10)
```

A última rodada não tem MixColumns, conforme o FIPS-197.

### Aritmética em GF(2⁸)

Cada byte é um polinômio de grau < 8 com coeficientes em {0, 1}. A soma é XOR e a multiplicação
é feita módulo o polinômio irredutível m(x) = x⁸ + x⁴ + x³ + x + 1.

- `xtime(a)` multiplica por {02}: desloca 1 bit para a esquerda e, se o bit 7 era 1, faz XOR com `0x1B`.
- A multiplicação genérica soma (XOR) as potências `a·x^i` correspondentes aos bits 1 do outro fator.
- No MixColumns: `{02}·a = xtime(a)` e `{03}·a = xtime(a) ^ a`.

### Origem da S-box

Para cada byte b: calcula-se o inverso multiplicativo b⁻¹ em GF(2⁸) (com 0 → 0) e aplica-se a
transformação afim `s = b' ⊕ rotl(b',1) ⊕ rotl(b',2) ⊕ rotl(b',3) ⊕ rotl(b',4) ⊕ 0x63`.
O inverso fornece a não linearidade; a transformação afim elimina pontos fixos. No código a S-box é
uma tabela de 256 bytes; os testes recalculam todos os valores a partir dessa definição e comparam.

### Expansão de chave

Gera 44 palavras de 4 bytes (`w[0..43]`), ou seja, 11 chaves de rodada de 16 bytes:

- `w[0..3]` = a própria chave;
- para `i ≥ 4`: `temp = w[i-1]`; se `i` é múltiplo de 4,
  `temp = SubWord(RotWord(temp)) ⊕ Rcon[i/4]`; então `w[i] = w[i-4] ⊕ temp`.

`RotWord` gira a palavra 1 byte para a esquerda, `SubWord` aplica a S-box aos 4 bytes e
`Rcon` = {01}, {02}, {04}, …, {36}, obtido aplicando `xtime` sucessivamente.

### API (`aes128.h`)

| Função | Descrição |
|---|---|
| `aes128_xtime`, `aes128_gf_mult` | Aritmética em GF(2⁸) |
| `aes128_sbox` | Valor da S-box |
| `aes128_sub_bytes`, `aes128_shift_rows`, `aes128_mix_columns`, `aes128_add_round_key` | Transformações, aplicadas no próprio estado |
| `aes128_expandir_chave` | Chave de 16 bytes → 176 bytes de chaves de rodada |
| `aes128_cifrar_bloco` | Cifra um bloco de 16 bytes |

As transformações são públicas para permitir testá-las isoladamente e registrar os estados
intermediários de cada rodada.

## Protocolo do quadro UART

| Campo | Tamanho | Valor |
|---|---|---|
| Marcador de início | 1 byte | `0xAA` (fixo) |
| Número de blocos `n` | 1 byte | quantidade de blocos de 16 bytes |
| Blocos cifrados | 16 × `n` bytes | texto cifrado em ECB |

Exemplo com dois blocos (34 bytes):

```
AA 02 3a3784991ab0acaa88d5fd94cb7b74ad cbea1d9e3e1f39da47111cf69064b166
```
