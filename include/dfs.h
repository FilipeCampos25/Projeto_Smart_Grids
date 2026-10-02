#ifndef DFS_H
#define DFS_H

#include "grafo_lista.h"
#include "matriz_adjacencia.h"

typedef enum {
    DFS_SUCESSO = 0,
    DFS_ARGUMENTO_INVALIDO,
    DFS_SEM_MEMORIA,
    DFS_LIMITE_EXCEDIDO
} ResultadoDFS;

/* Resultado independente do grafo, com indices internos (nao IDs da BDGD).
 * visitados tem quantidade_vertices posicoes: 1 se alcancado, 0 caso contrario.
 * ordem tem capacidade quantidade_vertices; somente as primeiras
 * quantidade_alcancados posicoes estao preenchidas, em ordem de descoberta.
 * O chamador pode ler os campos, mas nao deve alterar os ponteiros/contagens
 * nem liberar os vetores separadamente: use liberar_percurso_dfs.
 */
typedef struct {
    size_t quantidade_vertices;
    size_t quantidade_alcancados;
    unsigned char *visitados;
    size_t *ordem;
} PercursoDFS;

/* Percorre somente os vertices alcancaveis de origem pelas conexoes presentes.
 * Usa a iteracao natural de buscar_vizinhos_lista, sem ordenar os vizinhos.
 * grafo deve permanecer valido e sem modificacoes durante a chamada.
 * origem deve estar em [0, V); grafo NULL/vazio e origem invalida retornam
 * ARGUMENTO_INVALIDO. percurso deve apontar para um ponteiro iniciado em NULL;
 * um dono ocupado ou saida NULL tambem retorna ARGUMENTO_INVALIDO.
 * Retorna SEM_MEMORIA se uma alocacao falhar, LIMITE_EXCEDIDO se um tamanho
 * nao couber em size_t, ou SUCESSO. Em erro preserva grafo e *percurso,
 * liberando qualquer construcao parcial. Em sucesso, o chamador possui o
 * resultado e deve libera-lo com liberar_percurso_dfs, mesmo se liberar antes
 * o grafo. Pilha no heap, sem recursao: tempo O(V+E), memoria adicional O(V).
 */
ResultadoDFS executar_dfs_lista(const GrafoLista *grafo, size_t origem,
                                PercursoDFS **percurso);

/* Mesmo contrato de entradas, erros e propriedade de executar_dfs_lista.
 * Consulta vizinhos em ordem crescente de indice com consultar_adjacencia.
 * Conexoes removidas sao ignoradas; lacos nao repetem a visita. Nao altera
 * a matriz. Pilha no heap, sem recursao: tempo O(V^2), memoria adicional O(V).
 */
ResultadoDFS executar_dfs_matriz(const MatrizAdjacencia *matriz, size_t origem,
                                 PercursoDFS **percurso);

/* Libera os vetores e o resultado, definindo *percurso como NULL.
 * Aceita NULL e *percurso == NULL; pode ser repetida sobre o mesmo dono.
 * Copias antigas do ponteiro ficam invalidas. Nao acessa nem libera o grafo.
 */
void liberar_percurso_dfs(PercursoDFS **percurso);

#endif
