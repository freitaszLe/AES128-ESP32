/*
 * aes128.h — interface do AES-128 (FIPS-197).
 *
 * Versão inicial: esqueleto da biblioteca.
 */
#ifndef AES128_H
#define AES128_H

/* Os sketches são compilados como C++; o núcleo, como C. O bloco extern "C"
 * evita que o compilador C++ altere os nomes das funções (name mangling). */
#ifdef __cplusplus
extern "C" {
#endif

/* Versão do núcleo, usada para confirmar que a biblioteca foi ligada ao programa. */
const char *aes128core_versao(void);

#ifdef __cplusplus
}
#endif

#endif /* AES128_H */
