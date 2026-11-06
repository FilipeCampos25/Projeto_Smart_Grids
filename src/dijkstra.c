#include "dijkstra.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    size_t *vertices;
    size_t *posicoes;
    size_t quantidade;
    double *distancias;
} HeapDijkstra;

static int politica_valida(PoliticaPesoDijkstra politica)
{
    return politica == DIJKSTRA_REJEITAR_ARESTA_SEM_PESO ||
           politica == DIJKSTRA_IGNORAR_ARESTA_SEM_PESO;
}

static int multiplicacao_valida(size_t n, size_t tamanho)
{
    return tamanho == 0 || n <= SIZE_MAX / tamanho;
}

static ResultadoDijkstra *criar_resultado(void)
{
    ResultadoDijkstra *resultado = malloc(sizeof(*resultado));
    if (resultado != NULL) {
        resultado->caminho = NULL;
        resultado->quantidade_vertices_caminho = 0;
        resultado->quantidade_vertices_processados = 0;
        resultado->custo_total = 0.0;
        resultado->alcancavel = 0;
        resultado->utiliza_custo_assumido = 0;
    }
    return resultado;
}

static int heap_menor(const HeapDijkstra *heap, size_t a, size_t b)
{
    size_t va = heap->vertices[a];
    size_t vb = heap->vertices[b];
    return heap->distancias[va] < heap->distancias[vb] ||
           (heap->distancias[va] == heap->distancias[vb] && va < vb);
}

static void heap_trocar(HeapDijkstra *heap, size_t a, size_t b)
{
    size_t temporario = heap->vertices[a];
    heap->vertices[a] = heap->vertices[b];
    heap->vertices[b] = temporario;
    heap->posicoes[heap->vertices[a]] = a;
    heap->posicoes[heap->vertices[b]] = b;
}

static void heap_subir(HeapDijkstra *heap, size_t posicao)
{
    while (posicao > 0) {
        size_t pai = (posicao - 1) / 2;
        if (!heap_menor(heap, posicao, pai)) break;
        heap_trocar(heap, posicao, pai);
        posicao = pai;
    }
}

static void heap_inserir_ou_diminuir(HeapDijkstra *heap, size_t vertice)
{
    size_t posicao = heap->posicoes[vertice];
    if (posicao == SIZE_MAX) {
        posicao = heap->quantidade++;
        heap->vertices[posicao] = vertice;
        heap->posicoes[vertice] = posicao;
    }
    heap_subir(heap, posicao);
}

static size_t heap_remover_minimo(HeapDijkstra *heap)
{
    size_t minimo = heap->vertices[0];
    heap->posicoes[minimo] = SIZE_MAX;
    heap->quantidade--;
    if (heap->quantidade != 0) {
        size_t posicao = 0;
        heap->vertices[0] = heap->vertices[heap->quantidade];
        heap->posicoes[heap->vertices[0]] = 0;
        for (;;) {
            size_t esquerda = posicao * 2 + 1;
            size_t direita = esquerda + 1;
            size_t menor = posicao;
            if (esquerda < heap->quantidade && heap_menor(heap, esquerda, menor))
                menor = esquerda;
            if (direita < heap->quantidade && heap_menor(heap, direita, menor))
                menor = direita;
            if (menor == posicao) break;
            heap_trocar(heap, posicao, menor);
            posicao = menor;
        }
    }
    return minimo;
}

static StatusDijkstra validar_pesos_lista(const GrafoLista *grafo,
                                          PoliticaPesoDijkstra politica)
{
    size_t n = quantidade_vertices_lista(grafo);
    if (politica == DIJKSTRA_IGNORAR_ARESTA_SEM_PESO) return DIJKSTRA_SUCESSO;
    for (size_t origem = 0; origem < n; origem++) {
        const VizinhoLista *vizinho = NULL;
        if (buscar_vizinhos_lista(grafo, origem, &vizinho) != LISTA_SUCESSO)
            return DIJKSTRA_ERRO_GRAFO;
        for (; vizinho != NULL; vizinho = proximo_vizinho_lista(vizinho)) {
            PesoAresta peso;
            if (consultar_peso_aresta_lista(grafo, origem,
                    indice_vizinho_lista(vizinho), &peso) != LISTA_SUCESSO)
                return DIJKSTRA_ERRO_GRAFO;
            if (!peso.possui_peso) return DIJKSTRA_ARESTA_SEM_PESO;
        }
    }
    return DIJKSTRA_SUCESSO;
}

