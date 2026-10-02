#ifndef BFS_H
#define BFS_H

#include "grafo_lista.h"
#include "matriz_adjacencia.h"

#include <stdint.h>

/* Resultado de uma busca. O chamador pode ler os campos, mas nao deve alterar
 * ponteiros/contagens nem liberar os vetores separadamente.
 * ordem_visita[0..quantidade_alcancados) contem cada alcancado uma unica vez.
 * distancias[0..quantidade_vertices) contem o numero minimo de arestas desde
 * a origem; SIZE_MAX identifica quem nao foi alcancado (nao visitado).
 * O resultado possui seus vetores e independe da vida util do grafo. */
typedef struct {
    size_t quantidade_vertices;
    size_t quantidade_alcancados;
    size_t *ordem_visita;
    size_t *distancias;
} ResultadoBfs;

typedef enum {
    BFS_SUCESSO = 0,
    BFS_ARGUMENTO_INVALIDO,
    BFS_SEM_MEMORIA,
    BFS_LIMITE_EXCEDIDO
} StatusBfs;

/* Busca em largura a partir de um indice interno valido da lista.
 * resultado deve apontar para um ponteiro inicializado com NULL.
 * Em sucesso, entrega a ordem, distancias e quantidade de alcancados;
 * o chamador deve usar liberar_resultado_bfs. Nao altera o grafo.
 * Visita apenas a componente da origem, considerando as conexoes presentes.
 * Empates seguem a enumeracao da lista, sem promessa de ordem crescente.
 * Retorna ARGUMENTO_INVALIDO para NULL, origem fora da faixa, grafo vazio
 * ou saida ja ocupada; LIMITE_EXCEDIDO para tamanho nao representavel;
 * SEM_MEMORIA em falha de alocacao. Erros preservam *resultado e liberam
 * todos os recursos temporarios. Tempo O(V+E), memoria adicional O(V).
 * O grafo deve permanecer valido e sem modificacoes durante a chamada. */
StatusBfs executar_bfs_lista(const GrafoLista *grafo, size_t origem,
                            ResultadoBfs **resultado);

/* Mesmo contrato de executar_bfs_lista, usando a API real da matriz.
 * Examina destinos em ordem crescente; lacos nao repetem a visita.
 * Tempo O(V^2) no pior caso, memoria adicional O(V), sem converter o grafo. */
StatusBfs executar_bfs_matriz(const MatrizAdjacencia *matriz, size_t origem,
                             ResultadoBfs **resultado);

/* Libera os vetores e o resultado e define *resultado como NULL.
 * Aceita NULL e *resultado == NULL; repetir sobre o mesmo dono e seguro.
 * Copias de ponteiros para o resultado/vetores ficam invalidas. */
void liberar_resultado_bfs(ResultadoBfs **resultado);

#endif
