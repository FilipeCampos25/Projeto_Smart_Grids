#include "dataset.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAMANHO_LINHA 4096

typedef struct {
    const char *chave;
    size_t valor;
} EntradaMapa;

typedef struct {
    EntradaMapa *entradas;
    size_t capacidade;
} MapaIds;

static void definir_erro(ErroDataset *erro, size_t linha, const char *mensagem)
{
    if (erro != NULL) {
        erro->linha = linha;
        snprintf(erro->mensagem, sizeof(erro->mensagem), "%s", mensagem);
    }
}

static char *duplicar_texto(const char *texto)
{
    size_t tamanho = strlen(texto) + 1;
    char *copia = malloc(tamanho);
    if (copia != NULL) {
        memcpy(copia, texto, tamanho);
    }
    return copia;
}

static size_t hash_texto(const char *texto)
{
    size_t hash = (size_t)1469598103934665603ULL;
    while (*texto != '\0') {
        hash ^= (unsigned char)*texto++;
        hash *= (size_t)1099511628211ULL;
    }
    return hash;
}

static int criar_mapa(MapaIds *mapa, size_t quantidade)
{
    size_t capacidade = 8;
    if (quantidade > SIZE_MAX / 2) {
        return 0;
    }
    while (capacidade < quantidade * 2) {
        if (capacidade > SIZE_MAX / 2) {
            return 0;
        }
        capacidade *= 2;
    }
    mapa->entradas = calloc(capacidade, sizeof(*mapa->entradas));
    mapa->capacidade = mapa->entradas == NULL ? 0 : capacidade;
    return mapa->entradas != NULL;
}

static int consultar_mapa(const MapaIds *mapa, const char *chave, size_t *valor)
{
    size_t posicao = hash_texto(chave) & (mapa->capacidade - 1);
    while (mapa->entradas[posicao].chave != NULL) {
        if (strcmp(mapa->entradas[posicao].chave, chave) == 0) {
            *valor = mapa->entradas[posicao].valor;
            return 1;
        }
        posicao = (posicao + 1) & (mapa->capacidade - 1);
    }
    return 0;
}

static int inserir_mapa(MapaIds *mapa, const char *chave, size_t valor)
{
    size_t existente;
    size_t posicao = hash_texto(chave) & (mapa->capacidade - 1);
    if (consultar_mapa(mapa, chave, &existente)) {
        return 0;
    }
    while (mapa->entradas[posicao].chave != NULL) {
        posicao = (posicao + 1) & (mapa->capacidade - 1);
    }
    mapa->entradas[posicao].chave = chave;
    mapa->entradas[posicao].valor = valor;
    return 1;
}

/* O formato normalizado proibe virgulas e aspas nos campos textuais. Esta
 * divisao preserva campos vazios (necessarios para comprimento opcional). */
static int dividir_csv(char *linha, char **campos, size_t quantidade)
{
    size_t atual = 0;
    char *inicio = linha;
    for (char *cursor = linha;; cursor++) {
        if (*cursor == ',' || *cursor == '\0' || *cursor == '\n' ||
            *cursor == '\r') {
            char separador = *cursor;
            if (atual >= quantidade) {
                return 0;
            }
            *cursor = '\0';
            campos[atual++] = inicio;
            if (separador != ',') {
                break;
            }
            inicio = cursor + 1;
        }
    }
    return atual == quantidade;
}

static int cabecalho_igual(char *linha, const char *esperado)
{
    linha[strcspn(linha, "\r\n")] = '\0';
    return strcmp(linha, esperado) == 0;
}

static void liberar_vertice(VerticeDataset *vertice)
{
    free(vertice->id_externo);
    free(vertice->tipo);
    free(vertice->subestacao);
    free(vertice->circuito);
}

static void liberar_aresta(ArestaDataset *aresta)
{
    free(aresta->id_externo);
    free(aresta->tipo);
    free(aresta->status);
    free(aresta->circuito);
}

