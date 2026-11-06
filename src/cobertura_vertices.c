#include "cobertura_vertices.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t origem;
    size_t destino;
} ArestaCobertura;

typedef struct {
    size_t quantidade_vertices;
    size_t quantidade_arestas;
    ArestaCobertura *arestas;
} VisaoArestas;

typedef struct {
    const VisaoArestas *grafo;
    unsigned char *atual;
    unsigned char *melhor;
    unsigned char *usado_matching;
    size_t cardinalidade_atual;
    size_t cardinalidade_melhor;
    size_t nos_explorados;
    size_t limite_nos;
    int interrompido;
} ContextoExato;

static int soma_segura(size_t a, size_t b, size_t *resultado)
{
    if (a > SIZE_MAX - b) return 0;
    *resultado = a + b;
    return 1;
}

static int produto_seguro(size_t a, size_t b, size_t *resultado)
{
    if (a != 0 && b > SIZE_MAX / a) return 0;
    *resultado = a * b;
    return 1;
}

static void liberar_visao(VisaoArestas *visao)
{
    if (visao == NULL) return;
    free(visao->arestas);
    visao->arestas = NULL;
    visao->quantidade_vertices = 0;
    visao->quantidade_arestas = 0;
}

static StatusCoberturaVertices extrair_lista(const GrafoLista *grafo,
                                             VisaoArestas *visao)
{
    size_t n;
    size_t m;
    size_t bytes;
    size_t indice = 0;
    if (grafo == NULL || visao == NULL) return COBERTURA_ARGUMENTO_INVALIDO;
    n = quantidade_vertices_lista(grafo);
    m = quantidade_arestas_lista(grafo);
    if (!produto_seguro(m, sizeof(*visao->arestas), &bytes))
        return COBERTURA_SEM_MEMORIA;
    visao->arestas = m == 0 ? NULL : malloc(bytes);
    if (m != 0 && visao->arestas == NULL) return COBERTURA_SEM_MEMORIA;
    visao->quantidade_vertices = n;
    visao->quantidade_arestas = m;
    for (size_t origem = 0; origem < n; origem++) {
        const VizinhoLista *vizinho = NULL;
        if (buscar_vizinhos_lista(grafo, origem, &vizinho) != LISTA_SUCESSO) {
            liberar_visao(visao);
            return COBERTURA_ARGUMENTO_INVALIDO;
        }
        while (vizinho != NULL) {
            size_t destino = indice_vizinho_lista(vizinho);
            if (origem < destino) {
                if (indice >= m) {
                    liberar_visao(visao);
                    return COBERTURA_ARGUMENTO_INVALIDO;
                }
                visao->arestas[indice++] =
                    (ArestaCobertura){origem, destino};
            }
            vizinho = proximo_vizinho_lista(vizinho);
        }
    }
    if (indice != m) {
        liberar_visao(visao);
        return COBERTURA_ARGUMENTO_INVALIDO;
    }
    return COBERTURA_SUCESSO;
}

static StatusCoberturaVertices extrair_matriz(const MatrizAdjacencia *grafo,
                                              VisaoArestas *visao)
{
    size_t n;
    size_t m = 0;
    size_t bytes;
    size_t indice = 0;
    if (grafo == NULL || visao == NULL) return COBERTURA_ARGUMENTO_INVALIDO;
    if (matriz_e_direcionada(grafo)) return COBERTURA_GRAFO_DIRECIONADO;
    n = quantidade_vertices_matriz(grafo);
    for (size_t origem = 0; origem < n; origem++) {
        for (size_t destino = origem; destino < n; destino++) {
            int adjacente = 0;
            if (consultar_adjacencia(grafo, origem, destino, &adjacente) !=
                MATRIZ_SUCESSO) return COBERTURA_ARGUMENTO_INVALIDO;
            if (adjacente) {
                if (m == SIZE_MAX) return COBERTURA_SEM_MEMORIA;
                m++;
            }
        }
    }
    if (!produto_seguro(m, sizeof(*visao->arestas), &bytes))
        return COBERTURA_SEM_MEMORIA;
    visao->arestas = m == 0 ? NULL : malloc(bytes);
    if (m != 0 && visao->arestas == NULL) return COBERTURA_SEM_MEMORIA;
    visao->quantidade_vertices = n;
    visao->quantidade_arestas = m;
    for (size_t origem = 0; origem < n; origem++) {
        for (size_t destino = origem; destino < n; destino++) {
            int adjacente = 0;
            if (consultar_adjacencia(grafo, origem, destino, &adjacente) !=
                MATRIZ_SUCESSO) {
                liberar_visao(visao);
                return COBERTURA_ARGUMENTO_INVALIDO;
            }
            if (adjacente) {
                if (indice >= m) {
                    liberar_visao(visao);
                    return COBERTURA_ARGUMENTO_INVALIDO;
                }
                visao->arestas[indice++] =
                    (ArestaCobertura){origem, destino};
            }
        }
    }
    if (indice != m) {
        liberar_visao(visao);
        return COBERTURA_ARGUMENTO_INVALIDO;
    }
    return COBERTURA_SUCESSO;
}

