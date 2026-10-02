#include "bfs.h"
#include "componentes_conexos.h"
#include "dataset.h"
#include "dfs.h"
#include "simulacao_falha.h"

#include <stdio.h>
#include <stdlib.h>

static size_t verificacoes;
static size_t falhas;

static void verificar(const char *descricao, size_t esperado, size_t obtido)
{
    verificacoes++;
    printf("%s: RESULTADO ESPERADO=%zu RESULTADO OBTIDO=%zu %s\n",
           descricao, esperado, obtido, esperado == obtido ? "PASSOU" : "FALHOU");
    if (esperado != obtido) falhas++;
}

static size_t comparar_topologia(const GrafoLista *lista,
                                 const MatrizAdjacencia *matriz, size_t n)
{
    size_t divergencias = 0;
    for (size_t origem = 0; origem < n; origem++) {
        for (size_t destino = 0; destino < n; destino++) {
            int na_lista = 0;
            int na_matriz = 0;
            existe_aresta_lista(lista, origem, destino, &na_lista);
            consultar_adjacencia(matriz, origem, destino, &na_matriz);
            divergencias += na_lista != na_matriz;
        }
    }
    return divergencias;
}

static size_t comparar_particoes(const ComponentesConexos *a,
                                 const ComponentesConexos *b)
{
    size_t divergencias = 0;
    if (a->quantidade_vertices != b->quantidade_vertices) return 1;
    for (size_t i = 0; i < a->quantidade_vertices; i++) {
        for (size_t j = i; j < a->quantidade_vertices; j++) {
            int juntos_a = a->componente_por_vertice[i] == a->componente_por_vertice[j];
            int juntos_b = b->componente_por_vertice[i] == b->componente_por_vertice[j];
            divergencias += juntos_a != juntos_b;
        }
    }
    return divergencias;
}

int main(void)
{
    Dataset *dataset = NULL;
    Dataset *invalido = NULL;
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoBfs *bfs_lista = NULL, *bfs_matriz = NULL;
    PercursoDFS *dfs_lista = NULL, *dfs_matriz = NULL;
    ComponentesConexos *componentes_lista = NULL, *componentes_matriz = NULL;
    ResultadoSimulacaoFalha *simulacao = NULL;
    ErroDataset erro;
    size_t origem, destino;
    size_t divergencias;

    verificar("Carregar recorte real", DATASET_SUCESSO,
        carregar_dataset("data/forcel_2025_circuito_1/vertices.csv",
                         "data/forcel_2025_circuito_1/arestas.csv",
                         &dataset, &erro));
    if (dataset == NULL) return EXIT_FAILURE;
    verificar("Vertices reais", 1838, dataset->quantidade_vertices);
    verificar("Arestas reais", 1837, dataset->quantidade_arestas);
    verificar("Subestacao no primeiro indice", 0, dataset->indice_subestacao);
    verificar("Construir lista", DATASET_SUCESSO,
              construir_lista_dataset(dataset, &lista));
    verificar("Construir matriz", DATASET_SUCESSO,
              construir_matriz_dataset(dataset, &matriz));
    verificar("Mesma quantidade de vertices", quantidade_vertices_lista(lista),
              quantidade_vertices_matriz(matriz));
    verificar("Mesma topologia", 0,
              comparar_topologia(lista, matriz, dataset->quantidade_vertices));

    verificar("BFS lista", BFS_SUCESSO,
              executar_bfs_lista(lista, dataset->indice_subestacao, &bfs_lista));
    verificar("BFS matriz", BFS_SUCESSO,
              executar_bfs_matriz(matriz, dataset->indice_subestacao, &bfs_matriz));
    verificar("BFS lista alcanca todos", 1838, bfs_lista->quantidade_alcancados);
    verificar("BFS matriz alcanca todos", 1838, bfs_matriz->quantidade_alcancados);
    divergencias = 0;
    for (size_t i = 0; i < dataset->quantidade_vertices; i++)
        divergencias += (bfs_lista->distancias[i] == SIZE_MAX) !=
                        (bfs_matriz->distancias[i] == SIZE_MAX);
    verificar("Mesmo conjunto BFS", 0, divergencias);

    verificar("DFS lista", DFS_SUCESSO,
              executar_dfs_lista(lista, dataset->indice_subestacao, &dfs_lista));
    verificar("DFS matriz", DFS_SUCESSO,
              executar_dfs_matriz(matriz, dataset->indice_subestacao, &dfs_matriz));
    verificar("DFS lista alcanca todos", 1838, dfs_lista->quantidade_alcancados);
    verificar("DFS matriz alcanca todos", 1838, dfs_matriz->quantidade_alcancados);
    divergencias = 0;
    for (size_t i = 0; i < dataset->quantidade_vertices; i++)
        divergencias += dfs_lista->visitados[i] != dfs_matriz->visitados[i];
    verificar("Mesmo conjunto DFS", 0, divergencias);

    verificar("Componentes lista", COMPONENTES_SUCESSO,
        identificar_componentes_conexos_lista(lista, &componentes_lista));
    verificar("Componentes matriz", COMPONENTES_SUCESSO,
        identificar_componentes_conexos_matriz(matriz, &componentes_matriz));
    verificar("Uma componente real na lista", 1,
              componentes_lista->quantidade_componentes);
    verificar("Uma componente real na matriz", 1,
              componentes_matriz->quantidade_componentes);
    verificar("Mesma particao logica", 0,
              comparar_particoes(componentes_lista, componentes_matriz));

    buscar_indice_dataset(dataset, "72352", &origem);
    buscar_indice_dataset(dataset, "12692", &destino);
    verificar("Simulacao real", SIMULACAO_SUCESSO,
        simular_falha_aresta_lista(lista, origem, destino,
                                   dataset->indice_subestacao, &simulacao));
    verificar("Falha cria duas componentes", 2,
              simulacao->componentes_depois->quantidade_componentes);
    verificar("Ilha real identificada", 689, simulacao->quantidade_isolados);
    verificar("Restauracao confirmada", 1, simulacao->topologia_restaurada);
    {
        int existe = 0;
        existe_aresta_lista(lista, origem, destino, &existe);
        verificar("Aresta restaurada", 1, existe);
    }

    verificar("Referencia invalida rejeitada", DATASET_REFERENCIA_INVALIDA,
        carregar_dataset("tests/dados/vertices_invalidos.csv",
                         "tests/dados/arestas_invalidas.csv", &invalido, &erro));
    verificar("Carga invalida nao entrega objeto", 0, invalido != NULL);

    liberar_resultado_simulacao(&simulacao);
    liberar_componentes_conexos(&componentes_lista);
    liberar_componentes_conexos(&componentes_matriz);
    liberar_percurso_dfs(&dfs_lista);
    liberar_percurso_dfs(&dfs_matriz);
    liberar_resultado_bfs(&bfs_lista);
    liberar_resultado_bfs(&bfs_matriz);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
    liberar_dataset(&dataset);
    printf("\nRESULTADO DATASET REAL: %s (%zu verificacoes, %zu falhas)\n",
           falhas == 0 ? "PASSOU" : "FALHOU", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
