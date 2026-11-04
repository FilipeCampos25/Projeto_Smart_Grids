#include "matriz_adjacencia.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

/* As N*N celulas ocupam um bloco contiguo: nao ha ponteiros por linha.
 * A posicao [origem][destino] fica em origem*N + destino.
 */
struct MatrizAdjacencia {
    size_t quantidade_vertices;
    unsigned char *conexoes;
    double *pesos;
    unsigned char *origens_peso;
    size_t limite_bytes;
    int direcionada;
};

ResultadoMatriz estimar_memoria_matriz(size_t quantidade_vertices, size_t *bytes)
{
    size_t quantidade_celulas;

    if (bytes == NULL) {
        return MATRIZ_ARGUMENTO_INVALIDO;
    }
    /* Verifica a multiplicacao antes de executa-la. */
    if (quantidade_vertices != 0 &&
        quantidade_vertices > SIZE_MAX / quantidade_vertices) {
        return MATRIZ_TAMANHO_INVALIDO;
    }
    quantidade_celulas = quantidade_vertices * quantidade_vertices;
    /* Inclui o cabecalho no teste de overflow da soma. */
    if (quantidade_celulas >
        (SIZE_MAX - sizeof(MatrizAdjacencia)) / sizeof(unsigned char)) {
        return MATRIZ_TAMANHO_INVALIDO;
    }
    *bytes = sizeof(MatrizAdjacencia) + quantidade_celulas * sizeof(unsigned char);
    return MATRIZ_SUCESSO;
}

ResultadoMatriz estimar_memoria_matriz_ponderada(size_t quantidade_vertices,
                                                 size_t *bytes)
{
    size_t quantidade_celulas;
    size_t bytes_por_celula = sizeof(unsigned char) + sizeof(double) +
                              sizeof(unsigned char);
    if (bytes == NULL) {
        return MATRIZ_ARGUMENTO_INVALIDO;
    }
    if (quantidade_vertices != 0 &&
        quantidade_vertices > SIZE_MAX / quantidade_vertices) {
        return MATRIZ_TAMANHO_INVALIDO;
    }
    quantidade_celulas = quantidade_vertices * quantidade_vertices;
    if (quantidade_celulas >
        (SIZE_MAX - sizeof(MatrizAdjacencia)) / bytes_por_celula) {
        return MATRIZ_TAMANHO_INVALIDO;
    }
    *bytes = sizeof(MatrizAdjacencia) + quantidade_celulas * bytes_por_celula;
    return MATRIZ_SUCESSO;
}

/* Implementacao comum das duas criacoes. O campo direcionada muda apenas a
 * escrita de conexoes; armazenamento, limites e propriedade permanecem iguais. */
static ResultadoMatriz criar_matriz_com_tipo(size_t quantidade_vertices,
                                             size_t limite_bytes,
                                             int direcionada,
                                             MatrizAdjacencia **matriz_criada)
{
    size_t bytes;
    MatrizAdjacencia *nova_matriz;
    ResultadoMatriz resultado;

    if (matriz_criada == NULL || *matriz_criada != NULL) {
        return MATRIZ_ARGUMENTO_INVALIDO;
    }
    resultado = estimar_memoria_matriz(quantidade_vertices, &bytes);
    if (resultado != MATRIZ_SUCESSO) {
        return resultado;
    }
    if (bytes > limite_bytes) {
        return MATRIZ_LIMITE_MEMORIA;
    }
    nova_matriz = malloc(sizeof(MatrizAdjacencia));
    if (nova_matriz == NULL) {
        return MATRIZ_SEM_MEMORIA;
    }
    nova_matriz->quantidade_vertices = quantidade_vertices;
    nova_matriz->conexoes = NULL;
    nova_matriz->pesos = NULL;
    nova_matriz->origens_peso = NULL;
    nova_matriz->limite_bytes = limite_bytes;
    nova_matriz->direcionada = direcionada;
    if (quantidade_vertices != 0) {
        nova_matriz->conexoes = calloc(quantidade_vertices * quantidade_vertices,
                                      sizeof(unsigned char));
        if (nova_matriz->conexoes == NULL) {
            free(nova_matriz);
            return MATRIZ_SEM_MEMORIA;
        }
    }
    /* A propriedade so passa ao chamador depois da construcao completa. */
    *matriz_criada = nova_matriz;
    return MATRIZ_SUCESSO;
}