static int validar_visao(const VisaoArestas *grafo,
                         const unsigned char *selecionados)
{
    for (size_t i = 0; i < grafo->quantidade_arestas; i++) {
        ArestaCobertura aresta = grafo->arestas[i];
        if (!selecionados[aresta.origem] && !selecionados[aresta.destino])
            return 0;
    }
    return 1;
}

static StatusCoberturaVertices criar_resultado(
    size_t n, MetodoCoberturaVertices metodo,
    ResultadoCoberturaVertices **resultado)
{
    ResultadoCoberturaVertices *novo;
    size_t memoria;
    if (resultado == NULL || *resultado != NULL)
        return COBERTURA_ARGUMENTO_INVALIDO;
    if (!soma_segura(sizeof(*novo), n, &memoria))
        return COBERTURA_SEM_MEMORIA;
    novo = malloc(sizeof(*novo));
    if (novo == NULL) return COBERTURA_SEM_MEMORIA;
    novo->selecionados = n == 0 ? NULL : calloc(n, sizeof(unsigned char));
    if (n != 0 && novo->selecionados == NULL) {
        free(novo);
        return COBERTURA_SEM_MEMORIA;
    }
    novo->metodo = metodo;
    novo->quantidade_vertices_grafo = n;
    novo->cardinalidade = 0;
    novo->cobertura_valida = 0;
    novo->otima_comprovada = 0;
    novo->nos_explorados = 0;
    novo->memoria_resultado_bytes = memoria;
    novo->memoria_auxiliar_pico_bytes = 0;
    *resultado = novo;
    return COBERTURA_SUCESSO;
}

static void preencher_aproximacao(const VisaoArestas *grafo,
                                  unsigned char *selecionados,
                                  size_t *cardinalidade)
{
    *cardinalidade = 0;
    for (size_t i = 0; i < grafo->quantidade_arestas; i++) {
        size_t u = grafo->arestas[i].origem;
        size_t v = grafo->arestas[i].destino;
        if (!selecionados[u] && !selecionados[v]) {
            selecionados[u] = 1;
            (*cardinalidade)++;
            if (v != u) {
                selecionados[v] = 1;
                (*cardinalidade)++;
            }
        }
    }
}

static StatusCoberturaVertices aproximar_visao(
    const VisaoArestas *grafo, ResultadoCoberturaVertices **resultado)
{
    StatusCoberturaVertices status = criar_resultado(
        grafo->quantidade_vertices, COBERTURA_METODO_APROXIMADO, resultado);
    size_t bytes_arestas = 0;
    if (status != COBERTURA_SUCESSO) return status;
    preencher_aproximacao(grafo, (*resultado)->selecionados,
                          &(*resultado)->cardinalidade);
    (*resultado)->cobertura_valida =
        validar_visao(grafo, (*resultado)->selecionados);
    (void)produto_seguro(grafo->quantidade_arestas,
                         sizeof(*grafo->arestas), &bytes_arestas);
    (*resultado)->memoria_auxiliar_pico_bytes = bytes_arestas;
    return COBERTURA_SUCESSO;
}

static size_t limite_inferior_matching(ContextoExato *contexto,
                                       size_t *primeira_aresta)
{
    size_t limite = 0;
    *primeira_aresta = SIZE_MAX;
    memset(contexto->usado_matching, 0, contexto->grafo->quantidade_vertices);
    for (size_t i = 0; i < contexto->grafo->quantidade_arestas; i++) {
        size_t u = contexto->grafo->arestas[i].origem;
        size_t v = contexto->grafo->arestas[i].destino;
        if (contexto->atual[u] || contexto->atual[v]) continue;
        if (*primeira_aresta == SIZE_MAX) *primeira_aresta = i;
        if (!contexto->usado_matching[u] && !contexto->usado_matching[v]) {
            contexto->usado_matching[u] = 1;
            contexto->usado_matching[v] = 1;
            limite++;
        }
    }
    return limite;
}

