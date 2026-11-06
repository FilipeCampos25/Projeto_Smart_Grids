#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "grafo_lista.h"
#include "matriz_adjacencia.h"

#include <stddef.h>

typedef enum {
    DIJKSTRA_SUCESSO = 0,
    DIJKSTRA_ARGUMENTO_INVALIDO,
    DIJKSTRA_SEM_MEMORIA,
    DIJKSTRA_ARESTA_SEM_PESO,
    DIJKSTRA_OVERFLOW,
    DIJKSTRA_ERRO_GRAFO
} StatusDijkstra;

/* REJEITAR exige que toda aresta do grafo tenha peso. IGNORAR trata arestas
 * sem peso como indisponiveis, nunca como arestas de custo zero. */
typedef enum {
    DIJKSTRA_REJEITAR_ARESTA_SEM_PESO = 0,
    DIJKSTRA_IGNORAR_ARESTA_SEM_PESO
} PoliticaPesoDijkstra;

typedef struct {
    size_t *caminho;
    size_t quantidade_vertices_caminho;
    size_t quantidade_vertices_processados;
    double custo_total;
    int alcancavel;
    int utiliza_custo_assumido;
} ResultadoDijkstra;

/* Os vertices sao indices internos. Em sucesso, inclusive quando nao existe
 * caminho, entrega um ResultadoDijkstra que pertence ao chamador. A lista usa
 * heap binario indexado; a matriz usa selecao por varredura O(V^2). */
StatusDijkstra executar_dijkstra_lista(
    const GrafoLista *grafo, size_t origem, size_t destino,
    PoliticaPesoDijkstra politica, ResultadoDijkstra **resultado);
StatusDijkstra executar_dijkstra_matriz(
    const MatrizAdjacencia *matriz, size_t origem, size_t destino,
    PoliticaPesoDijkstra politica, ResultadoDijkstra **resultado);

void liberar_resultado_dijkstra(ResultadoDijkstra **resultado);
const char *descricao_status_dijkstra(StatusDijkstra status);

#endif
