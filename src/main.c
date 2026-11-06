#include "algoritmos_complementares.h"
#include "bfs.h"
#include "componentes_conexos.h"
#include "dataset.h"
#include "dijkstra.h"
#include "dfs.h"
#include "metricas.h"
#include "simulacao_falha.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERTICES_PADRAO "data/forcel_2025_circuito_1/vertices.csv"
#define ARESTAS_PADRAO "data/forcel_2025_circuito_1/arestas.csv"
#define FALHA_ORIGEM "72352"
#define FALHA_DESTINO "12692"

typedef enum { REPRESENTACAO_LISTA = 1, REPRESENTACAO_MATRIZ = 2 } Representacao;

typedef struct {
    const Dataset *dataset;
    GrafoLista *lista;
    MatrizAdjacencia *matriz;
    Representacao representacao;
    char ultima_metrica[200];
} Aplicacao;

static int ler_opcao(void)
{
    char linha[64];
    char *fim;
    long valor;
    if (fgets(linha, sizeof(linha), stdin) == NULL) {
        return 0;
    }
    valor = strtol(linha, &fim, 10);
    if (fim == linha || (*fim != '\n' && *fim != '\0')) {
        return -1;
    }
    return (int)valor;
}

static int ler_texto(char *texto, size_t capacidade)
{
    size_t tamanho;
    if (texto == NULL || capacidade == 0 || fgets(texto, capacidade, stdin) == NULL)
        return 0;
    tamanho = strlen(texto);
    if (tamanho != 0 && texto[tamanho - 1] == '\n') texto[--tamanho] = '\0';
    if (tamanho != 0 && texto[tamanho - 1] == '\r') texto[--tamanho] = '\0';
    return tamanho != 0;
}

static int ler_custo(double *custo)
{
    char linha[128];
    char *fim;
    double valor;
    if (custo == NULL || !ler_texto(linha, sizeof(linha))) return 0;
    valor = strtod(linha, &fim);
    if (fim == linha || *fim != '\0' || !isfinite(valor) || valor < 0.0)
        return 0;
    *custo = valor;
    return 1;
}

static const char *nome_representacao(Representacao representacao)
{
    return representacao == REPRESENTACAO_LISTA ? "Lista de Adjacencia" :
                                                   "Matriz de Adjacencia";
}

static void guardar_metrica(Aplicacao *app, const char *algoritmo,
                            double tempo_ms)
{
    size_t memoria = app->representacao == REPRESENTACAO_LISTA ?
        memoria_lista(app->lista) : memoria_matriz(app->matriz);
    snprintf(app->ultima_metrica, sizeof(app->ultima_metrica),
             "%s,%s,%zu,%zu,%.6f,%zu", algoritmo,
             app->representacao == REPRESENTACAO_LISTA ? "lista" : "matriz",
             app->dataset->quantidade_vertices, app->dataset->quantidade_arestas,
             tempo_ms, memoria);
    printf("Tempo: %.6f ms | Memoria estimada da representacao: %zu bytes\n",
           tempo_ms, memoria);
}

static int executar_bfs(Aplicacao *app)
{
    ResultadoBfs *resultado = NULL;
    double inicio = tempo_atual_milissegundos();
    int sucesso = app->representacao == REPRESENTACAO_LISTA ?
        executar_bfs_lista(app->lista, app->dataset->indice_subestacao,
                           &resultado) == BFS_SUCESSO :
        executar_bfs_matriz(app->matriz, app->dataset->indice_subestacao,
                            &resultado) == BFS_SUCESSO;
    double tempo = tempo_atual_milissegundos() - inicio;
    if (sucesso) {
        printf("BFS alcancou %zu de %zu vertices.\n",
               resultado->quantidade_alcancados, resultado->quantidade_vertices);
        guardar_metrica(app, "BFS", tempo);
    }
    liberar_resultado_bfs(&resultado);
    return sucesso;
}

static int executar_dfs(Aplicacao *app)
{
    PercursoDFS *resultado = NULL;
    double inicio = tempo_atual_milissegundos();
    int sucesso = app->representacao == REPRESENTACAO_LISTA ?
        executar_dfs_lista(app->lista, app->dataset->indice_subestacao,
                           &resultado) == DFS_SUCESSO :
        executar_dfs_matriz(app->matriz, app->dataset->indice_subestacao,
                            &resultado) == DFS_SUCESSO;
    double tempo = tempo_atual_milissegundos() - inicio;
    if (sucesso) {
        printf("DFS alcancou %zu de %zu vertices.\n",
               resultado->quantidade_alcancados, resultado->quantidade_vertices);
        guardar_metrica(app, "DFS", tempo);
    }
    liberar_percurso_dfs(&resultado);
    return sucesso;
}