static void buscar_exato(ContextoExato *contexto)
{
    size_t primeira;
    size_t inferior;
    ArestaCobertura aresta;
    if (contexto->interrompido) return;
    if (contexto->nos_explorados >= contexto->limite_nos) {
        contexto->interrompido = 1;
        return;
    }
    contexto->nos_explorados++;
    if (contexto->cardinalidade_atual >= contexto->cardinalidade_melhor) return;
    inferior = limite_inferior_matching(contexto, &primeira);
    if (primeira == SIZE_MAX) {
        contexto->cardinalidade_melhor = contexto->cardinalidade_atual;
        memcpy(contexto->melhor, contexto->atual,
               contexto->grafo->quantidade_vertices);
        return;
    }
    if (inferior >= contexto->cardinalidade_melhor -
                    contexto->cardinalidade_atual) return;
    aresta = contexto->grafo->arestas[primeira];
    contexto->atual[aresta.origem] = 1;
    contexto->cardinalidade_atual++;
    buscar_exato(contexto);
    contexto->cardinalidade_atual--;
    contexto->atual[aresta.origem] = 0;
    if (aresta.destino == aresta.origem || contexto->interrompido) return;
    contexto->atual[aresta.destino] = 1;
    contexto->cardinalidade_atual++;
    buscar_exato(contexto);
    contexto->cardinalidade_atual--;
    contexto->atual[aresta.destino] = 0;
}

static StatusCoberturaVertices resolver_exato(
    const VisaoArestas *grafo, const ConfiguracaoCoberturaExata *configuracao,
    ResultadoCoberturaVertices **resultado)
{
    ConfiguracaoCoberturaExata cfg = configuracao == NULL ?
        configuracao_cobertura_exata_padrao() : *configuracao;
    ResultadoCoberturaVertices *novo = NULL;
    ContextoExato contexto = {0};
    size_t bytes_arestas = 0;
    size_t bytes_vetores;
    size_t auxiliar;
    StatusCoberturaVertices status = criar_resultado(
        grafo->quantidade_vertices, COBERTURA_METODO_EXATO, &novo);
    if (resultado == NULL || *resultado != NULL)
        return COBERTURA_ARGUMENTO_INVALIDO;
    if (status != COBERTURA_SUCESSO) return status;
    preencher_aproximacao(grafo, novo->selecionados, &novo->cardinalidade);
    novo->cobertura_valida = validar_visao(grafo, novo->selecionados);
    (void)produto_seguro(grafo->quantidade_arestas,
                         sizeof(*grafo->arestas), &bytes_arestas);
    novo->memoria_auxiliar_pico_bytes = bytes_arestas;
    if (!produto_seguro(grafo->quantidade_vertices, 3, &bytes_vetores) ||
        !soma_segura(bytes_arestas, bytes_vetores, &auxiliar)) {
        liberar_resultado_cobertura(&novo);
        return COBERTURA_SEM_MEMORIA;
    }
    if (grafo->quantidade_vertices > cfg.maximo_vertices) {
        *resultado = novo;
        return COBERTURA_LIMITE_ATINGIDO;
    }
    novo->memoria_auxiliar_pico_bytes = auxiliar;
    contexto.atual = grafo->quantidade_vertices == 0 ? NULL :
        calloc(grafo->quantidade_vertices, sizeof(unsigned char));
    contexto.melhor = grafo->quantidade_vertices == 0 ? NULL :
        malloc(grafo->quantidade_vertices);
    contexto.usado_matching = grafo->quantidade_vertices == 0 ? NULL :
        malloc(grafo->quantidade_vertices);
    if (grafo->quantidade_vertices != 0 &&
        (contexto.atual == NULL || contexto.melhor == NULL ||
         contexto.usado_matching == NULL)) {
        free(contexto.atual);
        free(contexto.melhor);
        free(contexto.usado_matching);
        liberar_resultado_cobertura(&novo);
        return COBERTURA_SEM_MEMORIA;
    }
    contexto.grafo = grafo;
    contexto.cardinalidade_melhor = novo->cardinalidade;
    contexto.limite_nos = cfg.maximo_nos_explorados;
    if (grafo->quantidade_vertices != 0)
        memcpy(contexto.melhor, novo->selecionados,
               grafo->quantidade_vertices);
    buscar_exato(&contexto);
    if (grafo->quantidade_vertices != 0)
        memcpy(novo->selecionados, contexto.melhor,
               grafo->quantidade_vertices);
    novo->cardinalidade = contexto.cardinalidade_melhor;
    novo->nos_explorados = contexto.nos_explorados;
    novo->cobertura_valida = validar_visao(grafo, novo->selecionados);
    novo->otima_comprovada = !contexto.interrompido;
    free(contexto.atual);
    free(contexto.melhor);
    free(contexto.usado_matching);
    *resultado = novo;
    return contexto.interrompido ? COBERTURA_LIMITE_ATINGIDO :
                                   COBERTURA_SUCESSO;
}

ConfiguracaoCoberturaExata configuracao_cobertura_exata_padrao(void)
{
    ConfiguracaoCoberturaExata configuracao = {60, 1000000};
    return configuracao;
}