void liberar_dataset(Dataset **dataset)
{
    if (dataset == NULL || *dataset == NULL) {
        return;
    }
    for (size_t i = 0; i < (*dataset)->quantidade_vertices; i++) {
        liberar_vertice(&(*dataset)->vertices[i]);
    }
    for (size_t i = 0; i < (*dataset)->quantidade_arestas; i++) {
        liberar_aresta(&(*dataset)->arestas[i]);
    }
    free((*dataset)->vertices);
    free((*dataset)->arestas);
    free(*dataset);
    *dataset = NULL;
}

static StatusDataset ler_vertices(FILE *arquivo, Dataset *dataset,
                                  ErroDataset *erro)
{
    char linha[TAMANHO_LINHA];
    size_t numero_linha = 1;
    size_t capacidade = 0;
    if (fgets(linha, sizeof(linha), arquivo) == NULL ||
        !cabecalho_igual(linha, "id,tipo,subestacao,circuito")) {
        definir_erro(erro, 1, "cabecalho de vertices invalido");
        return DATASET_FORMATO_INVALIDO;
    }
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char *campos[4];
        VerticeDataset novo = {0};
        numero_linha++;
        if (strchr(linha, '\n') == NULL && !feof(arquivo)) {
            definir_erro(erro, numero_linha, "linha de vertice muito longa");
            return DATASET_FORMATO_INVALIDO;
        }
        if (!dividir_csv(linha, campos, 4) || campos[0][0] == '\0' ||
            campos[1][0] == '\0' || campos[3][0] == '\0') {
            definir_erro(erro, numero_linha, "registro de vertice invalido");
            return DATASET_FORMATO_INVALIDO;
        }
        if (dataset->quantidade_vertices == capacidade) {
            size_t nova_capacidade = capacidade == 0 ? 256 : capacidade * 2;
            VerticeDataset *novos = realloc(
                dataset->vertices, nova_capacidade * sizeof(*novos));
            if (novos == NULL) {
                return DATASET_SEM_MEMORIA;
            }
            dataset->vertices = novos;
            capacidade = nova_capacidade;
        }
        novo.id_externo = duplicar_texto(campos[0]);
        novo.tipo = duplicar_texto(campos[1]);
        novo.subestacao = duplicar_texto(campos[2]);
        novo.circuito = duplicar_texto(campos[3]);
        if (novo.id_externo == NULL || novo.tipo == NULL ||
            novo.subestacao == NULL || novo.circuito == NULL) {
            liberar_vertice(&novo);
            return DATASET_SEM_MEMORIA;
        }
        dataset->vertices[dataset->quantidade_vertices++] = novo;
    }
    if (ferror(arquivo) || dataset->quantidade_vertices == 0) {
        definir_erro(erro, numero_linha, "arquivo de vertices vazio ou ilegivel");
        return DATASET_FORMATO_INVALIDO;
    }
    return DATASET_SUCESSO;
}

static StatusDataset criar_indice_ids(Dataset *dataset, MapaIds *mapa,
                                      ErroDataset *erro)
{
    if (!criar_mapa(mapa, dataset->quantidade_vertices)) {
        return DATASET_SEM_MEMORIA;
    }
    dataset->indice_subestacao = SIZE_MAX;
    for (size_t i = 0; i < dataset->quantidade_vertices; i++) {
        if (!inserir_mapa(mapa, dataset->vertices[i].id_externo, i)) {
            definir_erro(erro, i + 2, "ID de vertice duplicado");
            return DATASET_DUPLICATA;
        }
        if (strcmp(dataset->vertices[i].tipo, "subestacao") == 0 &&
            dataset->indice_subestacao == SIZE_MAX) {
            dataset->indice_subestacao = i;
        }
    }
    if (dataset->indice_subestacao == SIZE_MAX) {
        definir_erro(erro, 0, "nenhuma subestacao identificada");
        return DATASET_FORMATO_INVALIDO;
    }
    return DATASET_SUCESSO;
}