static int executar_componentes(Aplicacao *app)
{
    ComponentesConexos *resultado = NULL;
    double inicio = tempo_atual_milissegundos();
    int sucesso = app->representacao == REPRESENTACAO_LISTA ?
        identificar_componentes_conexos_lista(app->lista, &resultado) ==
            COMPONENTES_SUCESSO :
        identificar_componentes_conexos_matriz(app->matriz, &resultado) ==
            COMPONENTES_SUCESSO;
    double tempo = tempo_atual_milissegundos() - inicio;
    if (sucesso) {
        printf("Componentes conexos: %zu. Componente da subestacao: %zu vertices.\n",
               resultado->quantidade_componentes,
               resultado->tamanhos_componentes[
                   resultado->componente_por_vertice[app->dataset->indice_subestacao]]);
        guardar_metrica(app, "Componentes", tempo);
    }
    liberar_componentes_conexos(&resultado);
    return sucesso;
}

static int simular_falha(Aplicacao *app, int exportar_dot)
{
    ResultadoSimulacaoFalha *resultado = NULL;
    size_t origem, destino;
    double inicio;
    StatusSimulacaoFalha status;
    if (!buscar_indice_dataset(app->dataset, FALHA_ORIGEM, &origem) ||
        !buscar_indice_dataset(app->dataset, FALHA_DESTINO, &destino)) {
        origem = app->dataset->arestas[0].origem;
        destino = app->dataset->arestas[0].destino;
    }
    printf("Falha real selecionada: %s -- %s (indices %zu -- %zu).\n",
           app->dataset->vertices[origem].id_externo,
           app->dataset->vertices[destino].id_externo, origem, destino);
    inicio = tempo_atual_milissegundos();
    status = app->representacao == REPRESENTACAO_LISTA ?
        simular_falha_aresta_lista(app->lista, origem, destino,
                                   app->dataset->indice_subestacao, &resultado) :
        simular_falha_aresta_matriz(app->matriz, origem, destino,
                                    app->dataset->indice_subestacao, &resultado);
    if (status != SIMULACAO_SUCESSO) {
        printf("Falha na simulacao: %s\n", descricao_status_simulacao(status));
        return 0;
    }
    printf("Antes: %zu componente(s); depois: %zu; restaurado: %zu.\n",
           resultado->componentes_antes->quantidade_componentes,
           resultado->componentes_depois->quantidade_componentes,
           resultado->componentes_apos_restauracao->quantidade_componentes);
    printf("Vertices fora da regiao da subestacao: %zu; topologia restaurada: %s.\n",
           resultado->quantidade_isolados,
           resultado->topologia_restaurada ? "sim" : "nao");
    guardar_metrica(app, "SimulacaoFalha",
                    tempo_atual_milissegundos() - inicio);
    if (exportar_dot) {
        FILE *dot = fopen("resultados/simulacao_falha.dot", "w");
        if (dot != NULL) {
            int ok = app->representacao == REPRESENTACAO_LISTA ?
                exportar_simulacao_dot_lista(app->lista, resultado,
                    REDE_DEPOIS_FALHA, dot) :
                exportar_simulacao_dot_matriz(app->matriz, resultado,
                    REDE_DEPOIS_FALHA, dot);
            if (fclose(dot) == 0 && ok) {
                puts("DOT salvo em resultados/simulacao_falha.dot");
            }
        }
    }
    liberar_resultado_simulacao(&resultado);
    return 1;
}

static void executar_complementares(Aplicacao *app)
{
    ResultadoCriticidade *criticidade = NULL;
    ResultadoColoracao *coloracao = NULL;
    if (analisar_criticidade_lista(app->lista, &criticidade) == ALGORITMO_SUCESSO &&
        colorir_grafo_guloso(app->lista, &coloracao) == ALGORITMO_SUCESSO) {
        printf("Articulacoes: %zu | Pontes: %zu | Cores gulosas: %zu\n",
               criticidade->quantidade_articulacoes,
               criticidade->quantidade_pontes, coloracao->quantidade_cores);
    }
    puts("Conectividade forte exige digrafo; Euler exige uma representacao planar com faces conhecidas.");
    liberar_resultado_criticidade(&criticidade);
    liberar_resultado_coloracao(&coloracao);
}

