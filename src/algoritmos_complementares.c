#include "algoritmos_complementares.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Percorre a matriz a partir de origem. No modo fraco, um arco em qualquer
 * sentido liga os vertices. No transposto, u->v e consultado como v->u.
 * visitados e fila possuem V posicoes e sao memoria do chamador interno. */
static void alcancar_digrafo(const MatrizAdjacencia *digrafo, size_t origem,
                             int modo_fraco, int transposto,
                             unsigned char *visitados, size_t *fila)
{
    const size_t quantidade_vertices = quantidade_vertices_matriz(digrafo);
    size_t inicio_fila = 0;
    size_t fim_fila = 0;

    visitados[origem] = 1;
    fila[fim_fila++] = origem;
    while (inicio_fila < fim_fila) {
        size_t vertice_atual = fila[inicio_fila++];
        for (size_t destino = 0; destino < quantidade_vertices; destino++) {
            int arco_direto = 0;
            int arco_inverso = 0;

            if (transposto) {
                consultar_adjacencia(digrafo, destino, vertice_atual,
                                     &arco_direto);
            } else {
                consultar_adjacencia(digrafo, vertice_atual, destino,
                                     &arco_direto);
            }
            if (modo_fraco && !arco_direto) {
                consultar_adjacencia(digrafo, destino, vertice_atual,
                                     &arco_inverso);
            }
            if ((arco_direto || arco_inverso) && !visitados[destino]) {
                visitados[destino] = 1; /* Marca antes de enfileirar. */
                fila[fim_fila++] = destino;
            }
        }
    }
}

/* Retorna 1 somente quando todas as V posicoes foram visitadas. */
static int todos_visitados(const unsigned char *visitados,
                           size_t quantidade_vertices)
{
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        if (!visitados[vertice]) {
            return 0;
        }
    }
    return 1;
}

ResultadoAlgoritmo verificar_conectividade_digrafo(
    const MatrizAdjacencia *digrafo, int *conectividade_fraca,
    int *conectividade_forte)
{
    const size_t quantidade_vertices = quantidade_vertices_matriz(digrafo);
    unsigned char *visitados;
    size_t *fila;
    int resultado_fraco;
    int resultado_forte;

    if (digrafo == NULL || !matriz_e_direcionada(digrafo) ||
        conectividade_fraca == NULL || conectividade_forte == NULL ||
        conectividade_fraca == conectividade_forte) {
        return ALGORITMO_ARGUMENTO_INVALIDO;
    }
    if (quantidade_vertices == 0) {
        *conectividade_fraca = 0;
        *conectividade_forte = 0;
        return ALGORITMO_SUCESSO;
    }
    if (quantidade_vertices > SIZE_MAX / sizeof(*fila)) {
        return ALGORITMO_LIMITE_EXCEDIDO;
    }
    visitados = calloc(quantidade_vertices, sizeof(*visitados));
    fila = malloc(quantidade_vertices * sizeof(*fila));
    if (visitados == NULL || fila == NULL) {
        free(visitados);
        free(fila);
        return ALGORITMO_SEM_MEMORIA;
    }

    /* Fraca: trata cada arco como uma aresta sem orientacao. */
    alcancar_digrafo(digrafo, 0, 1, 0, visitados, fila);
    resultado_fraco = todos_visitados(visitados, quantidade_vertices);

    /* Forte: a partir de 0, todos devem ser alcancaveis no digrafo e no
     * transposto. Isso equivale a existir caminho dirigido entre todo par. */
    memset(visitados, 0, quantidade_vertices);
    alcancar_digrafo(digrafo, 0, 0, 0, visitados, fila);
    resultado_forte = todos_visitados(visitados, quantidade_vertices);
    if (resultado_forte) {
        memset(visitados, 0, quantidade_vertices);
        alcancar_digrafo(digrafo, 0, 0, 1, visitados, fila);
        resultado_forte = todos_visitados(visitados, quantidade_vertices);
    }

    free(visitados);
    free(fila);
    *conectividade_fraca = resultado_fraco;
    *conectividade_forte = resultado_forte;
    return ALGORITMO_SUCESSO;
}

void liberar_resultado_criticidade(ResultadoCriticidade **resultado)
{
    if (resultado != NULL && *resultado != NULL) {
        free((*resultado)->articulacoes);
        free((*resultado)->pontes);
        free(*resultado);
        *resultado = NULL;
    }
}

typedef struct {
    const GrafoLista *grafo;
    ResultadoCriticidade *resultado;
    size_t *tempo_descoberta;
    size_t *menor_tempo;
    size_t *pais;
    size_t tempo_atual;
} ContextoCriticidade;