static StatusDataset ler_arestas(FILE *arquivo, Dataset *dataset,
                                 const MapaIds *mapa, ErroDataset *erro)
{
    char linha[TAMANHO_LINHA];
    size_t numero_linha = 1;
    size_t capacidade = 0;
    if (fgets(linha, sizeof(linha), arquivo) == NULL ||
        !cabecalho_igual(linha,
            "id,origem,destino,tipo,comprimento_m,status,circuito")) {
        definir_erro(erro, 1, "cabecalho de arestas invalido");
        return DATASET_FORMATO_INVALIDO;
    }
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char *campos[7];
        char *fim_numero;
        ArestaDataset nova = {0};
        numero_linha++;
        if (!dividir_csv(linha, campos, 7) || campos[0][0] == '\0' ||
            campos[1][0] == '\0' || campos[2][0] == '\0' ||
            campos[3][0] == '\0' || strcmp(campos[5], "ativo") != 0) {
            definir_erro(erro, numero_linha, "registro de aresta invalido/inativo");
            return DATASET_FORMATO_INVALIDO;
        }
        if (!consultar_mapa(mapa, campos[1], &nova.origem) ||
            !consultar_mapa(mapa, campos[2], &nova.destino)) {
            definir_erro(erro, numero_linha, "aresta referencia ID desconhecido");
            return DATASET_REFERENCIA_INVALIDA;
        }
        if (nova.origem == nova.destino) {
            definir_erro(erro, numero_linha, "self-loop nao permitido");
            return DATASET_FORMATO_INVALIDO;
        }
        if (campos[4][0] != '\0') {
            errno = 0;
            nova.comprimento_m = strtod(campos[4], &fim_numero);
            if (errno != 0 || *fim_numero != '\0' || nova.comprimento_m < 0) {
                definir_erro(erro, numero_linha, "comprimento invalido");
                return DATASET_FORMATO_INVALIDO;
            }
            nova.possui_comprimento = 1;
        }
        if (dataset->quantidade_arestas == capacidade) {
            size_t nova_capacidade = capacidade == 0 ? 256 : capacidade * 2;
            ArestaDataset *novas = realloc(
                dataset->arestas, nova_capacidade * sizeof(*novas));
            if (novas == NULL) {
                return DATASET_SEM_MEMORIA;
            }
            dataset->arestas = novas;
            capacidade = nova_capacidade;
        }
        nova.id_externo = duplicar_texto(campos[0]);
        nova.tipo = duplicar_texto(campos[3]);
        nova.status = duplicar_texto(campos[5]);
        nova.circuito = duplicar_texto(campos[6]);
        if (nova.id_externo == NULL || nova.tipo == NULL ||
            nova.status == NULL || nova.circuito == NULL) {
            liberar_aresta(&nova);
            return DATASET_SEM_MEMORIA;
        }
        dataset->arestas[dataset->quantidade_arestas++] = nova;
    }
    if (ferror(arquivo) || dataset->quantidade_arestas == 0) {
        definir_erro(erro, numero_linha, "arquivo de arestas vazio ou ilegivel");
        return DATASET_FORMATO_INVALIDO;
    }
    return DATASET_SUCESSO;
}

StatusDataset carregar_dataset(const char *arquivo_vertices,
                               const char *arquivo_arestas,
                               Dataset **dataset, ErroDataset *erro)
{
    FILE *vertices;
    FILE *arestas;
    Dataset *novo;
    MapaIds mapa = {0};
    StatusDataset status;
    if (arquivo_vertices == NULL || arquivo_arestas == NULL ||
        dataset == NULL || *dataset != NULL) {
        return DATASET_ARGUMENTO_INVALIDO;
    }
    if (erro != NULL) {
        erro->linha = 0;
        erro->mensagem[0] = '\0';
    }
    vertices = fopen(arquivo_vertices, "r");
    if (vertices == NULL) {
        definir_erro(erro, 0, "nao foi possivel abrir vertices.csv");
        return DATASET_ARQUIVO_INDISPONIVEL;
    }
    arestas = fopen(arquivo_arestas, "r");
    if (arestas == NULL) {
        fclose(vertices);
        definir_erro(erro, 0, "nao foi possivel abrir arestas.csv");
        return DATASET_ARQUIVO_INDISPONIVEL;
    }
    novo = calloc(1, sizeof(*novo));
    if (novo == NULL) {
        fclose(vertices);
        fclose(arestas);
        return DATASET_SEM_MEMORIA;
    }
    status = ler_vertices(vertices, novo, erro);
    if (status == DATASET_SUCESSO) {
        status = criar_indice_ids(novo, &mapa, erro);
    }
    if (status == DATASET_SUCESSO) {
        status = ler_arestas(arestas, novo, &mapa, erro);
    }
    free(mapa.entradas);
    fclose(vertices);
    fclose(arestas);
    if (status != DATASET_SUCESSO) {
        liberar_dataset(&novo);
        return status;
    }
    *dataset = novo;
    return DATASET_SUCESSO;
}

