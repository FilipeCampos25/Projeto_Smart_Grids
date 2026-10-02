#ifndef DATASET_H
#define DATASET_H

#include "grafo_lista.h"
#include "matriz_adjacencia.h"

#include <stddef.h>

typedef enum {
    DATASET_SUCESSO = 0,
    DATASET_ARGUMENTO_INVALIDO,
    DATASET_ARQUIVO_INDISPONIVEL,
    DATASET_FORMATO_INVALIDO,
    DATASET_REFERENCIA_INVALIDA,
    DATASET_DUPLICATA,
    DATASET_SEM_MEMORIA,
    DATASET_ERRO_GRAFO
} StatusDataset;

typedef struct {
    char *id_externo;
    char *tipo;
    char *subestacao;
    char *circuito;
} VerticeDataset;

typedef struct {
    char *id_externo;
    size_t origem;
    size_t destino;
    char *tipo;
    double comprimento_m;
    int possui_comprimento;
    char *status;
    char *circuito;
} ArestaDataset;

typedef struct {
    VerticeDataset *vertices;
    ArestaDataset *arestas;
    size_t quantidade_vertices;
    size_t quantidade_arestas;
    size_t indice_subestacao;
} Dataset;

typedef struct {
    size_t linha;
    char mensagem[160];
} ErroDataset;

/* Le os dois CSVs normalizados. Qualquer erro rejeita a carga inteira, informa
 * linha/motivo e libera a construcao parcial. IDs sao textuais e mapeados por
 * tabela hash; nunca sao usados diretamente como indices. */
StatusDataset carregar_dataset(const char *arquivo_vertices,
                               const char *arquivo_arestas,
                               Dataset **dataset, ErroDataset *erro);

/* Constroem a mesma topologia nao direcionada a partir das arestas resolvidas. */
StatusDataset construir_lista_dataset(const Dataset *dataset,
                                      GrafoLista **grafo);
StatusDataset construir_matriz_dataset(const Dataset *dataset,
                                       MatrizAdjacencia **matriz);

/* Busca linear para interacao/relatorios; a carga usa tabela hash. */
int buscar_indice_dataset(const Dataset *dataset, const char *id_externo,
                          size_t *indice);
void liberar_dataset(Dataset **dataset);
const char *descricao_status_dataset(StatusDataset status);

#endif
