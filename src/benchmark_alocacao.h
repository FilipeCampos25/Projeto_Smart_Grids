#ifndef BENCHMARK_ALOCACAO_H
#define BENCHMARK_ALOCACAO_H

#include <stddef.h>

void *benchmark_malloc(size_t tamanho);
void benchmark_free(void *ponteiro);
int benchmark_alocacao_reiniciar(void);
size_t benchmark_alocacao_pico(void);
size_t benchmark_alocacao_ativa(void);

#endif