StatusDataset construir_lista_dataset(const Dataset *dataset,
                                      GrafoLista **grafo)
{
    GrafoLista *novo;
    if (dataset == NULL || grafo == NULL || *grafo != NULL) {
        return DATASET_ARGUMENTO_INVALIDO;
    }
    novo = criar_grafo_lista();
    if (novo == NULL) {
        return DATASET_SEM_MEMORIA;
    }
    for (size_t i = 0; i < dataset->quantidade_vertices; i++) {
        size_t indice;
        if (inserir_vertice_lista(novo, &indice) != LISTA_SUCESSO || indice != i) {
            liberar_grafo_lista(novo);
            return DATASET_ERRO_GRAFO;
        }
    }
    for (size_t i = 0; i < dataset->quantidade_arestas; i++) {
        if (inserir_aresta_lista(novo, dataset->arestas[i].origem,
                                 dataset->arestas[i].destino) != LISTA_SUCESSO) {
            liberar_grafo_lista(novo);
            return DATASET_ERRO_GRAFO;
        }
    }
    *grafo = novo;
    return DATASET_SUCESSO;
}

StatusDataset construir_matriz_dataset(const Dataset *dataset,
                                       MatrizAdjacencia **matriz)
{
    size_t bytes;
    ResultadoMatriz status_matriz;
    if (dataset == NULL || matriz == NULL || *matriz != NULL) {
        return DATASET_ARGUMENTO_INVALIDO;
    }
    status_matriz = estimar_memoria_matriz(dataset->quantidade_vertices, &bytes);
    if (status_matriz != MATRIZ_SUCESSO) return DATASET_ERRO_GRAFO;
    status_matriz = criar_matriz(dataset->quantidade_vertices, bytes, matriz);
    if (status_matriz == MATRIZ_SEM_MEMORIA) return DATASET_SEM_MEMORIA;
    if (status_matriz != MATRIZ_SUCESSO) return DATASET_ERRO_GRAFO;
    for (size_t i = 0; i < dataset->quantidade_arestas; i++) {
        if (inserir_aresta(*matriz, dataset->arestas[i].origem,
                           dataset->arestas[i].destino) != MATRIZ_SUCESSO) {
            destruir_matriz(matriz);
            return DATASET_ERRO_GRAFO;
        }
    }
    return DATASET_SUCESSO;
}

int buscar_indice_dataset(const Dataset *dataset, const char *id_externo,
                          size_t *indice)
{
    if (dataset == NULL || id_externo == NULL || indice == NULL) {
        return 0;
    }
    for (size_t i = 0; i < dataset->quantidade_vertices; i++) {
        if (strcmp(dataset->vertices[i].id_externo, id_externo) == 0) {
            *indice = i;
            return 1;
        }
    }
    return 0;
}

const char *descricao_status_dataset(StatusDataset status)
{
    static const char *descricoes[] = {
        "sucesso", "argumento invalido", "arquivo indisponivel",
        "formato invalido", "referencia invalida", "ID duplicado",
        "memoria insuficiente", "erro ao construir grafo"
    };
    return status <= DATASET_ERRO_GRAFO ? descricoes[status] : "status desconhecido";
}
