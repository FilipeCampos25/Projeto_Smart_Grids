#include "metricas.h"

#include "bfs.h"
#include "componentes_conexos.h"
#include "dfs.h"

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

double tempo_atual_milissegundos(void)
{
#ifdef _WIN32
    LARGE_INTEGER contador;
    LARGE_INTEGER frequencia;
    if (QueryPerformanceFrequency(&frequencia) && QueryPerformanceCounter(&contador)) {
        return (double)contador.QuadPart * 1000.0 / (double)frequencia.QuadPart;
    }
#else
    struct timespec instante;
    if (timespec_get(&instante, TIME_UTC) != TIME_UTC) {
        return 1000.0 * (double)clock() / (double)CLOCKS_PER_SEC;
    }
    return (double)instante.tv_sec * 1000.0 +
           (double)instante.tv_nsec / 1000000.0;
#endif
    return 1000.0 * (double)clock() / (double)CLOCKS_PER_SEC;
}

int registrar_metrica(FILE *arquivo, const char *algoritmo,
                      const char *representacao, size_t vertices,
                      size_t arestas, double tempo_ms, size_t memoria_bytes,
                      size_t repeticao)
{
    return arquivo != NULL && algoritmo != NULL && representacao != NULL &&
           fprintf(arquivo, "%s,%s,%zu,%zu,%.6f,%zu,%zu\n", algoritmo,
                   representacao, vertices, arestas, tempo_ms, memoria_bytes,
                   repeticao) >= 0;
}

static int construir_subgrafos(const Dataset *dataset, size_t n,
                               GrafoLista **lista, MatrizAdjacencia **matriz,
                               size_t *quantidade_arestas)
{
    size_t bytes;
    *lista = criar_grafo_lista();
    if (*lista == NULL || estimar_memoria_matriz(n, &bytes) != MATRIZ_SUCESSO ||
        criar_matriz(n, bytes, matriz) != MATRIZ_SUCESSO) {
        liberar_grafo_lista(*lista);
        *lista = NULL;
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        size_t indice;
        if (inserir_vertice_lista(*lista, &indice) != LISTA_SUCESSO) {
            liberar_grafo_lista(*lista);
            destruir_matriz(matriz);
            *lista = NULL;
            return 0;
        }
    }
    *quantidade_arestas = 0;
    for (size_t i = 0; i < dataset->quantidade_arestas; i++) {
        const ArestaDataset *aresta = &dataset->arestas[i];
        if (aresta->origem < n && aresta->destino < n) {
            if (inserir_aresta_lista(*lista, aresta->origem, aresta->destino) !=
                    LISTA_SUCESSO ||
                inserir_aresta(*matriz, aresta->origem, aresta->destino) !=
                    MATRIZ_SUCESSO) {
                liberar_grafo_lista(*lista);
                destruir_matriz(matriz);
                *lista = NULL;
                return 0;
            }
            (*quantidade_arestas)++;
        }
    }
    return 1;
}

static int medir_lista(FILE *arquivo, const char *algoritmo,
                       const GrafoLista *lista, size_t n, size_t arestas,
                       size_t repeticao)
{
    double inicio = tempo_atual_milissegundos();
    double fim;
    int sucesso = 0;
    if (algoritmo[0] == 'B') {
        ResultadoBfs *resultado = NULL;
        sucesso = executar_bfs_lista(lista, 0, &resultado) == BFS_SUCESSO;
        fim = tempo_atual_milissegundos();
        liberar_resultado_bfs(&resultado);
    } else if (algoritmo[0] == 'D') {
        PercursoDFS *resultado = NULL;
        sucesso = executar_dfs_lista(lista, 0, &resultado) == DFS_SUCESSO;
        fim = tempo_atual_milissegundos();
        liberar_percurso_dfs(&resultado);
    } else {
        ComponentesConexos *resultado = NULL;
        sucesso = identificar_componentes_conexos_lista(lista, &resultado) ==
                  COMPONENTES_SUCESSO;
        fim = tempo_atual_milissegundos();
        liberar_componentes_conexos(&resultado);
    }
    return sucesso && registrar_metrica(arquivo, algoritmo, "lista", n,
        arestas, fim - inicio, memoria_lista(lista), repeticao);
}

static int medir_matriz(FILE *arquivo, const char *algoritmo,
                        const MatrizAdjacencia *matriz, size_t n,
                        size_t arestas, size_t repeticao)
{
    double inicio = tempo_atual_milissegundos();
    double fim;
    int sucesso = 0;
    if (algoritmo[0] == 'B') {
        ResultadoBfs *resultado = NULL;
        sucesso = executar_bfs_matriz(matriz, 0, &resultado) == BFS_SUCESSO;
        fim = tempo_atual_milissegundos();
        liberar_resultado_bfs(&resultado);
    } else if (algoritmo[0] == 'D') {
        PercursoDFS *resultado = NULL;
        sucesso = executar_dfs_matriz(matriz, 0, &resultado) == DFS_SUCESSO;
        fim = tempo_atual_milissegundos();
        liberar_percurso_dfs(&resultado);
    } else {
        ComponentesConexos *resultado = NULL;
        sucesso = identificar_componentes_conexos_matriz(matriz, &resultado) ==
                  COMPONENTES_SUCESSO;
        fim = tempo_atual_milissegundos();
        liberar_componentes_conexos(&resultado);
    }
    return sucesso && registrar_metrica(arquivo, algoritmo, "matriz", n,
        arestas, fim - inicio, memoria_matriz(matriz), repeticao);
}

int executar_experimentos(const Dataset *dataset, const char *arquivo_csv,
                          size_t repeticoes)
{
    const size_t tamanhos[] = {100, 500, 1000};
    const char *algoritmos[] = {"BFS", "DFS", "Componentes"};
    FILE *arquivo;
    if (dataset == NULL || arquivo_csv == NULL || repeticoes == 0 ||
        dataset->quantidade_vertices < 1000 || dataset->indice_subestacao != 0) {
        return 0;
    }
    arquivo = fopen(arquivo_csv, "w");
    if (arquivo == NULL) {
        return 0;
    }
    fputs("algoritmo,representacao,vertices,arestas,tempo_ms,memoria_bytes,repeticao\n",
          arquivo);
    for (size_t t = 0; t < sizeof(tamanhos) / sizeof(tamanhos[0]); t++) {
        GrafoLista *lista = NULL;
        MatrizAdjacencia *matriz = NULL;
        size_t arestas;
        if (!construir_subgrafos(dataset, tamanhos[t], &lista, &matriz, &arestas)) {
            fclose(arquivo);
            return 0;
        }
        for (size_t r = 1; r <= repeticoes; r++) {
            for (size_t a = 0; a < 3; a++) {
                if (!medir_lista(arquivo, algoritmos[a], lista, tamanhos[t],
                                 arestas, r) ||
                    !medir_matriz(arquivo, algoritmos[a], matriz, tamanhos[t],
                                  arestas, r)) {
                    liberar_grafo_lista(lista);
                    destruir_matriz(&matriz);
                    fclose(arquivo);
                    return 0;
                }
            }
        }
        liberar_grafo_lista(lista);
        destruir_matriz(&matriz);
    }
    return fclose(arquivo) == 0;
}