static StatusDijkstra validar_pesos_matriz(const MatrizAdjacencia *matriz,
                                           PoliticaPesoDijkstra politica)
{
    size_t n = quantidade_vertices_matriz(matriz);
    if (politica == DIJKSTRA_IGNORAR_ARESTA_SEM_PESO) return DIJKSTRA_SUCESSO;
    for (size_t origem = 0; origem < n; origem++) {
        for (size_t destino = 0; destino < n; destino++) {
            int adjacente;
            PesoAresta peso;
            if (consultar_adjacencia(matriz, origem, destino, &adjacente) !=
                MATRIZ_SUCESSO) return DIJKSTRA_ERRO_GRAFO;
            if (!adjacente) continue;
            if (consultar_peso_aresta(matriz, origem, destino, &peso) !=
                MATRIZ_SUCESSO) return DIJKSTRA_ERRO_GRAFO;
            if (!peso.possui_peso) return DIJKSTRA_ARESTA_SEM_PESO;
        }
    }
    return DIJKSTRA_SUCESSO;
}

static StatusDijkstra relaxar(size_t atual, size_t vizinho,
                              const PesoAresta *peso, double *distancias,
                              size_t *predecessores, unsigned char *visitados,
                              unsigned char *usa_assumido, HeapDijkstra *heap)
{
    double candidato;
    if (!peso->possui_peso || visitados[vizinho]) return DIJKSTRA_SUCESSO;
    if (distancias[atual] > DBL_MAX - peso->valor) return DIJKSTRA_OVERFLOW;
    candidato = distancias[atual] + peso->valor;
    if (!isfinite(candidato)) return DIJKSTRA_OVERFLOW;
    if (candidato < distancias[vizinho] ||
        (candidato == distancias[vizinho] && atual < predecessores[vizinho])) {
        int diminuiu = candidato < distancias[vizinho];
        distancias[vizinho] = candidato;
        predecessores[vizinho] = atual;
        usa_assumido[vizinho] = (unsigned char)(usa_assumido[atual] ||
            peso->origem == ORIGEM_PESO_CUSTO_ASSUMIDO);
        if (heap != NULL && diminuiu) heap_inserir_ou_diminuir(heap, vizinho);
    }
    return DIJKSTRA_SUCESSO;
}

static StatusDijkstra montar_caminho(size_t origem, size_t destino,
                                     const size_t *predecessores,
                                     const double *distancias,
                                     const unsigned char *usa_assumido,
                                     size_t n, ResultadoDijkstra *resultado)
{
    size_t quantidade = 1;
    size_t atual = destino;
    if (distancias[destino] == INFINITY) return DIJKSTRA_SUCESSO;
    while (atual != origem) {
        if (predecessores[atual] == SIZE_MAX || quantidade == n)
            return DIJKSTRA_ERRO_GRAFO;
        atual = predecessores[atual];
        quantidade++;
    }
    if (!multiplicacao_valida(quantidade, sizeof(*resultado->caminho)))
        return DIJKSTRA_SEM_MEMORIA;
    resultado->caminho = malloc(quantidade * sizeof(*resultado->caminho));
    if (resultado->caminho == NULL) return DIJKSTRA_SEM_MEMORIA;
    atual = destino;
    for (size_t i = quantidade; i > 0; i--) {
        resultado->caminho[i - 1] = atual;
        if (atual != origem) atual = predecessores[atual];
    }
    resultado->quantidade_vertices_caminho = quantidade;
    resultado->custo_total = distancias[destino];
    resultado->alcancavel = 1;
    resultado->utiliza_custo_assumido = usa_assumido[destino];
    return DIJKSTRA_SUCESSO;
}