static void executar_dijkstra_cli(Aplicacao *app)
{
    char id_origem[256];
    char id_destino[256];
    size_t origem, destino;
    int modo;
    ConfiguracaoPesosDataset configuracao = {
        PESO_AUSENTE_MANTER_SEM_PESO, 0.0
    };
    PoliticaPesoDijkstra politica = DIJKSTRA_REJEITAR_ARESTA_SEM_PESO;
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoDijkstra *resultado = NULL;
    StatusDataset status_dataset;
    StatusDijkstra status;
    double inicio;

    printf("ID externo de origem: ");
    if (!ler_texto(id_origem, sizeof(id_origem)) ||
        !buscar_indice_dataset(app->dataset, id_origem, &origem)) {
        puts("Vertice de origem inexistente no dataset.");
        return;
    }
    printf("ID externo de destino: ");
    if (!ler_texto(id_destino, sizeof(id_destino)) ||
        !buscar_indice_dataset(app->dataset, id_destino, &destino)) {
        puts("Vertice de destino inexistente no dataset.");
        return;
    }
    printf("Arestas sem comprimento: 1-rejeitar grafo, 2-ignorar arestas, "
           "3-usar custo assumido: ");
    modo = ler_opcao();
    if (modo == 2) {
        politica = DIJKSTRA_IGNORAR_ARESTA_SEM_PESO;
    } else if (modo == 3) {
        configuracao.modo_peso_ausente = PESO_AUSENTE_USAR_CUSTO_ASSUMIDO;
        printf("Custo abstrato assumido (finito e >= 0): ");
        if (!ler_custo(&configuracao.custo_assumido)) {
            puts("Custo assumido invalido.");
            return;
        }
    } else if (modo != 1) {
        puts("Modo de peso invalido.");
        return;
    }

    inicio = tempo_atual_milissegundos();
    if (app->representacao == REPRESENTACAO_LISTA) {
        status_dataset = construir_lista_ponderada_dataset(
            app->dataset, &configuracao, &lista);
        if (status_dataset == DATASET_SUCESSO)
            status = executar_dijkstra_lista(lista, origem, destino, politica,
                                              &resultado);
        else
            status = DIJKSTRA_ERRO_GRAFO;
    } else {
        status_dataset = construir_matriz_ponderada_dataset(
            app->dataset, &configuracao, &matriz);
        if (status_dataset == DATASET_SUCESSO)
            status = executar_dijkstra_matriz(matriz, origem, destino, politica,
                                               &resultado);
        else
            status = DIJKSTRA_ERRO_GRAFO;
    }
    if (status_dataset != DATASET_SUCESSO) {
        printf("Falha ao construir grafo ponderado: %s\n",
               descricao_status_dataset(status_dataset));
    } else if (status != DIJKSTRA_SUCESSO) {
        printf("Dijkstra nao executado: %s.\n",
               descricao_status_dijkstra(status));
    } else if (!resultado->alcancavel) {
        printf("Destino inalcançavel. Vertices processados: %zu.\n",
               resultado->quantidade_vertices_processados);
    } else {
        printf("Caminho (%zu vertices): ",
               resultado->quantidade_vertices_caminho);
        for (size_t i = 0; i < resultado->quantidade_vertices_caminho; i++) {
            if (i != 0) printf(" -> ");
            printf("%s", app->dataset->vertices[resultado->caminho[i]].id_externo);
        }
        printf("\nCusto acumulado: %.6f\nVertices processados: %zu\n",
               resultado->custo_total,
               resultado->quantidade_vertices_processados);
        if (resultado->utiliza_custo_assumido) {
            puts("CENARIO ABSTRATO: o custo inclui valor assumido e nao "
                 "representa distancia fisica real em metros.");
        } else {
            puts("O caminho usa somente comprimentos medidos; custo em metros.");
        }
    }
    if (status_dataset == DATASET_SUCESSO)
        printf("Tempo total (construcao + Dijkstra): %.6f ms\n",
               tempo_atual_milissegundos() - inicio);
    liberar_resultado_dijkstra(&resultado);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

static int executar_demo(Aplicacao *app)
{
    puts("DEMONSTRACAO AUTOMATICA");
    app->representacao = REPRESENTACAO_LISTA;
    if (!executar_bfs(app) || !executar_dfs(app) || !executar_componentes(app) ||
        !simular_falha(app, 1)) return 0;
    app->representacao = REPRESENTACAO_MATRIZ;
    return executar_bfs(app) && executar_dfs(app) &&
           executar_componentes(app) && simular_falha(app, 0);
}

static void menu(Aplicacao *app)
{
    int opcao = -1;
    while (opcao != 0) {
        printf("\n========================================\n SMART GRID - FASE I E II\n"
               "========================================\n"
               "Dataset: %zu vertices, %zu arestas\nSubestacao: %s (%s)\n"
               "Representacao atual: [%s]\n\n"
               "1 - Alterar representacao\n2 - Executar BFS\n3 - Executar DFS\n"
               "4 - Identificar componentes conexos\n5 - Simular falha real\n"
               "6 - Mostrar ultima metrica\n7 - Exportar simulacao Graphviz\n"
               "8 - Algoritmos complementares\n9 - Caminho minimo (Dijkstra)\n"
               "0 - Sair\nOpcao: ",
               app->dataset->quantidade_vertices, app->dataset->quantidade_arestas,
               app->dataset->vertices[app->dataset->indice_subestacao].subestacao,
               app->dataset->vertices[app->dataset->indice_subestacao].id_externo,
               nome_representacao(app->representacao));
        opcao = ler_opcao();
        switch (opcao) {
        case 1:
            app->representacao = app->representacao == REPRESENTACAO_LISTA ?
                REPRESENTACAO_MATRIZ : REPRESENTACAO_LISTA;
            break;
        case 2: executar_bfs(app); break;
        case 3: executar_dfs(app); break;
        case 4: executar_componentes(app); break;
        case 5: simular_falha(app, 0); break;
        case 6:
            puts(app->ultima_metrica[0] ? app->ultima_metrica :
                 "Nenhuma metrica registrada nesta sessao.");
            break;
        case 7: simular_falha(app, 1); break;
        case 8: executar_complementares(app); break;
        case 9: executar_dijkstra_cli(app); break;
        case 0: break;
        default: puts("Opcao invalida; informe um numero do menu.");
        }
    }
}

int main(int argc, char **argv)
{
    Dataset *dataset = NULL;
    ErroDataset erro;
    Aplicacao app = {0};
    StatusDataset status = carregar_dataset(VERTICES_PADRAO, ARESTAS_PADRAO,
                                             &dataset, &erro);
    if (status != DATASET_SUCESSO) {
        fprintf(stderr, "Erro ao carregar dataset: %s (linha %zu: %s)\n",
                descricao_status_dataset(status), erro.linha, erro.mensagem);
        return EXIT_FAILURE;
    }
    app.dataset = dataset;
    app.representacao = REPRESENTACAO_LISTA;
    if (construir_lista_dataset(dataset, &app.lista) != DATASET_SUCESSO ||
        construir_matriz_dataset(dataset, &app.matriz) != DATASET_SUCESSO) {
        fputs("Nao foi possivel construir as duas representacoes.\n", stderr);
        liberar_grafo_lista(app.lista);
        destruir_matriz(&app.matriz);
        liberar_dataset(&dataset);
        return EXIT_FAILURE;
    }
    if (argc > 1 && strcmp(argv[1], "--experimentos") == 0) {
        if (!executar_experimentos(dataset, "resultados/fase1_metricas.csv", 7)) {
            fputs("Falha nos experimentos.\n", stderr);
            status = DATASET_ERRO_GRAFO;
        } else {
            puts("Experimentos concluidos: resultados/fase1_metricas.csv");
        }
    } else if (argc > 1 && strcmp(argv[1], "--demo") == 0) {
        if (!executar_demo(&app)) status = DATASET_ERRO_GRAFO;
    } else {
        menu(&app);
    }
    liberar_grafo_lista(app.lista);
    destruir_matriz(&app.matriz);
    liberar_dataset(&dataset);
    return status == DATASET_SUCESSO ? EXIT_SUCCESS : EXIT_FAILURE;
}
