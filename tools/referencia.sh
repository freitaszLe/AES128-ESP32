#!/bin/sh
# referencia.sh — conferência externa com OpenSSL.
#
# NÃO faz parte do código entregue: serve apenas como "gabarito" para comparar
# os resultados da nossa implementação do AES-128.
#
# Uso:
#   tools/referencia.sh cifrar   <chave_hex_32> <blocos_hex>
#   tools/referencia.sh decifrar <chave_hex_32> <blocos_hex>
#   tools/referencia.sh texto    <chave_hex_32> "<texto múltiplo de 16 bytes>"
#
# Modo ECB, sem preenchimento automático (-nopad): a entrada deve ter múltiplo de 16 bytes.
set -eu

uso() { sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'; exit 1; }
[ $# -eq 3 ] || uso

op=$1; chave=$2; dado=$3
case "$chave" in *[!0-9a-fA-F]*) echo "chave não é hexadecimal" >&2; exit 1;; esac
[ ${#chave} -eq 32 ] || { echo "a chave deve ter 32 dígitos hex (16 bytes)" >&2; exit 1; }

case "$op" in
  cifrar)   printf '%s' "$dado" | xxd -r -p | openssl enc -aes-128-ecb -nopad -K "$chave" | xxd -p -c 16 ;;
  decifrar) printf '%s' "$dado" | xxd -r -p | openssl enc -d -aes-128-ecb -nopad -K "$chave" | xxd -p -c 16 ;;
  texto)    printf '%s' "$dado" | openssl enc -aes-128-ecb -nopad -K "$chave" | xxd -p -c 16 ;;
  *) uso ;;
esac
