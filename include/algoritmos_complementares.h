#ifndef ALGORITMOS_COMPLEMENTARES_H
#define ALGORITMOS_COMPLEMENTARES_H

#include "grafo_lista.h"
#include "matriz_adjacencia.h"

#include <stddef.h>

typedef enum {
    ALGORITMO_SUCESSO = 0,
    ALGORITMO_ARGUMENTO_INVALIDO,
    ALGORITMO_SEM_MEMORIA,
    ALGORITMO_LIMITE_EXCEDIDO
} ResultadoAlgoritmo;

/* Analisa um digrafo armazenado na matriz direcionada. Conectividade fraca
 * ignora a orientacao dos arcos; forte exige caminho dirigido entre todo par.
 * As saidas recebem 0 ou 1 somente em sucesso e sao preservadas em erro.
 * O digrafo vazio resulta em fraca=0 e forte=0; um unico vertice resulta 1/1.
 * Nao altera a matriz. Tempo O(V^2), memoria auxiliar O(V). */
ResultadoAlgoritmo verificar_conectividade_digrafo(
    const MatrizAdjacencia *digrafo, int *conectividade_fraca,
    int *conectividade_forte);

typedef struct {
    size_t origem;
    size_t destino;
} Ponte;

/* Resultado conjunto da DFS de Tarjan. articulacoes[v] vale 1 quando v e
 * articulacao. Cada ponte aparece uma vez, com origem < destino.
 * O resultado possui os vetores e deve ser liberado pela funcao abaixo. */
typedef struct {
    size_t quantidade_vertices;
    size_t quantidade_articulacoes;
    unsigned char *articulacoes;
    size_t quantidade_pontes;
    Ponte *pontes;
} ResultadoCriticidade;

/* Encontra articulacoes e pontes da lista nao direcionada em uma unica DFS,
 * inclusive quando o grafo tem varias componentes. resultado deve apontar
 * para NULL. Em erro, preserva a saida e libera construcoes parciais.
 * Nao modifica o grafo. Tempo O(V+E), memoria auxiliar O(V+E).
 * A DFS e recursiva para manter a demonstracao classica didatica. */
ResultadoAlgoritmo analisar_criticidade_lista(
    const GrafoLista *grafo, ResultadoCriticidade **resultado);

/* Libera articulacoes, pontes e resultado; aceita NULL e zera o dono. */
void liberar_resultado_criticidade(ResultadoCriticidade **resultado);

typedef struct {
    size_t quantidade_vertices;
    size_t quantidade_cores;
    size_t *cores;
} ResultadoColoracao;

/* Colore os vertices na ordem 0..V-1. Cada vertice recebe a menor cor
 * (0, 1, 2, ...) ausente entre vizinhos ja coloridos. resultado deve apontar
 * para NULL. Nao altera o grafo. Tempo O(V+E), memoria auxiliar O(V). */
ResultadoAlgoritmo colorir_grafo_guloso(
    const GrafoLista *grafo, ResultadoColoracao **resultado);

/* Libera o vetor de cores e o resultado; aceita NULL e zera o dono. */
void liberar_resultado_coloracao(ResultadoColoracao **resultado);

typedef struct {
    int satisfaz_formula;
    int limite_arestas_aplicavel;
    int satisfaz_limite_arestas;
} ResultadoEuler;

/* Verifica V - E + F = 2 pela forma V + F = E + 2. Tambem informa a
 * condicao E <= 3V-6 quando V >= 3. Essa desigualdade e apenas NECESSARIA
 * para planaridade de grafo simples planar (e nao prova planaridade).
 * A formula pressupoe uma representacao planar conexa e faces conhecidas;
 * isoladamente, tambem nao e um algoritmo geral de planaridade.
 * Retorna LIMITE_EXCEDIDO se uma soma da formula nao couber em size_t. */
ResultadoAlgoritmo verificar_formula_euler(size_t quantidade_vertices,
                                          size_t quantidade_arestas,
                                          size_t quantidade_faces,
                                          ResultadoEuler *resultado);

#endif
