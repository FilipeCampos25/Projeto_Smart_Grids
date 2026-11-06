#include "agm.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    size_t origem;
    size_t destino;
    PesoAresta peso;
} ArestaInterna;

typedef struct {
    size_t vertices;
    ArestaInterna *arestas;
    size_t quantidade_arestas;
} GrafoInterno;

typedef struct {
    size_t *pai;
    size_t *rank;
} UnionFind;

typedef struct {
    ArestaAGM *arestas;
    size_t quantidade_arestas;
    double custo_total;
    size_t quantidade_componentes;
} ConstrucaoResultado;

static void liberar_grafo_interno(GrafoInterno *grafo)
{
    free(grafo->arestas);
    grafo->arestas = NULL;
    grafo->quantidade_arestas = 0;
}

static StatusAGM validar_aresta(size_t vertices, size_t origem,
                                size_t destino, PesoAresta peso)
{
    if (origem >= vertices || destino >= vertices || origem == destino) {
        return AGM_ARGUMENTO_INVALIDO;
    }
    if (!peso.possui_peso) {
        return AGM_PESO_AUSENTE;
    }
    if (!isfinite(peso.valor) || peso.valor < 0.0 ||
        (peso.origem != ORIGEM_PESO_COMPRIMENTO_MEDIDO &&
         peso.origem != ORIGEM_PESO_CUSTO_ASSUMIDO)) {
        return AGM_PESO_INVALIDO;
    }
    return AGM_SUCESSO;
}

static StatusAGM alocar_arestas(GrafoInterno *grafo, size_t quantidade)
{
    if (quantidade > SIZE_MAX / sizeof(*grafo->arestas)) {
        return AGM_LIMITE_EXCEDIDO;
    }
    if (quantidade == 0) {
        grafo->arestas = NULL;
        return AGM_SUCESSO;
    }
    grafo->arestas = malloc(quantidade * sizeof(*grafo->arestas));
    if (grafo->arestas == NULL) {
        return AGM_SEM_MEMORIA;
    }
    return AGM_SUCESSO;
}

static StatusAGM carregar_lista(const GrafoLista *lista, GrafoInterno *grafo)
{
    size_t quantidade = quantidade_arestas_lista(lista);
    size_t proxima_aresta = 0;
    StatusAGM status;

    grafo->vertices = quantidade_vertices_lista(lista);
    grafo->quantidade_arestas = 0;
    grafo->arestas = NULL;
    status = alocar_arestas(grafo, quantidade);
    if (status != AGM_SUCESSO) {
        return status;
    }

    for (size_t origem = 0; origem < grafo->vertices; origem++) {
        const VizinhoLista *vizinho = NULL;
        if (buscar_vizinhos_lista(lista, origem, &vizinho) != LISTA_SUCESSO) {
            liberar_grafo_interno(grafo);
            return AGM_ARGUMENTO_INVALIDO;
        }
        for (; vizinho != NULL; vizinho = proximo_vizinho_lista(vizinho)) {
            size_t destino = indice_vizinho_lista(vizinho);
            PesoAresta peso = peso_vizinho_lista(vizinho);
            if (destino >= grafo->vertices) {
                liberar_grafo_interno(grafo);
                return AGM_ARGUMENTO_INVALIDO;
            }
            if (origem >= destino) {
                continue;
            }
            status = validar_aresta(grafo->vertices, origem, destino, peso);
            if (status != AGM_SUCESSO) {
                liberar_grafo_interno(grafo);
                return status;
            }
            if (proxima_aresta >= quantidade) {
                liberar_grafo_interno(grafo);
                return AGM_ARGUMENTO_INVALIDO;
            }
            grafo->arestas[proxima_aresta++] =
                (ArestaInterna){origem, destino, peso};
        }
    }
    if (proxima_aresta != quantidade) {
        liberar_grafo_interno(grafo);
        return AGM_ARGUMENTO_INVALIDO;
    }
    grafo->quantidade_arestas = quantidade;
    return AGM_SUCESSO;
}

static StatusAGM contar_arestas_matriz(const MatrizAdjacencia *matriz,
                                       size_t vertices, size_t *quantidade)
{
    size_t total = 0;
    for (size_t origem = 0; origem < vertices; origem++) {
        for (size_t destino = origem + 1; destino < vertices; destino++) {
            int adjacente = 0;
            if (consultar_adjacencia(matriz, origem, destino, &adjacente) !=
                MATRIZ_SUCESSO) {
                return AGM_ARGUMENTO_INVALIDO;
            }
            if (adjacente) {
                if (total == SIZE_MAX) {
                    return AGM_LIMITE_EXCEDIDO;
                }
                total++;
            }
        }
    }
    *quantidade = total;
    return AGM_SUCESSO;
}

