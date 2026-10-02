#ifndef GRAFO_LISTA_H
#define GRAFO_LISTA_H

#include <stddef.h>

/* Tipos opacos: somente o modulo altera ou libera suas listas. */
typedef struct GrafoLista GrafoLista;
typedef struct VizinhoLista VizinhoLista;

typedef enum {
    LISTA_SUCESSO,
    LISTA_ARGUMENTO_INVALIDO,
    LISTA_SEM_MEMORIA,
    LISTA_ARESTA_DUPLICADA,
    LISTA_LACO_NAO_PERMITIDO,
    LISTA_ARESTA_INEXISTENTE,
    LISTA_LIMITE_EXCEDIDO
} ResultadoLista;

/* Cria um grafo vazio; retorna NULL se faltar memoria.
 * O chamador deve entregar o resultado a liberar_grafo_lista. */
GrafoLista *criar_grafo_lista(void);

/* Insere um vertice isolado e escreve seu indice interno em indice_vertice.
 * Os indices sao consecutivos, estaveis e independentes dos IDs do dataset.
 * Retorna SUCESSO, ARGUMENTO_INVALIDO, SEM_MEMORIA ou LIMITE_EXCEDIDO.
 * Em caso de erro, preserva o grafo e a saida. O grafo possui a memoria. */
ResultadoLista inserir_vertice_lista(GrafoLista *grafo, size_t *indice_vertice);

/* Insere uma unica conexao nao direcionada entre indices internos validos.
 * Rejeita lacos e duplicatas, inclusive (destino, origem). Retorna o erro
 * correspondente do enum; qualquer erro preserva ambas as vizinhancas.
 * Os dois nos alocados pertencem ao grafo. Nao ha pesos/atributos nesta API. */
ResultadoLista inserir_aresta_lista(GrafoLista *grafo, size_t origem,
                                   size_t destino);

/* Remove e libera a conexao em ambos os sentidos, decrementando a contagem
 * uma vez. Retorna SUCESSO, ARGUMENTO_INVALIDO ou ARESTA_INEXISTENTE.
 * Restaure com inserir_aresta_lista; a remocao nao guarda historico. */
ResultadoLista remover_aresta_lista(GrafoLista *grafo, size_t origem,
                                   size_t destino);

/* Escreve 1 se a conexao existe, ou 0 se nao existe (incluindo origem ==
 * destino). Retorna SUCESSO ou ARGUMENTO_INVALIDO; erro preserva a saida. */
ResultadoLista existe_aresta_lista(const GrafoLista *grafo, size_t origem,
                                 size_t destino, int *existe);

/* Inicia consulta em O(1), escrevendo o primeiro vizinho ou NULL para isolado.
 * Retorna SUCESSO ou ARGUMENTO_INVALIDO; erro preserva a saida.
 * A lista e emprestada, sem alocacao/copia, e nao promete ordem. Nao a libere.
 * Por contrato, encerre a iteracao antes de modificar/liberar o grafo. */
ResultadoLista buscar_vizinhos_lista(const GrafoLista *grafo, size_t vertice,
                                    const VizinhoLista **primeiro_vizinho);

/* Retorna o indice de um vizinho, ou SIZE_MAX se vizinho == NULL. */
size_t indice_vizinho_lista(const VizinhoLista *vizinho);

/* Avanca em O(1); retorna NULL no fim ou quando vizinho == NULL. */
const VizinhoLista *proximo_vizinho_lista(const VizinhoLista *vizinho);

/* Retornam as contagens; grafo == NULL resulta em zero. Cada conexao conta
 * uma vez, embora seja armazenada em duas listas. Nao alocam memoria. */
size_t quantidade_vertices_lista(const GrafoLista *grafo);
size_t quantidade_arestas_lista(const GrafoLista *grafo);

/* Libera todos os nos, o vetor e o grafo, inclusive construcao parcial.
 * Aceita NULL. O chamador deve descartar ponteiros/iteradores antigos. */
void liberar_grafo_lista(GrafoLista *grafo);

#endif
