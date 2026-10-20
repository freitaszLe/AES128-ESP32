/*
 * test_main.c — testes do núcleo AES128Core executados no computador.
 *
 * Confirma que o núcleo compila e é ligado ao programa de teste.
 */
#include "teste.h"
#include "aes128.h"

static void teste_biblioteca_ligada(void)
{
    printf("[biblioteca] versão %s\n", aes128core_versao());
    CHECAR(aes128core_versao() != NULL);
    CHECAR(strcmp(aes128core_versao(), "0.1.0") == 0);
}

int main(void)
{
    teste_biblioteca_ligada();
    return teste_resumo();
}