static StatusAGM carregar_matriz(const MatrizAdjacencia *matriz,
                                 GrafoInterno *grafo)
{
    size_t proxima_aresta = 0;
    StatusAGM status;

    if (matriz_e_direcionada(matriz)) {
        return AGM_GRAFO_DIRECIONADO;
    }
    grafo->vertices = quantidade_vertices_matriz(matriz);
    grafo->arestas = NULL;
    grafo->quantidade_arestas = 0;
    status = contar_arestas_matriz(matriz, grafo->vertices,
                                   &grafo->quantidade_arestas);
    if (status != AGM_SUCESSO) {
        return status;
    }
    status = alocar_arestas(grafo, grafo->quantidade_arestas);
    if (status != AGM_SUCESSO) {
        grafo->quantidade_arestas = 0;
        return status;
    }

    for (size_t origem = 0; origem < grafo->vertices; origem++) {
        for (size_t destino = origem + 1; destino < grafo->vertices; destino++) {
            int adjacente = 0;
            PesoAresta peso;
            if (consultar_adjacencia(matriz, origem, destino, &adjacente) !=
                MATRIZ_SUCESSO) {
                liberar_grafo_interno(grafo);
                return AGM_ARGUMENTO_INVALIDO;
            }
            if (!adjacente) {
                continue;
            }
            if (consultar_peso_aresta(matriz, origem, destino, &peso) !=
                MATRIZ_SUCESSO) {
                liberar_grafo_interno(grafo);
                return AGM_ARGUMENTO_INVALIDO;
            }
            status = validar_aresta(grafo->vertices, origem, destino, peso);
            if (status != AGM_SUCESSO) {
                liberar_grafo_interno(grafo);
                return status;
            }
            if (proxima_aresta >= grafo->quantidade_arestas) {
                liberar_grafo_interno(grafo);
                return AGM_ARGUMENTO_INVALIDO;
            }
            grafo->arestas[proxima_aresta++] =
                (ArestaInterna){origem, destino, peso};
        }
    }
    if (proxima_aresta != grafo->quantidade_arestas) {
        liberar_grafo_interno(grafo);
        return AGM_ARGUMENTO_INVALIDO;
    }
    return AGM_SUCESSO;
}

static int comparar_arestas(const void *primeiro, const void *segundo)
{
    const ArestaInterna *a = primeiro;
    const ArestaInterna *b = segundo;
    if (a->peso.valor < b->peso.valor) return -1;
    if (a->peso.valor > b->peso.valor) return 1;
    if (a->origem < b->origem) return -1;
    if (a->origem > b->origem) return 1;
    if (a->destino < b->destino) return -1;
    if (a->destino > b->destino) return 1;
    return 0;
}

static void trocar_arestas(ArestaInterna *arestas, size_t primeiro,
                          size_t segundo)
{
    ArestaInterna temporaria = arestas[primeiro];
    arestas[primeiro] = arestas[segundo];
    arestas[segundo] = temporaria;
}

static void descer_monticulo(ArestaInterna *arestas, size_t raiz,
                             size_t quantidade)
{
    while (raiz < quantidade / 2) {
        size_t filho = raiz * 2 + 1;
        if (filho + 1 < quantidade &&
            comparar_arestas(&arestas[filho], &arestas[filho + 1]) < 0) {
            filho++;
        }
        if (comparar_arestas(&arestas[raiz], &arestas[filho]) >= 0) {
            return;
        }
        trocar_arestas(arestas, raiz, filho);
        raiz = filho;
    }
}

static void ordenar_arestas(ArestaInterna *arestas, size_t quantidade)
{
    for (size_t inicio = quantidade / 2; inicio > 0; inicio--) {
        descer_monticulo(arestas, inicio - 1, quantidade);
    }
    for (size_t fim = quantidade; fim > 1; fim--) {
        trocar_arestas(arestas, 0, fim - 1);
        descer_monticulo(arestas, 0, fim - 1);
    }
}