StatusDijkstra executar_dijkstra_lista(
    const GrafoLista *grafo, size_t origem, size_t destino,
    PoliticaPesoDijkstra politica, ResultadoDijkstra **saida)
{
    size_t n = quantidade_vertices_lista(grafo);
    ResultadoDijkstra *resultado = NULL;
    double *distancias = NULL;
    size_t *predecessores = NULL, *vertices_heap = NULL, *posicoes = NULL;
    unsigned char *visitados = NULL, *usa_assumido = NULL;
    HeapDijkstra heap;
    StatusDijkstra status;
    if (grafo == NULL || origem >= n || destino >= n || !politica_valida(politica) ||
        saida == NULL || *saida != NULL) return DIJKSTRA_ARGUMENTO_INVALIDO;
    status = validar_pesos_lista(grafo, politica);
    if (status != DIJKSTRA_SUCESSO) return status;
    if (!multiplicacao_valida(n, sizeof(*distancias)) ||
        !multiplicacao_valida(n, sizeof(*predecessores))) return DIJKSTRA_SEM_MEMORIA;
    resultado = criar_resultado();
    distancias = malloc(n * sizeof(*distancias));
    predecessores = malloc(n * sizeof(*predecessores));
    visitados = calloc(n, sizeof(*visitados));
    usa_assumido = calloc(n, sizeof(*usa_assumido));
    vertices_heap = malloc(n * sizeof(*vertices_heap));
    posicoes = malloc(n * sizeof(*posicoes));
    if (resultado == NULL || distancias == NULL || predecessores == NULL ||
        visitados == NULL || usa_assumido == NULL || vertices_heap == NULL ||
        posicoes == NULL) { status = DIJKSTRA_SEM_MEMORIA; goto fim; }
    for (size_t i = 0; i < n; i++) {
        distancias[i] = INFINITY;
        predecessores[i] = SIZE_MAX;
        posicoes[i] = SIZE_MAX;
    }
    distancias[origem] = 0.0;
    heap = (HeapDijkstra){vertices_heap, posicoes, 0, distancias};
    heap_inserir_ou_diminuir(&heap, origem);
    while (heap.quantidade != 0) {
        size_t atual = heap_remover_minimo(&heap);
        const VizinhoLista *vizinho = NULL;
        visitados[atual] = 1;
        resultado->quantidade_vertices_processados++;
        if (atual == destino) break;
        if (buscar_vizinhos_lista(grafo, atual, &vizinho) != LISTA_SUCESSO) {
            status = DIJKSTRA_ERRO_GRAFO; goto fim;
        }
        for (; vizinho != NULL; vizinho = proximo_vizinho_lista(vizinho)) {
            size_t proximo = indice_vizinho_lista(vizinho);
            PesoAresta peso;
            if (consultar_peso_aresta_lista(grafo, atual, proximo, &peso) !=
                LISTA_SUCESSO) { status = DIJKSTRA_ERRO_GRAFO; goto fim; }
            if (!peso.possui_peso) continue;
            status = relaxar(atual, proximo, &peso, distancias, predecessores,
                             visitados, usa_assumido, &heap);
            if (status != DIJKSTRA_SUCESSO) goto fim;
        }
    }
    status = montar_caminho(origem, destino, predecessores, distancias,
                            usa_assumido, n, resultado);
    if (status == DIJKSTRA_SUCESSO) { *saida = resultado; resultado = NULL; }
fim:
    liberar_resultado_dijkstra(&resultado);
    free(distancias); free(predecessores); free(visitados); free(usa_assumido);
    free(vertices_heap); free(posicoes);
    return status;
}

