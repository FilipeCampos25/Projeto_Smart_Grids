#ifndef COBERTURA_VERTICES_H
#define COBERTURA_VERTICES_H

#include "grafo_lista.h"
#include "matriz_adjacencia.h"

#include <stddef.h>

typedef enum {
    COBERTURA_SUCESSO = 0,
    COBERTURA_ARGUMENTO_INVALIDO,
    COBERTURA_SEM_MEMORIA,
    COBERTURA_LIMITE_ATINGIDO,
    COBERTURA_GRAFO_DIRECIONADO
} StatusCoberturaVertices;

typedef enum {
    COBERTURA_METODO_EXATO = 0,
    COBERTURA_METODO_APROXIMADO
} MetodoCoberturaVertices;

/* SIZE_MAX desativa cada limite. Um limite zero interrompe antes de explorar
 * qualquer instancia/no, respectivamente. A configuracao padrao e deliberada-
 * mente conservadora para impedir Backtracking acidental na rede completa. */
typedef struct {
    size_t maximo_vertices;
    size_t maximo_nos_explorados;
} ConfiguracaoCoberturaExata;

typedef struct {
    MetodoCoberturaVertices metodo;
    size_t quantidade_vertices_grafo;
    size_t cardinalidade;
    unsigned char *selecionados;
    int cobertura_valida;
    int otima_comprovada;
    size_t nos_explorados;
    size_t memoria_resultado_bytes;
    size_t memoria_auxiliar_pico_bytes;
} ResultadoCoberturaVertices;

/* Limites seguros usados quando configuracao == NULL: ate 60 vertices e
 * 1.000.000 de nos da arvore de busca. */
ConfiguracaoCoberturaExata configuracao_cobertura_exata_padrao(void);

/* Solucao exata por Backtracking. COBERTURA_SUCESSO implica otima_comprovada=1.
 * COBERTURA_LIMITE_ATINGIDO pode entregar uma cobertura viavel, mas sempre com
 * otima_comprovada=0. No pior caso, o tempo e O(2^V * (V+E)); a memoria
 * auxiliar e O(V+E), alem da pilha recursiva O(V). */
StatusCoberturaVertices cobertura_vertices_exata_lista(
    const GrafoLista *grafo, const ConfiguracaoCoberturaExata *configuracao,
    ResultadoCoberturaVertices **resultado);
StatusCoberturaVertices cobertura_vertices_exata_matriz(
    const MatrizAdjacencia *grafo,
    const ConfiguracaoCoberturaExata *configuracao,
    ResultadoCoberturaVertices **resultado);

/* Aproximacao 2 para grafos nao direcionados: constroi um matching maximal e
 * escolhe os dois extremos de cada aresta do matching. Tempo O(V+E) depois da
 * extracao da lista, ou O(V^2) para percorrer a matriz; memoria O(V+E). */
StatusCoberturaVertices cobertura_vertices_aproximada_lista(
    const GrafoLista *grafo, ResultadoCoberturaVertices **resultado);
StatusCoberturaVertices cobertura_vertices_aproximada_matriz(
    const MatrizAdjacencia *grafo, ResultadoCoberturaVertices **resultado);

/* Validam um vetor booleano com exatamente um item por vertice. */
StatusCoberturaVertices validar_cobertura_lista(
    const GrafoLista *grafo, const unsigned char *selecionados,
    size_t quantidade_vertices, int *valida);
StatusCoberturaVertices validar_cobertura_matriz(
    const MatrizAdjacencia *grafo, const unsigned char *selecionados,
    size_t quantidade_vertices, int *valida);

/* Acesso emprestado: o vetor permanece pertencendo ao resultado. */
const unsigned char *vertices_selecionados_cobertura(
    const ResultadoCoberturaVertices *resultado);
size_t cardinalidade_cobertura(const ResultadoCoberturaVertices *resultado);

void liberar_resultado_cobertura(ResultadoCoberturaVertices **resultado);
const char *descricao_status_cobertura(StatusCoberturaVertices status);

#endif
