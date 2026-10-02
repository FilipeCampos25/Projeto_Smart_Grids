#ifndef ALOCACAO_TESTE_H
#define ALOCACAO_TESTE_H

#include <stddef.h>

/* Substitutos usados somente no objeto de teste do modulo. Simulam falta
 * de memoria e contam blocos vivos. O teste continua usando a libc normal. */
void *teste_malloc(size_t tamanho);
void *teste_realloc(void *ponteiro, size_t tamanho);
void teste_free(void *ponteiro);

#endif
