/*
 * analise_host.c — gera no computador o mesmo relatório de análise que o
 * sketch firmware/analise imprime no ESP32, para conferência.
 *
 * Uso: build/analise [matricula_A matricula_B]
 */
#include <stdio.h>

#include "analise.h"

#define MATRICULA_A "2022178440126"
#define MATRICULA_B "2022178440142"

static void escrever(const char *linha)
{
    puts(linha);
}

int main(int argc, char **argv)
{
    if (argc == 3)
        analise_relatorio(argv[1], argv[2], escrever);
    else
        analise_relatorio(MATRICULA_A, MATRICULA_B, escrever);
    return 0;
}
