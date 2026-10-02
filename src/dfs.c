#include "dfs.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Uma chamada suspensa da DFS na matriz: de qual vertice e de qual coluna
 * continuar depois de explorar um filho. A lista usa diretamente iteradores.
 */
typedef struct {
    size_t vertice;
    size_t proximo_vizinho;
} QuadroMatriz;

void liberar_percurso_dfs(PercursoDFS **percurso)
{
    if (percurso != NULL && *percurso != NULL) {
        free((*percurso)->visitados);
        free((*percurso)->ordem);
        free(*percurso);
        *percurso = NULL;
    }
}

/* Reserva resultado e vetores para V > 0, com visitados zerados.
 * Retorna erro de tamanho/memoria e preserva a saida em falha. O chamador
 * interno deve liberar o resultado ou transferi-lo ao usuario em sucesso.
 */
static ResultadoDFS criar_percurso(size_t quantidade_vertices,
                                   PercursoDFS **percurso)
{
    PercursoDFS *novo_percurso;

    if (quantidade_vertices > SIZE_MAX / sizeof(size_t)) {
        return DFS_LIMITE_EXCEDIDO;
    }
    novo_percurso = malloc(sizeof(*novo_percurso));
    if (novo_percurso == NULL) {
        return DFS_SEM_MEMORIA;
    }
    *novo_percurso = (PercursoDFS){0};
    novo_percurso->visitados = malloc(quantidade_vertices);
    novo_percurso->ordem = malloc(quantidade_vertices * sizeof(size_t));
    if (novo_percurso->visitados == NULL || novo_percurso->ordem == NULL) {
        liberar_percurso_dfs(&novo_percurso);
        return DFS_SEM_MEMORIA;
    }
    memset(novo_percurso->visitados, 0, quantidade_vertices);
    novo_percurso->quantidade_vertices = quantidade_vertices;
    *percurso = novo_percurso;
    return DFS_SUCESSO;
}

/* Registra uma descoberta unica. O chamador garante indice valido e ainda
 * nao visitado; assim quantidade_alcancados nunca ultrapassa V. Nao aloca.
 */
static void registrar_descoberta(PercursoDFS *percurso, size_t vertice)
{
    percurso->visitados[vertice] = 1;
    percurso->ordem[percurso->quantidade_alcancados++] = vertice;
}

ResultadoDFS executar_dfs_lista(const GrafoLista *grafo, size_t origem,
                                PercursoDFS **percurso)
{
    const size_t quantidade_vertices = quantidade_vertices_lista(grafo);
    const VizinhoLista **pilha;
    PercursoDFS *novo_percurso = NULL;
    ResultadoDFS resultado;
    size_t profundidade = 1;

    if (grafo == NULL || origem >= quantidade_vertices ||
        percurso == NULL || *percurso != NULL) {
        return DFS_ARGUMENTO_INVALIDO;
    }
    if (quantidade_vertices > SIZE_MAX / sizeof(*pilha)) {
        return DFS_LIMITE_EXCEDIDO;
    }
    resultado = criar_percurso(quantidade_vertices, &novo_percurso);
    if (resultado != DFS_SUCESSO) {
        return resultado;
    }
    pilha = malloc(quantidade_vertices * sizeof(*pilha));
    if (pilha == NULL) {
        liberar_percurso_dfs(&novo_percurso);
        return DFS_SEM_MEMORIA;
    }

    registrar_descoberta(novo_percurso, origem);
    /* Os indices sao validos por construcao e o grafo nao muda na travessia;
     * as consultas abaixo nao alocam e satisfazem o contrato da lista.
     */
    buscar_vizinhos_lista(grafo, origem, &pilha[0]);
    while (profundidade != 0) {
        const VizinhoLista *vizinho = pilha[profundidade - 1];
        size_t vertice_atual;

        if (vizinho == NULL) {
            profundidade--; /* Terminou o ramo: retorna ao pai. */
            continue;
        }
        /* Salva a continuacao antes de descer: cada elo e examinado uma vez. */
        pilha[profundidade - 1] = proximo_vizinho_lista(vizinho);
        vertice_atual = indice_vizinho_lista(vizinho);
        if (novo_percurso->visitados[vertice_atual]) {
            continue;
        }
        registrar_descoberta(novo_percurso, vertice_atual);
        buscar_vizinhos_lista(grafo, vertice_atual, &pilha[profundidade]);
        profundidade++;
    }
    free(pilha);
    *percurso = novo_percurso;
    return DFS_SUCESSO;
}

ResultadoDFS executar_dfs_matriz(const MatrizAdjacencia *matriz, size_t origem,
                                 PercursoDFS **percurso)
{
    const size_t quantidade_vertices = quantidade_vertices_matriz(matriz);
    QuadroMatriz *pilha;
    PercursoDFS *novo_percurso = NULL;
    ResultadoDFS resultado;
    size_t profundidade = 1;

    if (matriz == NULL || origem >= quantidade_vertices ||
        percurso == NULL || *percurso != NULL) {
        return DFS_ARGUMENTO_INVALIDO;
    }
    if (quantidade_vertices > SIZE_MAX / sizeof(*pilha)) {
        return DFS_LIMITE_EXCEDIDO;
    }
    resultado = criar_percurso(quantidade_vertices, &novo_percurso);
    if (resultado != DFS_SUCESSO) {
        return resultado;
    }
    pilha = malloc(quantidade_vertices * sizeof(*pilha));
    if (pilha == NULL) {
        liberar_percurso_dfs(&novo_percurso);
        return DFS_SEM_MEMORIA;
    }

    registrar_descoberta(novo_percurso, origem);
    pilha[0] = (QuadroMatriz){origem, 0};
    while (profundidade != 0) {
        QuadroMatriz *quadro = &pilha[profundidade - 1];
        size_t vertice_atual;
        int adjacente = 0;

        if (quadro->proximo_vizinho == quantidade_vertices) {
            profundidade--;
            continue;
        }
        vertice_atual = quadro->proximo_vizinho++;
        /* Ambos os indices sao validos; consulta O(1), sem alocacao. */
        consultar_adjacencia(matriz, quadro->vertice, vertice_atual, &adjacente);
        if (!adjacente || novo_percurso->visitados[vertice_atual]) {
            continue;
        }
        registrar_descoberta(novo_percurso, vertice_atual);
        pilha[profundidade++] = (QuadroMatriz){vertice_atual, 0};
    }
    free(pilha);
    *percurso = novo_percurso;
    return DFS_SUCESSO;
}