static size_t encontrar_raiz(UnionFind *union_find, size_t vertice)
{
    size_t raiz = vertice;
    while (union_find->pai[raiz] != raiz) {
        raiz = union_find->pai[raiz];
    }
    while (union_find->pai[vertice] != vertice) {
        size_t proximo = union_find->pai[vertice];
        union_find->pai[vertice] = raiz;
        vertice = proximo;
    }
    return raiz;
}

static int unir(UnionFind *union_find, size_t primeiro, size_t segundo)
{
    size_t raiz_a = encontrar_raiz(union_find, primeiro);
    size_t raiz_b = encontrar_raiz(union_find, segundo);
    if (raiz_a == raiz_b) {
        return 0;
    }
    if (union_find->rank[raiz_a] < union_find->rank[raiz_b]) {
        union_find->pai[raiz_a] = raiz_b;
    } else if (union_find->rank[raiz_a] > union_find->rank[raiz_b]) {
        union_find->pai[raiz_b] = raiz_a;
    } else {
        union_find->pai[raiz_b] = raiz_a;
        union_find->rank[raiz_a]++;
    }
    return 1;
}

static StatusAGM somar_custo(double atual, double peso, double *soma)
{
    if (!isfinite(atual) || !isfinite(peso) || peso < 0.0 ||
        peso > DBL_MAX - atual) {
        return AGM_OVERFLOW_CUSTO;
    }
    *soma = atual + peso;
    return isfinite(*soma) ? AGM_SUCESSO : AGM_OVERFLOW_CUSTO;
}

static StatusAGM alocar_saida(size_t vertices, ConstrucaoResultado *saida)
{
    size_t capacidade = vertices == 0 ? 0 : vertices - 1;
    if (capacidade > SIZE_MAX / sizeof(*saida->arestas)) {
        return AGM_LIMITE_EXCEDIDO;
    }
    saida->arestas = capacidade == 0 ? NULL :
        malloc(capacidade * sizeof(*saida->arestas));
    if (capacidade != 0 && saida->arestas == NULL) {
        return AGM_SEM_MEMORIA;
    }
    saida->quantidade_arestas = 0;
    saida->custo_total = 0.0;
    saida->quantidade_componentes = 0;
    return AGM_SUCESSO;
}

static void liberar_saida(ConstrucaoResultado *saida)
{
    free(saida->arestas);
    saida->arestas = NULL;
}

static StatusAGM executar_kruskal(GrafoInterno *grafo,
                                  ConstrucaoResultado *saida)
{
    UnionFind union_find = {NULL, NULL};
    size_t vertices = grafo->vertices;
    StatusAGM status = alocar_saida(vertices, saida);

    if (status != AGM_SUCESSO) {
        return status;
    }
    if (vertices != 0) {
        if (vertices > SIZE_MAX / sizeof(*union_find.pai) ||
            vertices > SIZE_MAX / sizeof(*union_find.rank)) {
            liberar_saida(saida);
            return AGM_LIMITE_EXCEDIDO;
        }
        union_find.pai = malloc(vertices * sizeof(*union_find.pai));
        union_find.rank = malloc(vertices * sizeof(*union_find.rank));
        if (union_find.pai == NULL || union_find.rank == NULL) {
            free(union_find.pai);
            free(union_find.rank);
            liberar_saida(saida);
            return AGM_SEM_MEMORIA;
        }
        for (size_t vertice = 0; vertice < vertices; vertice++) {
            union_find.pai[vertice] = vertice;
            union_find.rank[vertice] = 0;
        }
    }

    saida->quantidade_componentes = vertices;
    ordenar_arestas(grafo->arestas, grafo->quantidade_arestas);
    for (size_t indice = 0; indice < grafo->quantidade_arestas; indice++) {
        ArestaInterna aresta = grafo->arestas[indice];
        double novo_custo;
        if (unir(&union_find, aresta.origem, aresta.destino)) {
            status = somar_custo(saida->custo_total, aresta.peso.valor,
                                 &novo_custo);
            if (status != AGM_SUCESSO) {
                free(union_find.pai);
                free(union_find.rank);
                liberar_saida(saida);
                return status;
            }
            saida->custo_total = novo_custo;
            saida->arestas[saida->quantidade_arestas++] =
                (ArestaAGM){aresta.origem, aresta.destino, aresta.peso};
            saida->quantidade_componentes--;
        }
    }
    free(union_find.pai);
    free(union_find.rank);
    return AGM_SUCESSO;
}