ResultadoMatriz criar_matriz(size_t quantidade_vertices, size_t limite_bytes,
                            MatrizAdjacencia **matriz_criada)
{
    return criar_matriz_com_tipo(quantidade_vertices, limite_bytes, 0,
                                 matriz_criada);
}

ResultadoMatriz criar_matriz_direcionada(size_t quantidade_vertices,
                                        size_t limite_bytes,
                                        MatrizAdjacencia **matriz_criada)
{
    return criar_matriz_com_tipo(quantidade_vertices, limite_bytes, 1,
                                 matriz_criada);
}

/* Valida o objeto e os dois indices, antes de calcular qualquer posicao.
 * Retorna 1 se validos, 0 caso contrario; nao altera nem aloca memoria.
 */
static int indices_validos(const MatrizAdjacencia *matriz, size_t vertice_origem,
                          size_t vertice_destino)
{
    return matriz != NULL && vertice_origem < matriz->quantidade_vertices &&
           vertice_destino < matriz->quantidade_vertices;
}

/* Grava 0 ou 1 simetricamente para os indices dados. Retorna erro sem
 * alterar o objeto se algum argumento for invalido. Nao aloca memoria.
 */
static ResultadoMatriz definir_conexao(MatrizAdjacencia *matriz,
                                      size_t vertice_origem,
                                      size_t vertice_destino,
                                      unsigned char conectados)
{
    size_t quantidade_vertices;
    size_t posicao_direta;
    size_t posicao_inversa;

    if (!indices_validos(matriz, vertice_origem, vertice_destino)) {
        return MATRIZ_ARGUMENTO_INVALIDO;
    }
    quantidade_vertices = matriz->quantidade_vertices;
    posicao_direta = vertice_origem * quantidade_vertices + vertice_destino;
    posicao_inversa = vertice_destino * quantidade_vertices + vertice_origem;
    matriz->conexoes[posicao_direta] = conectados;
    if (!matriz->direcionada) {
        matriz->conexoes[posicao_inversa] = conectados;
    }
    if (!conectados && matriz->pesos != NULL) {
        matriz->pesos[posicao_direta] = 0.0;
        matriz->origens_peso[posicao_direta] = ORIGEM_PESO_NAO_INFORMADA;
        if (!matriz->direcionada) {
            matriz->pesos[posicao_inversa] = 0.0;
            matriz->origens_peso[posicao_inversa] = ORIGEM_PESO_NAO_INFORMADA;
        }
    }
    return MATRIZ_SUCESSO;
}

ResultadoMatriz inserir_aresta(MatrizAdjacencia *matriz, size_t vertice_origem,
                              size_t vertice_destino)
{
    return definir_conexao(matriz, vertice_origem, vertice_destino, 1);
}

static ResultadoMatriz garantir_armazenamento_pesos(MatrizAdjacencia *matriz)
{
    size_t quantidade_celulas;
    size_t memoria_ponderada;
    double *pesos;
    unsigned char *origens;
    ResultadoMatriz status;
    if (matriz->pesos != NULL) {
        return MATRIZ_SUCESSO;
    }
    status = estimar_memoria_matriz_ponderada(matriz->quantidade_vertices,
                                              &memoria_ponderada);
    if (status != MATRIZ_SUCESSO) {
        return status;
    }
    if (memoria_ponderada > matriz->limite_bytes) {
        return MATRIZ_LIMITE_MEMORIA;
    }
    quantidade_celulas = matriz->quantidade_vertices *
                          matriz->quantidade_vertices;
    pesos = calloc(quantidade_celulas, sizeof(*pesos));
    origens = calloc(quantidade_celulas, sizeof(*origens));
    if (pesos == NULL || origens == NULL) {
        free(pesos);
        free(origens);
        return MATRIZ_SEM_MEMORIA;
    }
    matriz->pesos = pesos;
    matriz->origens_peso = origens;
    return MATRIZ_SUCESSO;
}

