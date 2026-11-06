#ifndef AGM_H
#define AGM_H

#include "grafo_lista.h"
#include "matriz_adjacencia.h"

#include <stddef.h>

typedef enum {
    AGM_SUCESSO = 0,
    AGM_ARGUMENTO_INVALIDO,
    AGM_SEM_MEMORIA,
    AGM_LIMITE_EXCEDIDO,
    AGM_PESO_AUSENTE,
    AGM_PESO_INVALIDO,
    AGM_GRAFO_DIRECIONADO,
    AGM_OVERFLOW_CUSTO
} StatusAGM;

typedef struct {
    size_t origem;
    size_t destino;
    PesoAresta peso;
} ArestaAGM;

typedef struct {
    ArestaAGM *arestas;
    size_t quantidade_arestas;
    double custo_total;
    size_t quantidade_componentes;
    int arvore_completa;
} ResultadoAGM;

/*
 * Todas as funcoes recebem um grafo nao direcionado e ponderado. Qualquer
 * aresta existente sem peso invalida toda a entrada (AGM_PESO_AUSENTE); peso
 * zero e valido. Valores e origens sao copiados sem conversao. O custo agrega
 * valores numericos, nao atribui uma unidade fisica a custos assumidos.
 *
 * A saida deve estar vazia (arestas == NULL). Em erro, ela permanece intacta;
 * em sucesso, o chamador libera a propriedade com liberar_resultado_agm.
 * Grafos desconectados produzem floresta minima. O grafo recebido nao muda.
 * O vazio tem zero componentes e nao e arvore completa; grafo com um vertice
 * isolado tem uma componente e e arvore completa.
 *
 * Prim usa O(V^2 + E) tempo e O(V + E) memoria auxiliar/resultado, em ambas
 * as representacoes. Kruskal usa O(V + E log E) tempo e O(V + E) memoria.
 * Para matriz, ambas as funcoes tambem precisam inspecionar O(V^2) celulas.
 */
StatusAGM agm_prim_lista(const GrafoLista *grafo, ResultadoAGM *resultado);
StatusAGM agm_kruskal_lista(const GrafoLista *grafo, ResultadoAGM *resultado);
StatusAGM agm_prim_matriz(const MatrizAdjacencia *matriz,
                          ResultadoAGM *resultado);
StatusAGM agm_kruskal_matriz(const MatrizAdjacencia *matriz,
                             ResultadoAGM *resultado);

void liberar_resultado_agm(ResultadoAGM *resultado);

#endif
