#ifndef METRICAS_H
#define METRICAS_H

#include "dataset.h"

#include <stdio.h>

double tempo_atual_milissegundos(void);

int registrar_metrica(FILE *arquivo, const char *algoritmo,
                      const char *representacao, size_t vertices,
                      size_t arestas, double tempo_ms, size_t memoria_bytes,
                      size_t repeticao);

/* Executa BFS, DFS e componentes nas duas representacoes para prefixos
 * deterministas da ordem BFS gravada no dataset normalizado. */
int executar_experimentos(const Dataset *dataset, const char *arquivo_csv,
                          size_t repeticoes);

#endif