/* DFS classica de Tarjan para grafo simples nao direcionado.
 * tempo_descoberta[v] registra quando v entrou na DFS.
 * menor_tempo[v] e o menor tempo alcancavel por arestas da arvore e uma
 * aresta de retorno. A raiz e articulacao com mais de um filho. Um vertice
 * nao raiz e articulacao se algum filho nao alcanca um ancestral dele.
 * A aresta para um filho e ponte se o filho nao alcanca o vertice nem seus
 * ancestrais: menor_tempo[filho] > tempo_descoberta[vertice]. */
static void visitar_criticidade(ContextoCriticidade *contexto, size_t vertice)
{
    const VizinhoLista *vizinho = NULL;
    size_t quantidade_filhos = 0;

    contexto->tempo_descoberta[vertice] = contexto->tempo_atual;
    contexto->menor_tempo[vertice] = contexto->tempo_atual++;
    buscar_vizinhos_lista(contexto->grafo, vertice, &vizinho);
    for (; vizinho != NULL; vizinho = proximo_vizinho_lista(vizinho)) {
        size_t destino = indice_vizinho_lista(vizinho);
        if (contexto->tempo_descoberta[destino] == SIZE_MAX) {
            quantidade_filhos++;
            contexto->pais[destino] = vertice;
            visitar_criticidade(contexto, destino);
            if (contexto->menor_tempo[destino] < contexto->menor_tempo[vertice]) {
                contexto->menor_tempo[vertice] = contexto->menor_tempo[destino];
            }
            if (contexto->pais[vertice] == SIZE_MAX) {
                if (quantidade_filhos > 1) {
                    contexto->resultado->articulacoes[vertice] = 1;
                }
            } else if (contexto->menor_tempo[destino] >=
                       contexto->tempo_descoberta[vertice]) {
                contexto->resultado->articulacoes[vertice] = 1;
            }
            if (contexto->menor_tempo[destino] >
                contexto->tempo_descoberta[vertice]) {
                Ponte *ponte = &contexto->resultado->pontes[
                    contexto->resultado->quantidade_pontes++];
                ponte->origem = vertice < destino ? vertice : destino;
                ponte->destino = vertice < destino ? destino : vertice;
            }
        } else if (destino != contexto->pais[vertice] &&
                   contexto->tempo_descoberta[destino] <
                   contexto->menor_tempo[vertice]) {
            contexto->menor_tempo[vertice] =
                contexto->tempo_descoberta[destino];
        }
    }
}

ResultadoAlgoritmo analisar_criticidade_lista(
    const GrafoLista *grafo, ResultadoCriticidade **resultado)
{
    const size_t quantidade_vertices = quantidade_vertices_lista(grafo);
    const size_t quantidade_arestas = quantidade_arestas_lista(grafo);
    ResultadoCriticidade *nova_analise;
    ContextoCriticidade contexto;

    if (grafo == NULL || resultado == NULL || *resultado != NULL) {
        return ALGORITMO_ARGUMENTO_INVALIDO;
    }
    if (quantidade_vertices > SIZE_MAX / sizeof(size_t) ||
        quantidade_arestas > SIZE_MAX / sizeof(Ponte)) {
        return ALGORITMO_LIMITE_EXCEDIDO;
    }
    nova_analise = calloc(1, sizeof(*nova_analise));
    if (nova_analise == NULL) {
        return ALGORITMO_SEM_MEMORIA;
    }
    nova_analise->quantidade_vertices = quantidade_vertices;
    if (quantidade_vertices != 0) {
        nova_analise->articulacoes = calloc(quantidade_vertices, 1);
    }
    if (quantidade_arestas != 0) {
        nova_analise->pontes = malloc(quantidade_arestas * sizeof(Ponte));
    }
    contexto.tempo_descoberta = quantidade_vertices == 0 ? NULL :
        malloc(quantidade_vertices * sizeof(size_t));
    contexto.menor_tempo = quantidade_vertices == 0 ? NULL :
        malloc(quantidade_vertices * sizeof(size_t));
    contexto.pais = quantidade_vertices == 0 ? NULL :
        malloc(quantidade_vertices * sizeof(size_t));
    if ((quantidade_vertices != 0 &&
         (nova_analise->articulacoes == NULL ||
          contexto.tempo_descoberta == NULL || contexto.menor_tempo == NULL ||
          contexto.pais == NULL)) ||
        (quantidade_arestas != 0 && nova_analise->pontes == NULL)) {
        free(contexto.tempo_descoberta);
        free(contexto.menor_tempo);
        free(contexto.pais);
        liberar_resultado_criticidade(&nova_analise);
        return ALGORITMO_SEM_MEMORIA;
    }

    contexto.grafo = grafo;
    contexto.resultado = nova_analise;
    contexto.tempo_atual = 0;
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        contexto.tempo_descoberta[vertice] = SIZE_MAX;
        contexto.menor_tempo[vertice] = SIZE_MAX;
        contexto.pais[vertice] = SIZE_MAX;
    }
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        if (contexto.tempo_descoberta[vertice] == SIZE_MAX) {
            visitar_criticidade(&contexto, vertice);
        }
    }
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        nova_analise->quantidade_articulacoes +=
            nova_analise->articulacoes[vertice] != 0;
    }
    free(contexto.tempo_descoberta);
    free(contexto.menor_tempo);
    free(contexto.pais);
    *resultado = nova_analise;
    return ALGORITMO_SUCESSO;
}