ResultadoMatriz inserir_aresta_ponderada(
    MatrizAdjacencia *matriz, size_t vertice_origem, size_t vertice_destino,
    double peso, OrigemPesoAresta origem_peso)
{
    size_t n;
    size_t posicao_direta;
    size_t posicao_inversa;
    ResultadoMatriz status;
    if (!indices_validos(matriz, vertice_origem, vertice_destino)) {
        return MATRIZ_ARGUMENTO_INVALIDO;
    }
    if (!isfinite(peso) || peso < 0.0 ||
        (origem_peso != ORIGEM_PESO_COMPRIMENTO_MEDIDO &&
         origem_peso != ORIGEM_PESO_CUSTO_ASSUMIDO)) {
        return MATRIZ_PESO_INVALIDO;
    }
    status = garantir_armazenamento_pesos(matriz);
    if (status != MATRIZ_SUCESSO) {
        return status;
    }
    n = matriz->quantidade_vertices;
    posicao_direta = vertice_origem * n + vertice_destino;
    posicao_inversa = vertice_destino * n + vertice_origem;
    matriz->conexoes[posicao_direta] = 1;
    matriz->pesos[posicao_direta] = peso;
    matriz->origens_peso[posicao_direta] = (unsigned char)origem_peso;
    if (!matriz->direcionada) {
        matriz->conexoes[posicao_inversa] = 1;
        matriz->pesos[posicao_inversa] = peso;
        matriz->origens_peso[posicao_inversa] = (unsigned char)origem_peso;
    }
    return MATRIZ_SUCESSO;
}

ResultadoMatriz remover_aresta(MatrizAdjacencia *matriz, size_t vertice_origem,
                              size_t vertice_destino)
{
    return definir_conexao(matriz, vertice_origem, vertice_destino, 0);
}

ResultadoMatriz consultar_adjacencia(const MatrizAdjacencia *matriz,
                                    size_t vertice_origem,
                                    size_t vertice_destino, int *adjacente)
{
    if (adjacente == NULL || !indices_validos(matriz, vertice_origem, vertice_destino)) {
        return MATRIZ_ARGUMENTO_INVALIDO;
    }
    *adjacente = matriz->conexoes[vertice_origem * matriz->quantidade_vertices +
                                vertice_destino];
    return MATRIZ_SUCESSO;
}

ResultadoMatriz consultar_peso_aresta(const MatrizAdjacencia *matriz,
                                      size_t vertice_origem,
                                      size_t vertice_destino,
                                      PesoAresta *peso)
{
    size_t posicao;
    if (peso == NULL || !indices_validos(matriz, vertice_origem,
                                         vertice_destino)) {
        return MATRIZ_ARGUMENTO_INVALIDO;
    }
    posicao = vertice_origem * matriz->quantidade_vertices + vertice_destino;
    if (!matriz->conexoes[posicao]) {
        return MATRIZ_ARESTA_INEXISTENTE;
    }
    if (matriz->pesos == NULL ||
        matriz->origens_peso[posicao] == ORIGEM_PESO_NAO_INFORMADA) {
        *peso = (PesoAresta){0, 0.0, ORIGEM_PESO_NAO_INFORMADA};
    } else {
        *peso = (PesoAresta){1, matriz->pesos[posicao],
            (OrigemPesoAresta)matriz->origens_peso[posicao]};
    }
    return MATRIZ_SUCESSO;
}

size_t quantidade_vertices_matriz(const MatrizAdjacencia *matriz)
{
    return matriz == NULL ? 0 : matriz->quantidade_vertices;
}

int matriz_e_direcionada(const MatrizAdjacencia *matriz)
{
    return matriz != NULL && matriz->direcionada;
}

size_t memoria_matriz(const MatrizAdjacencia *matriz)
{
    size_t bytes = 0;

    if (matriz != NULL) {
        /* N foi validado na criacao e nao pode ser alterado pela API. */
        if (matriz->pesos != NULL) {
            estimar_memoria_matriz_ponderada(matriz->quantidade_vertices,
                                             &bytes);
        } else {
            estimar_memoria_matriz(matriz->quantidade_vertices, &bytes);
        }
    }
    return bytes;
}

void destruir_matriz(MatrizAdjacencia **matriz)
{
    if (matriz != NULL && *matriz != NULL) {
        free((*matriz)->origens_peso);
        free((*matriz)->pesos);
        free((*matriz)->conexoes);
        free(*matriz);
        *matriz = NULL;
    }
}
