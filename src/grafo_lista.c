#include "grafo_lista.h"

#include <stdint.h>
#include <stdlib.h>

/* Cada conexao possui dois nos, um em cada extremidade. */
struct VizinhoLista {
    size_t indice;
    struct VizinhoLista *proximo;
};

struct GrafoLista {
    VizinhoLista **vizinhos;
    size_t quantidade_vertices;
    size_t quantidade_arestas;
    size_t capacidade_vertices;
};

/* Valida o grafo e os dois indices sem acessar memoria fora do vetor. */
static int extremos_validos(const GrafoLista *grafo, size_t origem,
                           size_t destino)
{
    return grafo != NULL && origem < grafo->quantidade_vertices &&
           destino < grafo->quantidade_vertices;
}

/* Busca um indice na lista fornecida; devolve um no emprestado ou NULL.
 * Nao aloca nem modifica a lista. Custo proporcional ao grau. */
static const VizinhoLista *encontrar_vizinho(const VizinhoLista *vizinho,
                                           size_t destino)
{
    while (vizinho != NULL && vizinho->indice != destino) {
        vizinho = vizinho->proximo;
    }
    return vizinho;
}

/* Amplia o vetor quando necessario. Checa o limite antes de multiplicar.
 * realloc malsucedido preserva o vetor antigo; a memoria pertence ao grafo. */
static ResultadoLista reservar_vertice(GrafoLista *grafo)
{
    const size_t capacidade_maxima = SIZE_MAX / sizeof(*grafo->vizinhos);
    size_t nova_capacidade;
    VizinhoLista **novas_listas;

    if (grafo->quantidade_vertices < grafo->capacidade_vertices) {
        return LISTA_SUCESSO;
    }
    if (grafo->capacidade_vertices == capacidade_maxima) {
        return LISTA_LIMITE_EXCEDIDO;
    }
    if (grafo->capacidade_vertices == 0) {
        nova_capacidade = capacidade_maxima < 8 ? capacidade_maxima : 8;
    } else if (grafo->capacidade_vertices > capacidade_maxima / 2) {
        nova_capacidade = capacidade_maxima;
    } else {
        nova_capacidade = grafo->capacidade_vertices * 2;
    }
    novas_listas = realloc(grafo->vizinhos,
                           nova_capacidade * sizeof(*novas_listas));
    if (novas_listas == NULL) {
        return LISTA_SEM_MEMORIA;
    }
    grafo->vizinhos = novas_listas;
    grafo->capacidade_vertices = nova_capacidade;
    return LISTA_SUCESSO;
}

GrafoLista *criar_grafo_lista(void)
{
    GrafoLista *grafo = malloc(sizeof(*grafo));
    if (grafo != NULL) {
        grafo->vizinhos = NULL;
        grafo->quantidade_vertices = 0;
        grafo->quantidade_arestas = 0;
        grafo->capacidade_vertices = 0;
    }
    return grafo;
}

ResultadoLista inserir_vertice_lista(GrafoLista *grafo, size_t *indice_vertice)
{
    ResultadoLista resultado;
    if (grafo == NULL || indice_vertice == NULL) {
        return LISTA_ARGUMENTO_INVALIDO;
    }
    resultado = reservar_vertice(grafo);
    if (resultado != LISTA_SUCESSO) {
        return resultado;
    }
    grafo->vizinhos[grafo->quantidade_vertices] = NULL;
    *indice_vertice = grafo->quantidade_vertices;
    grafo->quantidade_vertices++;
    return LISTA_SUCESSO;
}

ResultadoLista inserir_aresta_lista(GrafoLista *grafo, size_t origem,
                                   size_t destino)
{
    VizinhoLista *vizinho_origem;
    VizinhoLista *vizinho_destino;

    if (!extremos_validos(grafo, origem, destino)) {
        return LISTA_ARGUMENTO_INVALIDO;
    }
    if (origem == destino) {
        return LISTA_LACO_NAO_PERMITIDO;
    }
    if (encontrar_vizinho(grafo->vizinhos[origem], destino) != NULL) {
        return LISTA_ARESTA_DUPLICADA;
    }
    if (grafo->quantidade_arestas == SIZE_MAX) {
        return LISTA_LIMITE_EXCEDIDO;
    }
    vizinho_origem = malloc(sizeof(*vizinho_origem));
    if (vizinho_origem == NULL) {
        return LISTA_SEM_MEMORIA;
    }
    vizinho_destino = malloc(sizeof(*vizinho_destino));
    if (vizinho_destino == NULL) {
        free(vizinho_origem);
        return LISTA_SEM_MEMORIA;
    }

    /* So publica a conexao depois de obter memoria para os dois sentidos. */
    vizinho_origem->indice = destino;
    vizinho_origem->proximo = grafo->vizinhos[origem];
    vizinho_destino->indice = origem;
    vizinho_destino->proximo = grafo->vizinhos[destino];
    grafo->vizinhos[origem] = vizinho_origem;
    grafo->vizinhos[destino] = vizinho_destino;
    grafo->quantidade_arestas++;
    return LISTA_SUCESSO;
}