static StatusAGM executar_prim(const GrafoInterno *grafo,
                               ConstrucaoResultado *saida)
{
    size_t vertices = grafo->vertices;
    size_t quantidade_arcos;
    size_t *cabecas = NULL;
    size_t *proximos = NULL;
    size_t *indices_arestas = NULL;
    size_t *aresta_pai = NULL;
    double *melhores = NULL;
    unsigned char *inserido = NULL;
    unsigned char *tem_melhor = NULL;
    StatusAGM status = alocar_saida(vertices, saida);

    if (status != AGM_SUCESSO) {
        return status;
    }
    if (vertices == 0) {
        return AGM_SUCESSO;
    }
    if (vertices > SIZE_MAX / sizeof(*cabecas) ||
        vertices > SIZE_MAX / sizeof(*aresta_pai) ||
        vertices > SIZE_MAX / sizeof(*melhores) ||
        vertices > SIZE_MAX / sizeof(*inserido) ||
        vertices > SIZE_MAX / sizeof(*tem_melhor) ||
        grafo->quantidade_arestas > SIZE_MAX / 2) {
        liberar_saida(saida);
        return AGM_LIMITE_EXCEDIDO;
    }
    quantidade_arcos = grafo->quantidade_arestas * 2;
    if (quantidade_arcos > SIZE_MAX / sizeof(*proximos) ||
        quantidade_arcos > SIZE_MAX / sizeof(*indices_arestas)) {
        liberar_saida(saida);
        return AGM_LIMITE_EXCEDIDO;
    }
    cabecas = malloc(vertices * sizeof(*cabecas));
    aresta_pai = malloc(vertices * sizeof(*aresta_pai));
    melhores = malloc(vertices * sizeof(*melhores));
    inserido = malloc(vertices * sizeof(*inserido));
    tem_melhor = malloc(vertices * sizeof(*tem_melhor));
    if (quantidade_arcos != 0) {
        proximos = malloc(quantidade_arcos * sizeof(*proximos));
        indices_arestas = malloc(quantidade_arcos * sizeof(*indices_arestas));
    }
    if (cabecas == NULL || aresta_pai == NULL || melhores == NULL ||
        inserido == NULL || tem_melhor == NULL ||
        (quantidade_arcos != 0 &&
         (proximos == NULL || indices_arestas == NULL))) {
        status = AGM_SEM_MEMORIA;
        goto cleanup;
    }
    for (size_t vertice = 0; vertice < vertices; vertice++) {
        cabecas[vertice] = SIZE_MAX;
        inserido[vertice] = 0;
        tem_melhor[vertice] = 0;
    }
    for (size_t indice = 0; indice < grafo->quantidade_arestas; indice++) {
        ArestaInterna aresta = grafo->arestas[indice];
        size_t arco = indice * 2;
        indices_arestas[arco] = indice;
        proximos[arco] = cabecas[aresta.origem];
        cabecas[aresta.origem] = arco;
        indices_arestas[arco + 1] = indice;
        proximos[arco + 1] = cabecas[aresta.destino];
        cabecas[aresta.destino] = arco + 1;
    }

    for (size_t raiz = 0; raiz < vertices; raiz++) {
        if (inserido[raiz]) {
            continue;
        }
        saida->quantidade_componentes++;
        inserido[raiz] = 1;
        for (size_t arco = cabecas[raiz]; arco != SIZE_MAX;
             arco = proximos[arco]) {
            ArestaInterna aresta = grafo->arestas[indices_arestas[arco]];
            size_t vizinho = aresta.origem == raiz ? aresta.destino :
                                                        aresta.origem;
            if (!inserido[vizinho] &&
                (!tem_melhor[vizinho] ||
                 aresta.peso.valor < melhores[vizinho])) {
                melhores[vizinho] = aresta.peso.valor;
                aresta_pai[vizinho] = indices_arestas[arco];
                tem_melhor[vizinho] = 1;
            }
        }
        for (;;) {
            size_t proximo_vertice = SIZE_MAX;
            for (size_t vertice = 0; vertice < vertices; vertice++) {
                if (!inserido[vertice] && tem_melhor[vertice] &&
                    (proximo_vertice == SIZE_MAX ||
                     melhores[vertice] < melhores[proximo_vertice])) {
                    proximo_vertice = vertice;
                }
            }
            if (proximo_vertice == SIZE_MAX) {
                break;
            }
            {
                ArestaInterna aresta = grafo->arestas[aresta_pai[proximo_vertice]];
                double novo_custo;
                status = somar_custo(saida->custo_total, aresta.peso.valor,
                                     &novo_custo);
                if (status != AGM_SUCESSO) {
                    goto cleanup;
                }
                saida->custo_total = novo_custo;
                saida->arestas[saida->quantidade_arestas++] =
                    (ArestaAGM){aresta.origem, aresta.destino, aresta.peso};
            }
            inserido[proximo_vertice] = 1;
            for (size_t arco = cabecas[proximo_vertice]; arco != SIZE_MAX;
                 arco = proximos[arco]) {
                ArestaInterna aresta = grafo->arestas[indices_arestas[arco]];
                size_t vizinho = aresta.origem == proximo_vertice ?
                    aresta.destino : aresta.origem;
                if (!inserido[vizinho] &&
                    (!tem_melhor[vizinho] ||
                     aresta.peso.valor < melhores[vizinho])) {
                    melhores[vizinho] = aresta.peso.valor;
                    aresta_pai[vizinho] = indices_arestas[arco];
                    tem_melhor[vizinho] = 1;
                }
            }
        }
    }
    status = AGM_SUCESSO;

cleanup:
    free(cabecas);
    free(proximos);
    free(indices_arestas);
    free(aresta_pai);
    free(melhores);
    free(inserido);
    free(tem_melhor);
    if (status != AGM_SUCESSO) {
        liberar_saida(saida);
    }
    return status;
}