StatusDijkstra executar_dijkstra_matriz(
    const MatrizAdjacencia *matriz, size_t origem, size_t destino,
    PoliticaPesoDijkstra politica, ResultadoDijkstra **saida)
{
    size_t n = quantidade_vertices_matriz(matriz);
    ResultadoDijkstra *resultado = NULL;
    double *distancias = NULL;
    size_t *predecessores = NULL;
    unsigned char *visitados = NULL, *usa_assumido = NULL;
    StatusDijkstra status;
    if (matriz == NULL || origem >= n || destino >= n || !politica_valida(politica) ||
        saida == NULL || *saida != NULL) return DIJKSTRA_ARGUMENTO_INVALIDO;
    status = validar_pesos_matriz(matriz, politica);
    if (status != DIJKSTRA_SUCESSO) return status;
    if (!multiplicacao_valida(n, sizeof(*distancias)) ||
        !multiplicacao_valida(n, sizeof(*predecessores))) return DIJKSTRA_SEM_MEMORIA;
    resultado = criar_resultado();
    distancias = malloc(n * sizeof(*distancias));
    predecessores = malloc(n * sizeof(*predecessores));
    visitados = calloc(n, sizeof(*visitados));
    usa_assumido = calloc(n, sizeof(*usa_assumido));
    if (resultado == NULL || distancias == NULL || predecessores == NULL ||
        visitados == NULL || usa_assumido == NULL) {
        status = DIJKSTRA_SEM_MEMORIA; goto fim;
    }
    for (size_t i = 0; i < n; i++) {
        distancias[i] = INFINITY;
        predecessores[i] = SIZE_MAX;
    }
    distancias[origem] = 0.0;
    for (size_t iteracao = 0; iteracao < n; iteracao++) {
        size_t atual = SIZE_MAX;
        for (size_t i = 0; i < n; i++) {
            if (!visitados[i] && distancias[i] != INFINITY &&
                (atual == SIZE_MAX || distancias[i] < distancias[atual] ||
                 (distancias[i] == distancias[atual] && i < atual))) atual = i;
        }
        if (atual == SIZE_MAX) break;
        visitados[atual] = 1;
        resultado->quantidade_vertices_processados++;
        if (atual == destino) break;
        for (size_t vizinho = 0; vizinho < n; vizinho++) {
            int adjacente;
            PesoAresta peso;
            if (consultar_adjacencia(matriz, atual, vizinho, &adjacente) !=
                MATRIZ_SUCESSO) { status = DIJKSTRA_ERRO_GRAFO; goto fim; }
            if (!adjacente) continue;
            if (consultar_peso_aresta(matriz, atual, vizinho, &peso) !=
                MATRIZ_SUCESSO) { status = DIJKSTRA_ERRO_GRAFO; goto fim; }
            if (!peso.possui_peso) continue;
            status = relaxar(atual, vizinho, &peso, distancias, predecessores,
                             visitados, usa_assumido, NULL);
            if (status != DIJKSTRA_SUCESSO) goto fim;
        }
    }
    status = montar_caminho(origem, destino, predecessores, distancias,
                            usa_assumido, n, resultado);
    if (status == DIJKSTRA_SUCESSO) { *saida = resultado; resultado = NULL; }
fim:
    liberar_resultado_dijkstra(&resultado);
    free(distancias); free(predecessores); free(visitados); free(usa_assumido);
    return status;
}

void liberar_resultado_dijkstra(ResultadoDijkstra **resultado)
{
    if (resultado == NULL || *resultado == NULL) return;
    free((*resultado)->caminho);
    free(*resultado);
    *resultado = NULL;
}

const char *descricao_status_dijkstra(StatusDijkstra status)
{
    switch (status) {
    case DIJKSTRA_SUCESSO: return "sucesso";
    case DIJKSTRA_ARGUMENTO_INVALIDO: return "argumento ou vertice invalido";
    case DIJKSTRA_SEM_MEMORIA: return "memoria insuficiente";
    case DIJKSTRA_ARESTA_SEM_PESO: return "grafo contem aresta sem peso";
    case DIJKSTRA_OVERFLOW: return "overflow na soma dos pesos";
    case DIJKSTRA_ERRO_GRAFO: return "inconsistencia na representacao do grafo";
    default: return "status desconhecido";
    }
}
