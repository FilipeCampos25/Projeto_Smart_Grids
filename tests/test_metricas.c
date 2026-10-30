#include "dataset.h"
#include "grafo_lista.h"
#include "matriz_adjacencia.h"
#include "metricas.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARQUIVO_TEMPORARIO "build/metricas_teste.csv"

enum {
    QUANTIDADE_TAMANHOS = 3,
    QUANTIDADE_ALGORITMOS = 3,
    QUANTIDADE_REPRESENTACOES = 2,
    REPETICOES_TESTE = 2
};

/* Mesmos recortes documentados para a Fase I; nao sao bytes esperados fixos. */
static const size_t TAMANHOS_EXPERIMENTO[QUANTIDADE_TAMANHOS] = {100, 500, 1000};

static int falhas;

static void verificar(const char *descricao, int condicao)
{
    printf("%s: %s\n", descricao, condicao ? "PASSOU" : "FALHOU");
    falhas += !condicao;
}

static int indice_algoritmo(const char *nome)
{
    const char *nomes[] = {"BFS", "DFS", "Componentes"};
    for (int i = 0; i < QUANTIDADE_ALGORITMOS; i++) {
        if (strcmp(nome, nomes[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static int indice_tamanho(size_t vertices)
{
    for (int i = 0; i < QUANTIDADE_TAMANHOS; i++) {
        if (vertices == TAMANHOS_EXPERIMENTO[i]) {
            return i;
        }
    }
    return -1;
}

/* Calcula os bytes esperados pelas APIs das representacoes. A cadeia possui
 * N-1 arestas, como cada prefixo conectado do dataset em arvore da Fase I. */
static int calcular_memorias_esperadas(
    size_t memorias[QUANTIDADE_TAMANHOS][QUANTIDADE_REPRESENTACOES])
{
    for (int t = 0; t < QUANTIDADE_TAMANHOS; t++) {
        GrafoLista *lista = criar_grafo_lista();
        size_t vertices = TAMANHOS_EXPERIMENTO[t];
        size_t bytes_matriz = 0;
        if (lista == NULL ||
            estimar_memoria_matriz(vertices, &bytes_matriz) != MATRIZ_SUCESSO) {
            liberar_grafo_lista(lista);
            return 0;
        }
        for (size_t vertice = 0; vertice < vertices; vertice++) {
            size_t indice;
            if (inserir_vertice_lista(lista, &indice) != LISTA_SUCESSO ||
                indice != vertice) {
                liberar_grafo_lista(lista);
                return 0;
            }
        }
        for (size_t vertice = 1; vertice < vertices; vertice++) {
            if (inserir_aresta_lista(lista, vertice - 1, vertice) !=
                LISTA_SUCESSO) {
                liberar_grafo_lista(lista);
                return 0;
            }
        }
        memorias[t][0] = memoria_lista(lista);
        memorias[t][1] = bytes_matriz;
        liberar_grafo_lista(lista);
    }
    return 1;
}

static void testar_registro_invalido(void)
{
    FILE *arquivo = tmpfile();
    verificar("Arquivo temporario para registro", arquivo != NULL);
    if (arquivo == NULL) {
        return;
    }
    verificar("Rejeita tempo negativo",
              !registrar_metrica(arquivo, "BFS", "lista", 1, 0, -0.1, 0, 1));
    verificar("Rejeita tempo NaN",
              !registrar_metrica(arquivo, "BFS", "lista", 1, 0, NAN, 0, 1));
    verificar("Rejeita tempo infinito",
              !registrar_metrica(arquivo, "BFS", "lista", 1, 0, INFINITY, 0, 1));
    fclose(arquivo);
}

static void testar_csv_experimental(void)
{
    Dataset *dataset = NULL;
    ErroDataset erro;
    FILE *arquivo;
    char linha[256];
    int vistos[QUANTIDADE_TAMANHOS][QUANTIDADE_ALGORITMOS]
              [QUANTIDADE_REPRESENTACOES][REPETICOES_TESTE] = {{{{0}}}};
    size_t memorias_esperadas[QUANTIDADE_TAMANHOS]
                              [QUANTIDADE_REPRESENTACOES];
    size_t linhas = 0;
    int valido = carregar_dataset("data/forcel_2025_circuito_1/vertices.csv",
                                  "data/forcel_2025_circuito_1/arestas.csv",
                                  &dataset, &erro) == DATASET_SUCESSO;
    verificar("Carrega dataset dos experimentos", valido);
    if (!valido) {
        return;
    }

    valido = calcular_memorias_esperadas(memorias_esperadas);
    verificar("Calcula memoria esperada para N=100/500/1000", valido);
    if (!valido) {
        liberar_dataset(&dataset);
        return;
    }

    verificar("Executa duas repeticoes", executar_experimentos(
        dataset, ARQUIVO_TEMPORARIO, REPETICOES_TESTE));
    liberar_dataset(&dataset);

    arquivo = fopen(ARQUIVO_TEMPORARIO, "r");
    verificar("Abre CSV experimental", arquivo != NULL);
    if (arquivo == NULL) {
        return;
    }
    valido = fgets(linha, sizeof(linha), arquivo) != NULL &&
             strcmp(linha, "algoritmo,representacao,vertices,arestas,tempo_ms,"
                           "memoria_bytes,repeticao\n") == 0;
    verificar("Cabecalho CSV exato", valido);

    valido = 1;
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char algoritmo[32];
        char representacao[16];
        size_t vertices, arestas, memoria, repeticao;
        double tempo_ms;
        int consumidos = 0;
        int a, t, r;
        int p;
        linha[strcspn(linha, "\r\n")] = '\0';
        if (sscanf(linha, "%31[^,],%15[^,],%zu,%zu,%lf,%zu,%zu%n",
                   algoritmo, representacao, &vertices, &arestas, &tempo_ms,
                   &memoria, &repeticao, &consumidos) != 7 ||
            linha[consumidos] != '\0') {
            valido = 0;
            continue;
        }
        a = indice_algoritmo(algoritmo);
        t = indice_tamanho(vertices);
        p = strcmp(representacao, "lista") == 0 ? 0 :
            strcmp(representacao, "matriz") == 0 ? 1 : -1;
        r = repeticao >= 1 && repeticao <= REPETICOES_TESTE ?
            (int)repeticao - 1 : -1;
        if (a < 0 || t < 0 || p < 0 || r < 0 || !isfinite(tempo_ms) ||
            tempo_ms < 0.0 || arestas != vertices - 1 ||
            memoria != memorias_esperadas[t][p]) {
            valido = 0;
            continue;
        }
        vistos[t][a][p][r]++;
        linhas++;
    }
    verificar("Linhas com metadados e tempos validos", valido);
    verificar("Quantidade de linhas",
              linhas == QUANTIDADE_TAMANHOS * QUANTIDADE_ALGORITMOS *
                        QUANTIDADE_REPRESENTACOES * REPETICOES_TESTE);
    valido = 1;
    for (int t = 0; t < QUANTIDADE_TAMANHOS; t++) {
        for (int a = 0; a < QUANTIDADE_ALGORITMOS; a++) {
            for (int p = 0; p < QUANTIDADE_REPRESENTACOES; p++) {
                for (int r = 0; r < REPETICOES_TESTE; r++) {
                    valido = valido && vistos[t][a][p][r] == 1;
                }
            }
        }
    }
    verificar("Cada repeticao aparece uma vez", valido);
    fclose(arquivo);
    remove(ARQUIVO_TEMPORARIO);
}

int main(void)
{
    testar_registro_invalido();
    testar_csv_experimental();
    printf("Total de falhas: %d\n", falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
