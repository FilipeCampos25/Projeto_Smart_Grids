#include "matriz_adjacencia.h"

#include <stdint.h>
#include <stdlib.h>

/* As N*N celulas ocupam um bloco contiguo: nao ha ponteiros por linha.
 * A posicao [origem][destino] fica em origem*N + destino.
 */
struct MatrizAdjacencia {
    size_t quantidade_vertices;
    unsigned char *conexoes;
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

ResultadoMatriz criar_matriz(size_t quantidade_vertices, size_t limite_bytes,
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

    if (!indices_validos(matriz, vertice_origem, vertice_destino)) {
        return MATRIZ_ARGUMENTO_INVALIDO;
    }
    quantidade_vertices = matriz->quantidade_vertices;
    matriz->conexoes[vertice_origem * quantidade_vertices + vertice_destino] = conectados;
    matriz->conexoes[vertice_destino * quantidade_vertices + vertice_origem] = conectados;
    return MATRIZ_SUCESSO;
}

ResultadoMatriz inserir_aresta(MatrizAdjacencia *matriz, size_t vertice_origem,
                              size_t vertice_destino)
{
    return definir_conexao(matriz, vertice_origem, vertice_destino, 1);
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

size_t quantidade_vertices_matriz(const MatrizAdjacencia *matriz)
{
    return matriz == NULL ? 0 : matriz->quantidade_vertices;
}

size_t memoria_matriz(const MatrizAdjacencia *matriz)
{
    size_t bytes = 0;

    if (matriz != NULL) {
        /* N foi validado na criacao e nao pode ser alterado pela API. */
        estimar_memoria_matriz(matriz->quantidade_vertices, &bytes);
    }
    return bytes;
}

void destruir_matriz(MatrizAdjacencia **matriz)
{
    if (matriz != NULL && *matriz != NULL) {
        free((*matriz)->conexoes);
        free(*matriz);
        *matriz = NULL;
    }
}
