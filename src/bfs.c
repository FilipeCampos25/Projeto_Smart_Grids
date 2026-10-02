#include "bfs_interno.h"

#include <stdlib.h>

void liberar_resultado_bfs(ResultadoBfs **resultado)
{
    if (resultado != NULL && *resultado != NULL) {
        free((*resultado)->ordem_visita);
        free((*resultado)->distancias);
        free(*resultado);
        *resultado = NULL;
    }
}

/* Reserva o resultado e seus dois vetores para V > 0 ja validado.
 * Inicializa distancias como nao visitadas e a fila vazia. Em erro, libera
 * a construcao parcial e preserva a saida; em sucesso transfere a posse.
 * A multiplicacao e verificada antes de qualquer pedido de memoria. */
StatusBfs criar_busca_bfs(size_t quantidade_vertices, ResultadoBfs **resultado)
{
    ResultadoBfs *novo_resultado;
    if (quantidade_vertices > SIZE_MAX / sizeof(size_t)) {
        return BFS_LIMITE_EXCEDIDO;
    }
    novo_resultado = malloc(sizeof(*novo_resultado));
    if (novo_resultado == NULL) {
        return BFS_SEM_MEMORIA;
    }
    novo_resultado->quantidade_vertices = quantidade_vertices;
    novo_resultado->quantidade_alcancados = 0;
    novo_resultado->ordem_visita = NULL;
    novo_resultado->distancias = NULL;
    novo_resultado->ordem_visita = malloc(quantidade_vertices * sizeof(size_t));
    if (novo_resultado->ordem_visita == NULL) {
        liberar_resultado_bfs(&novo_resultado);
        return BFS_SEM_MEMORIA;
    }
    novo_resultado->distancias = malloc(quantidade_vertices * sizeof(size_t));
    if (novo_resultado->distancias == NULL) {
        liberar_resultado_bfs(&novo_resultado);
        return BFS_SEM_MEMORIA;
    }
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        novo_resultado->distancias[vertice] = SIZE_MAX;
    }
    *resultado = novo_resultado;
    return BFS_SUCESSO;
}

/* Marca e enfileira um indice valido AINDA NAO visitado, sem alocar.
 * A fila FIFO e o proprio vetor ordem_visita: seu fim e quantidade_alcancados.
 * Cada vertice entra uma vez; logo a capacidade de V posicoes e suficiente.
 * Manter os itens retirados permite devolver a ordem sem outra copia. */
static void enfileirar_vertice(ResultadoBfs *busca, size_t vertice, size_t distancia)
{
    busca->distancias[vertice] = distancia;
    busca->ordem_visita[busca->quantidade_alcancados] = vertice;
    busca->quantidade_alcancados++;
}

StatusBfs continuar_bfs_lista(const GrafoLista *grafo, size_t origem,
                             ResultadoBfs *busca)
{
    size_t inicio_fila = busca->quantidade_alcancados;
    enfileirar_vertice(busca, origem, 0);
    while (inicio_fila < busca->quantidade_alcancados) {
        size_t vertice_atual = busca->ordem_visita[inicio_fila++];
        const VizinhoLista *vizinho = NULL;
        if (buscar_vizinhos_lista(grafo, vertice_atual, &vizinho) != LISTA_SUCESSO) {
            return BFS_ARGUMENTO_INVALIDO;
        }
        for (; vizinho != NULL; vizinho = proximo_vizinho_lista(vizinho)) {
            size_t destino = indice_vizinho_lista(vizinho);
            if (busca->distancias[destino] == SIZE_MAX) {
                enfileirar_vertice(busca, destino, busca->distancias[vertice_atual] + 1);
            }
        }
    }
    return BFS_SUCESSO;
}

StatusBfs continuar_bfs_matriz(const MatrizAdjacencia *matriz, size_t origem,
                              ResultadoBfs *busca)
{
    size_t quantidade_vertices = quantidade_vertices_matriz(matriz);
    size_t inicio_fila = busca->quantidade_alcancados;
    enfileirar_vertice(busca, origem, 0);
    while (inicio_fila < busca->quantidade_alcancados) {
        size_t vertice_atual = busca->ordem_visita[inicio_fila++];
        for (size_t destino = 0; destino < quantidade_vertices; destino++) {
            int adjacente = 0;
            if (consultar_adjacencia(matriz, vertice_atual, destino, &adjacente)
                != MATRIZ_SUCESSO) {
                return BFS_ARGUMENTO_INVALIDO;
            }
            if (adjacente && busca->distancias[destino] == SIZE_MAX) {
                enfileirar_vertice(busca, destino, busca->distancias[vertice_atual] + 1);
            }
        }
    }
    return BFS_SUCESSO;
}

StatusBfs executar_bfs_lista(const GrafoLista *grafo, size_t origem,
                            ResultadoBfs **resultado)
{
    size_t quantidade_vertices = quantidade_vertices_lista(grafo);
    ResultadoBfs *busca = NULL;
    StatusBfs status;

    if (grafo == NULL || origem >= quantidade_vertices ||
        resultado == NULL || *resultado != NULL) {
        return BFS_ARGUMENTO_INVALIDO;
    }
    status = criar_busca_bfs(quantidade_vertices, &busca);
    if (status != BFS_SUCESSO) {
        return status;
    }
    status = continuar_bfs_lista(grafo, origem, busca);
    if (status != BFS_SUCESSO) {
        liberar_resultado_bfs(&busca);
        return status;
    }
    *resultado = busca;
    return BFS_SUCESSO;
}

StatusBfs executar_bfs_matriz(const MatrizAdjacencia *matriz, size_t origem,
                             ResultadoBfs **resultado)
{
    size_t quantidade_vertices = quantidade_vertices_matriz(matriz);
    ResultadoBfs *busca = NULL;
    StatusBfs status;

    if (matriz == NULL || origem >= quantidade_vertices ||
        resultado == NULL || *resultado != NULL) {
        return BFS_ARGUMENTO_INVALIDO;
    }
    status = criar_busca_bfs(quantidade_vertices, &busca);
    if (status != BFS_SUCESSO) {
        return status;
    }
    status = continuar_bfs_matriz(matriz, origem, busca);
    if (status != BFS_SUCESSO) {
        liberar_resultado_bfs(&busca);
        return status;
    }
    *resultado = busca;
    return BFS_SUCESSO;
}
