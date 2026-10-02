#ifndef BFS_INTERNO_H
#define BFS_INTERNO_H

#include "bfs.h"

/* Interface interna compartilhada por bfs.c e componentes_conexos.c.
 * Reserva uma busca para V > 0, inicializando visitados (distancias) uma vez.
 * resultado deve apontar para NULL. Retorna os erros de tamanho/alocacao
 * da BFS, preservando a saida em erro. Libere com liberar_resultado_bfs. */
StatusBfs criar_busca_bfs(size_t quantidade_vertices, ResultadoBfs **resultado);

/* Continua uma busca criada acima, no mesmo grafo inalterado e com V igual.
 * origem deve ser valida e ainda nao visitada. Os chamadores internos garantem
 * essas precondicoes. Nao aloca nem limpa vetores: acrescenta descobertas em
 * ordem_visita a partir de quantidade_alcancados, preservando os visitados.
 * A fila comeca nessa posicao, sem processar componentes anteriores.
 * Retorna SUCESSO ou ARGUMENTO_INVALIDO se uma consulta ao grafo falhar;
 * nesse erro o chamador deve descartar a busca inteira, possivelmente parcial.
 * Distancias de cada nova componente partem de zero em sua propria origem. */
StatusBfs continuar_bfs_lista(const GrafoLista *grafo, size_t origem,
                             ResultadoBfs *busca);
StatusBfs continuar_bfs_matriz(const MatrizAdjacencia *matriz, size_t origem,
                              ResultadoBfs *busca);

#endif