StatusCoberturaVertices cobertura_vertices_aproximada_lista(
    const GrafoLista *grafo, ResultadoCoberturaVertices **resultado)
{
    VisaoArestas visao = {0};
    StatusCoberturaVertices status;
    if (resultado == NULL || *resultado != NULL)
        return COBERTURA_ARGUMENTO_INVALIDO;
    status = extrair_lista(grafo, &visao);
    if (status == COBERTURA_SUCESSO) status = aproximar_visao(&visao, resultado);
    liberar_visao(&visao);
    return status;
}

StatusCoberturaVertices cobertura_vertices_aproximada_matriz(
    const MatrizAdjacencia *grafo, ResultadoCoberturaVertices **resultado)
{
    VisaoArestas visao = {0};
    StatusCoberturaVertices status;
    if (resultado == NULL || *resultado != NULL)
        return COBERTURA_ARGUMENTO_INVALIDO;
    status = extrair_matriz(grafo, &visao);
    if (status == COBERTURA_SUCESSO) status = aproximar_visao(&visao, resultado);
    liberar_visao(&visao);
    return status;
}

StatusCoberturaVertices cobertura_vertices_exata_lista(
    const GrafoLista *grafo, const ConfiguracaoCoberturaExata *configuracao,
    ResultadoCoberturaVertices **resultado)
{
    VisaoArestas visao = {0};
    StatusCoberturaVertices status;
    if (resultado == NULL || *resultado != NULL)
        return COBERTURA_ARGUMENTO_INVALIDO;
    status = extrair_lista(grafo, &visao);
    if (status == COBERTURA_SUCESSO)
        status = resolver_exato(&visao, configuracao, resultado);
    liberar_visao(&visao);
    return status;
}

StatusCoberturaVertices cobertura_vertices_exata_matriz(
    const MatrizAdjacencia *grafo,
    const ConfiguracaoCoberturaExata *configuracao,
    ResultadoCoberturaVertices **resultado)
{
    VisaoArestas visao = {0};
    StatusCoberturaVertices status;
    if (resultado == NULL || *resultado != NULL)
        return COBERTURA_ARGUMENTO_INVALIDO;
    status = extrair_matriz(grafo, &visao);
    if (status == COBERTURA_SUCESSO)
        status = resolver_exato(&visao, configuracao, resultado);
    liberar_visao(&visao);
    return status;
}

StatusCoberturaVertices validar_cobertura_lista(
    const GrafoLista *grafo, const unsigned char *selecionados,
    size_t quantidade_vertices, int *valida)
{
    VisaoArestas visao = {0};
    StatusCoberturaVertices status;
    if (grafo == NULL || valida == NULL ||
        quantidade_vertices != quantidade_vertices_lista(grafo) ||
        (quantidade_vertices != 0 && selecionados == NULL))
        return COBERTURA_ARGUMENTO_INVALIDO;
    status = extrair_lista(grafo, &visao);
    if (status == COBERTURA_SUCESSO) *valida = validar_visao(&visao, selecionados);
    liberar_visao(&visao);
    return status;
}

StatusCoberturaVertices validar_cobertura_matriz(
    const MatrizAdjacencia *grafo, const unsigned char *selecionados,
    size_t quantidade_vertices, int *valida)
{
    VisaoArestas visao = {0};
    StatusCoberturaVertices status;
    if (grafo == NULL || valida == NULL ||
        quantidade_vertices != quantidade_vertices_matriz(grafo) ||
        (quantidade_vertices != 0 && selecionados == NULL))
        return COBERTURA_ARGUMENTO_INVALIDO;
    status = extrair_matriz(grafo, &visao);
    if (status == COBERTURA_SUCESSO) *valida = validar_visao(&visao, selecionados);
    liberar_visao(&visao);
    return status;
}

const unsigned char *vertices_selecionados_cobertura(
    const ResultadoCoberturaVertices *resultado)
{
    return resultado == NULL ? NULL : resultado->selecionados;
}

size_t cardinalidade_cobertura(const ResultadoCoberturaVertices *resultado)
{
    return resultado == NULL ? 0 : resultado->cardinalidade;
}

void liberar_resultado_cobertura(ResultadoCoberturaVertices **resultado)
{
    if (resultado == NULL || *resultado == NULL) return;
    free((*resultado)->selecionados);
    free(*resultado);
    *resultado = NULL;
}

const char *descricao_status_cobertura(StatusCoberturaVertices status)
{
    switch (status) {
    case COBERTURA_SUCESSO: return "sucesso";
    case COBERTURA_ARGUMENTO_INVALIDO: return "argumento invalido";
    case COBERTURA_SEM_MEMORIA: return "memoria insuficiente";
    case COBERTURA_LIMITE_ATINGIDO: return "execucao interrompida por limite";
    case COBERTURA_GRAFO_DIRECIONADO: return "grafo direcionado nao suportado";
    default: return "status desconhecido";
    }
}