static StatusAGM executar(const GrafoInterno *grafo, int usar_prim,
                          ResultadoAGM *resultado)
{
    GrafoInterno copia = *grafo;
    ConstrucaoResultado saida = {NULL, 0, 0.0, 0};
    StatusAGM status;
    if (usar_prim) {
        status = executar_prim(&copia, &saida);
    } else {
        status = executar_kruskal(&copia, &saida);
    }
    if (status != AGM_SUCESSO) {
        liberar_saida(&saida);
        return status;
    }
    resultado->arestas = saida.arestas;
    resultado->quantidade_arestas = saida.quantidade_arestas;
    resultado->custo_total = saida.custo_total;
    resultado->quantidade_componentes = saida.quantidade_componentes;
    resultado->arvore_completa = grafo->vertices != 0 &&
                                 saida.quantidade_componentes == 1;
    return AGM_SUCESSO;
}

static StatusAGM calcular_lista(const GrafoLista *lista, int usar_prim,
                                ResultadoAGM *resultado)
{
    GrafoInterno grafo = {0, NULL, 0};
    StatusAGM status;
    if (lista == NULL || resultado == NULL || resultado->arestas != NULL) {
        return AGM_ARGUMENTO_INVALIDO;
    }
    status = carregar_lista(lista, &grafo);
    if (status == AGM_SUCESSO) {
        status = executar(&grafo, usar_prim, resultado);
    }
    liberar_grafo_interno(&grafo);
    return status;
}

static StatusAGM calcular_matriz(const MatrizAdjacencia *matriz, int usar_prim,
                                 ResultadoAGM *resultado)
{
    GrafoInterno grafo = {0, NULL, 0};
    StatusAGM status;
    if (matriz == NULL || resultado == NULL || resultado->arestas != NULL) {
        return AGM_ARGUMENTO_INVALIDO;
    }
    status = carregar_matriz(matriz, &grafo);
    if (status == AGM_SUCESSO) {
        status = executar(&grafo, usar_prim, resultado);
    }
    liberar_grafo_interno(&grafo);
    return status;
}

StatusAGM agm_prim_lista(const GrafoLista *grafo, ResultadoAGM *resultado)
{
    return calcular_lista(grafo, 1, resultado);
}

StatusAGM agm_kruskal_lista(const GrafoLista *grafo, ResultadoAGM *resultado)
{
    return calcular_lista(grafo, 0, resultado);
}

StatusAGM agm_prim_matriz(const MatrizAdjacencia *matriz,
                          ResultadoAGM *resultado)
{
    return calcular_matriz(matriz, 1, resultado);
}

StatusAGM agm_kruskal_matriz(const MatrizAdjacencia *matriz,
                             ResultadoAGM *resultado)
{
    return calcular_matriz(matriz, 0, resultado);
}

void liberar_resultado_agm(ResultadoAGM *resultado)
{
    if (resultado == NULL) {
        return;
    }
    free(resultado->arestas);
    *resultado = (ResultadoAGM){0};
}