void liberar_resultado_coloracao(ResultadoColoracao **resultado)
{
    if (resultado != NULL && *resultado != NULL) {
        free((*resultado)->cores);
        free(*resultado);
        *resultado = NULL;
    }
}

ResultadoAlgoritmo colorir_grafo_guloso(
    const GrafoLista *grafo, ResultadoColoracao **resultado)
{
    const size_t quantidade_vertices = quantidade_vertices_lista(grafo);
    ResultadoColoracao *nova_coloracao;
    size_t *marcacoes;

    if (grafo == NULL || resultado == NULL || *resultado != NULL) {
        return ALGORITMO_ARGUMENTO_INVALIDO;
    }
    if (quantidade_vertices > SIZE_MAX / sizeof(size_t)) {
        return ALGORITMO_LIMITE_EXCEDIDO;
    }
    nova_coloracao = calloc(1, sizeof(*nova_coloracao));
    if (nova_coloracao == NULL) {
        return ALGORITMO_SEM_MEMORIA;
    }
    nova_coloracao->quantidade_vertices = quantidade_vertices;
    if (quantidade_vertices == 0) {
        *resultado = nova_coloracao;
        return ALGORITMO_SUCESSO;
    }
    nova_coloracao->cores = malloc(quantidade_vertices * sizeof(size_t));
    marcacoes = calloc(quantidade_vertices, sizeof(size_t));
    if (nova_coloracao->cores == NULL || marcacoes == NULL) {
        free(marcacoes);
        liberar_resultado_coloracao(&nova_coloracao);
        return ALGORITMO_SEM_MEMORIA;
    }
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        nova_coloracao->cores[vertice] = SIZE_MAX;
    }

    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        const VizinhoLista *vizinho = NULL;
        size_t cor = 0;
        buscar_vizinhos_lista(grafo, vertice, &vizinho);
        for (; vizinho != NULL; vizinho = proximo_vizinho_lista(vizinho)) {
            size_t destino = indice_vizinho_lista(vizinho);
            if (nova_coloracao->cores[destino] != SIZE_MAX) {
                marcacoes[nova_coloracao->cores[destino]] = vertice + 1;
            }
        }
        while (marcacoes[cor] == vertice + 1) {
            cor++;
        }
        nova_coloracao->cores[vertice] = cor;
        if (cor + 1 > nova_coloracao->quantidade_cores) {
            nova_coloracao->quantidade_cores = cor + 1;
        }
    }
    free(marcacoes);
    *resultado = nova_coloracao;
    return ALGORITMO_SUCESSO;
}

ResultadoAlgoritmo verificar_formula_euler(size_t quantidade_vertices,
                                          size_t quantidade_arestas,
                                          size_t quantidade_faces,
                                          ResultadoEuler *resultado)
{
    ResultadoEuler novo_resultado = {0, 0, 0};
    size_t vertices_mais_faces;
    size_t arestas_mais_dois;

    if (resultado == NULL) {
        return ALGORITMO_ARGUMENTO_INVALIDO;
    }
    if (quantidade_vertices > SIZE_MAX - quantidade_faces ||
        quantidade_arestas > SIZE_MAX - 2) {
        return ALGORITMO_LIMITE_EXCEDIDO;
    }
    vertices_mais_faces = quantidade_vertices + quantidade_faces;
    arestas_mais_dois = quantidade_arestas + 2;
    novo_resultado.satisfaz_formula =
        vertices_mais_faces == arestas_mais_dois;
    if (quantidade_vertices >= 3) {
        const size_t limite_dividido_por_tres = quantidade_vertices - 2;
        const size_t quociente = quantidade_arestas / 3;
        const size_t resto = quantidade_arestas % 3;
        novo_resultado.limite_arestas_aplicavel = 1;
        novo_resultado.satisfaz_limite_arestas =
            quociente < limite_dividido_por_tres ||
            (quociente == limite_dividido_por_tres && resto == 0);
    }
    *resultado = novo_resultado;
    return ALGORITMO_SUCESSO;
}
