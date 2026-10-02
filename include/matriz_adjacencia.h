#ifndef MATRIZ_ADJACENCIA_H
#define MATRIZ_ADJACENCIA_H

#include <stddef.h>

/* Grafo nao direcionado, nao ponderado, com indices de 0 a N-1.
 * A estrutura opaca impede alteracoes externas que quebrem a simetria.
 * Duplicatas sao idempotentes; lacos sao aceitos na diagonal.
 * Essas politicas sao provisorias ate a conclusao da modelagem (#3).
 */
typedef struct MatrizAdjacencia MatrizAdjacencia;

typedef enum {
    MATRIZ_SUCESSO = 0,
    MATRIZ_ARGUMENTO_INVALIDO,
    MATRIZ_TAMANHO_INVALIDO,
    MATRIZ_LIMITE_MEMORIA,
    MATRIZ_SEM_MEMORIA
} ResultadoMatriz;

/* Calcula sizeof(estrutura) + N*N*sizeof(unsigned char), sem alocar.
 * N=0 e valido (somente o cabecalho). Retorna TAMANHO_INVALIDO se o
 * calculo exceder SIZE_MAX ou ARGUMENTO_INVALIDO se bytes for NULL.
 * Em erro, nao altera *bytes. Nao inclui metadados do alocador nem RSS.
 */
ResultadoMatriz estimar_memoria_matriz(size_t quantidade_vertices, size_t *bytes);

/* Cria uma matriz sem conexoes. limite_bytes e o teto para a estrutura
 * inteira; zero rejeita inclusive a matriz vazia. matriz_criada deve
 * apontar para um ponteiro inicializado com NULL (nao sobrescreve donos).
 * Retorna erro de argumento, tamanho, limite ou alocacao; em falha nao
 * entrega uma matriz parcial e libera tudo que tiver alocado.
 * Em sucesso, o chamador deve usar destruir_matriz para liberar a matriz.
 */
ResultadoMatriz criar_matriz(size_t quantidade_vertices, size_t limite_bytes,
                            MatrizAdjacencia **matriz_criada);

/* Insere a conexao nos dois sentidos. Indices devem estar em [0, N).
 * Repetir uma insercao (inclusive invertida) nao cria arestas paralelas.
 * Lacos usam uma unica celula. Retorna ARGUMENTO_INVALIDO para matriz
 * NULL ou indices invalidos, sem alterar a matriz. Nao aloca memoria.
 */
ResultadoMatriz inserir_aresta(MatrizAdjacencia *matriz, size_t vertice_origem,
                              size_t vertice_destino);

/* Remove a conexao nos dois sentidos, mesmo se ja estiver ausente.
 * Valida como inserir_aresta, nao aloca e permite restauracao por nova
 * insercao. Nao guarda historico nem o status de segmentos individuais.
 */
ResultadoMatriz remover_aresta(MatrizAdjacencia *matriz, size_t vertice_origem,
                              size_t vertice_destino);

/* Escreve 1 (conexao) ou 0 (ausencia) em *adjacente. Retorna
 * ARGUMENTO_INVALIDO se matriz/adjacente for NULL ou algum indice for
 * invalido; nesse caso nao altera a saida. Nao aloca memoria.
 * BFS/DFS poderao percorrer os destinos de 0 a N-1 para obter vizinhos.
 */
ResultadoMatriz consultar_adjacencia(const MatrizAdjacencia *matriz,
                                    size_t vertice_origem,
                                    size_t vertice_destino, int *adjacente);

/* Retorna N, ou zero para NULL. Nao aloca nem transfere propriedade. */
size_t quantidade_vertices_matriz(const MatrizAdjacencia *matriz);

/* Retorna os bytes solicitados para cabecalho e celulas, ou zero para
 * NULL. Tem a mesma abrangencia de estimar_memoria_matriz; nao e RSS.
 * Nao inclui estruturas do chamador e nao aloca memoria.
 */
size_t memoria_matriz(const MatrizAdjacencia *matriz);

/* Libera celulas e cabecalho e define *matriz como NULL. Aceita NULL
 * e *matriz == NULL; repetir a chamada sobre o mesmo dono e seguro.
 * Outros ponteiros para a matriz ficam invalidos e nao devem ser usados.
 */
void destruir_matriz(MatrizAdjacencia **matriz);

#endif