/* Retorna o endereco do elo que aponta para destino, ou para o NULL final.
 * Isso permite remover tanto a cabeca quanto um no interno da lista. */
static VizinhoLista **encontrar_elo(VizinhoLista **elo, size_t destino)
{
    while (*elo != NULL && (*elo)->indice != destino) {
        elo = &(*elo)->proximo;
    }
    return elo;
}

ResultadoLista remover_aresta_lista(GrafoLista *grafo, size_t origem,
                                   size_t destino)
{
    VizinhoLista **elo_origem;
    VizinhoLista **elo_destino;
    VizinhoLista *no_origem;
    VizinhoLista *no_destino;

    if (!extremos_validos(grafo, origem, destino)) {
        return LISTA_ARGUMENTO_INVALIDO;
    }
    elo_origem = encontrar_elo(&grafo->vizinhos[origem], destino);
    elo_destino = encontrar_elo(&grafo->vizinhos[destino], origem);
    if (*elo_origem == NULL || *elo_destino == NULL) {
        return LISTA_ARESTA_INEXISTENTE;
    }
    no_origem = *elo_origem;
    no_destino = *elo_destino;
    *elo_origem = no_origem->proximo;
    *elo_destino = no_destino->proximo;
    free(no_origem);
    free(no_destino);
    grafo->quantidade_arestas--;
    return LISTA_SUCESSO;
}

ResultadoLista existe_aresta_lista(const GrafoLista *grafo, size_t origem,
                                 size_t destino, int *existe)
{
    if (!extremos_validos(grafo, origem, destino) || existe == NULL) {
        return LISTA_ARGUMENTO_INVALIDO;
    }
    *existe = encontrar_vizinho(grafo->vizinhos[origem], destino) != NULL;
    return LISTA_SUCESSO;
}

ResultadoLista buscar_vizinhos_lista(const GrafoLista *grafo, size_t vertice,
                                    const VizinhoLista **primeiro_vizinho)
{
    if (grafo == NULL || vertice >= grafo->quantidade_vertices ||
        primeiro_vizinho == NULL) {
        return LISTA_ARGUMENTO_INVALIDO;
    }
    *primeiro_vizinho = grafo->vizinhos[vertice];
    return LISTA_SUCESSO;
}

size_t indice_vizinho_lista(const VizinhoLista *vizinho)
{
    return vizinho == NULL ? SIZE_MAX : vizinho->indice;
}

const VizinhoLista *proximo_vizinho_lista(const VizinhoLista *vizinho)
{
    return vizinho == NULL ? NULL : vizinho->proximo;
}

size_t quantidade_vertices_lista(const GrafoLista *grafo)
{
    return grafo == NULL ? 0 : grafo->quantidade_vertices;
}

size_t quantidade_arestas_lista(const GrafoLista *grafo)
{
    return grafo == NULL ? 0 : grafo->quantidade_arestas;
}

size_t memoria_lista(const GrafoLista *grafo)
{
    if (grafo == NULL) {
        return 0;
    }
    return sizeof(*grafo) +
           grafo->capacidade_vertices * sizeof(*grafo->vizinhos) +
           2 * grafo->quantidade_arestas * sizeof(VizinhoLista);
}

void liberar_grafo_lista(GrafoLista *grafo)
{
    if (grafo == NULL) {
        return;
    }
    for (size_t vertice = 0; vertice < grafo->quantidade_vertices; vertice++) {
        VizinhoLista *vizinho = grafo->vizinhos[vertice];
        while (vizinho != NULL) {
            VizinhoLista *proximo = vizinho->proximo;
            free(vizinho);
            vizinho = proximo;
        }
    }
    free(grafo->vizinhos);
    free(grafo);
}
