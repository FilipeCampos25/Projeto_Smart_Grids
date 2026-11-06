#include "algoritmos_complementares.h"
#include "agm.h"
#include "bfs.h"
#include "componentes_conexos.h"
#include "dataset.h"
#include "dfs.h"
#include "metricas.h"
#include "simulacao_falha.h"

#include <errno.h>
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

static int ler_linha(char *linha, size_t capacidade)
{
    size_t tamanho;
    if (linha == NULL || capacidade < 2 ||
        fgets(linha, (int)capacidade, stdin) == NULL) {
        return 0;
    }
    tamanho = strlen(linha);
    if (tamanho > 0 && linha[tamanho - 1] == '\n') {
        linha[--tamanho] = '\0';
    } else if (!feof(stdin)) {
        int caractere;
        while ((caractere = getchar()) != '\n' && caractere != EOF) {
        }
        return -1;
    }
    if (tamanho > 0 && linha[tamanho - 1] == '\r') {
        linha[tamanho - 1] = '\0';
    }
    return 1;
}

static int ler_inteiro(const char *prompt, long minimo, long maximo,
                       long *valor)
{
    char linha[128];
    char *fim;
    long convertido;
    int status;
    fputs(prompt, stdout);
    fflush(stdout);
    status = ler_linha(linha, sizeof(linha));
    if (status <= 0) {
        return status;
    }
    errno = 0;
    convertido = strtol(linha, &fim, 10);
    while (*fim == ' ' || *fim == '\t') {
        fim++;
    }
    if (errno == ERANGE || fim == linha || *fim != '\0' ||
        convertido < minimo || convertido > maximo) {
        return -1;
    }
    *valor = convertido;
    return 1;
}

static int ler_double_nao_negativo(const char *prompt, double *valor)
{
    char linha[128];
    char *fim;
    double convertido;
    int status;
    fputs(prompt, stdout);
    fflush(stdout);
    status = ler_linha(linha, sizeof(linha));
    if (status <= 0) {
        return status;
    }
    errno = 0;
    convertido = strtod(linha, &fim);
    while (*fim == ' ' || *fim == '\t') {
        fim++;
    }
    if (errno == ERANGE || fim == linha || *fim != '\0' ||
        !isfinite(convertido) || convertido < 0.0) {
        return -1;
    }
    *valor = convertido;
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

static const char *nome_status_agm(StatusAGM status)
{
    switch (status) {
    case AGM_SUCESSO: return "sucesso";
    case AGM_ARGUMENTO_INVALIDO: return "argumento ou resultado invalido";
    case AGM_SEM_MEMORIA: return "memoria insuficiente";
    case AGM_LIMITE_EXCEDIDO: return "limite de representacao excedido";
    case AGM_PESO_AUSENTE: return "existe aresta sem peso";
    case AGM_PESO_INVALIDO: return "peso ou origem invalida";
    case AGM_GRAFO_DIRECIONADO: return "AGM nao aceita matriz direcionada";
    case AGM_OVERFLOW_CUSTO: return "overflow no custo acumulado";
    default: return "status AGM desconhecido";
    }
}

static int executar_agm(Aplicacao *app, int interativo)
{
    long algoritmo = 1;
    long representacao = app->representacao;
    long politica_peso = 1;
    double custo_assumido = 0.0;
    size_t arestas_com_peso = 0;
    size_t arestas_sem_peso = 0;
    size_t arestas_metradas = 0;
    size_t arestas_assumidas = 0;
    double custo_medido = 0.0;
    double custo_assumido_selecionado = 0.0;
    ConfiguracaoPesosDataset configuracao = {
        PESO_AUSENTE_MANTER_SEM_PESO, 0.0
    };
    GrafoLista *lista_ponderada = NULL;
    MatrizAdjacencia *matriz_ponderada = NULL;
    ResultadoAGM resultado = {0};
    StatusDataset status_dataset;
    StatusAGM status_agm;
    double inicio;
    double tempo;

    if (interativo) {
        int leitura = ler_inteiro(
            "Algoritmo: 1=Prim, 2=Kruskal: ", 1, 2, &algoritmo);
        if (leitura <= 0) {
            if (leitura == 0) return 1;
            puts("Entrada invalida; escolha Prim (1) ou Kruskal (2).");
            return 0;
        }
        leitura = ler_inteiro(
            "Representacao: 1=Lista, 2=Matriz: ", 1, 2, &representacao);
        if (leitura <= 0) {
            if (leitura == 0) return 1;
            puts("Entrada invalida; escolha Lista (1) ou Matriz (2).");
            return 0;
        }
    }

    for (size_t indice = 0; indice < app->dataset->quantidade_arestas;
         indice++) {
        const ArestaDataset *aresta = &app->dataset->arestas[indice];
        if (!aresta->possui_comprimento) {
            arestas_sem_peso++;
        } else {
            arestas_com_peso++;
        }
    }
    printf("Pesos no dataset: %zu comprimentos medidos em metros; "
           "%zu arestas sem peso.\n", arestas_com_peso, arestas_sem_peso);
    if (interativo) {
        int leitura = ler_inteiro(
            "Arestas sem comprimento: 1=manter sem peso, "
            "2=informar custo assumido: ", 1, 2, &politica_peso);
        if (leitura <= 0) {
            if (leitura == 0) return 1;
            puts("Entrada invalida; informe 1 ou 2.");
            return 0;
        }
        if (politica_peso == 2) {
            leitura = ler_double_nao_negativo(
                "Valor abstrato a atribuir a cada aresta faltante "
                "(nao representa metros): ", &custo_assumido);
            if (leitura <= 0) {
                if (leitura == 0) return 1;
                puts("Custo invalido; informe um numero finito nao negativo.");
                return 0;
            }
        }
    }
    if (politica_peso == 2) {
        configuracao.modo_peso_ausente = PESO_AUSENTE_USAR_CUSTO_ASSUMIDO;
        configuracao.custo_assumido = custo_assumido;
        puts("AVISO: a solucao combina comprimentos medidos em metros com "
             "custos assumidos abstratos; a soma nao e uma grandeza fisica.");
    }

    if (representacao == REPRESENTACAO_LISTA) {
        status_dataset = construir_lista_ponderada_dataset(
            app->dataset, &configuracao, &lista_ponderada);
    } else {
        status_dataset = construir_matriz_ponderada_dataset(
            app->dataset, &configuracao, &matriz_ponderada);
    }
    if (status_dataset != DATASET_SUCESSO) {
        fprintf(stderr, "Falha ao criar grafo ponderado: %s.\n",
                descricao_status_dataset(status_dataset));
        goto erro;
    }

    inicio = tempo_atual_milissegundos();
    if (representacao == REPRESENTACAO_LISTA) {
        status_agm = algoritmo == 1 ?
            agm_prim_lista(lista_ponderada, &resultado) :
            agm_kruskal_lista(lista_ponderada, &resultado);
    } else {
        status_agm = algoritmo == 1 ?
            agm_prim_matriz(matriz_ponderada, &resultado) :
            agm_kruskal_matriz(matriz_ponderada, &resultado);
    }
    tempo = tempo_atual_milissegundos() - inicio;
    if (status_agm != AGM_SUCESSO) {
        fprintf(stderr, "Nao foi possivel executar %s: %s.\n",
                algoritmo == 1 ? "Prim" : "Kruskal",
                nome_status_agm(status_agm));
        if (status_agm == AGM_PESO_AUSENTE) {
            puts("Escolha explicitamente um custo assumido para as arestas "
                 "faltantes se desejar incluir toda a topologia.");
        }
        goto erro;
    }
    for (size_t indice = 0; indice < resultado.quantidade_arestas; indice++) {
        if (resultado.arestas[indice].peso.origem ==
            ORIGEM_PESO_COMPRIMENTO_MEDIDO) {
            arestas_metradas++;
            custo_medido += resultado.arestas[indice].peso.valor;
        } else if (resultado.arestas[indice].peso.origem ==
                   ORIGEM_PESO_CUSTO_ASSUMIDO) {
            arestas_assumidas++;
            custo_assumido_selecionado += resultado.arestas[indice].peso.valor;
        }
    }
    printf("%s em %s: %s, %zu componente(s), %zu aresta(s) selecionada(s).\n",
           algoritmo == 1 ? "Prim" : "Kruskal",
           representacao == REPRESENTACAO_LISTA ? "Lista" : "Matriz",
           resultado.arvore_completa ? "arvore geradora minima completa" :
                                       "floresta geradora minima",
           resultado.quantidade_componentes, resultado.quantidade_arestas);
    printf("Arestas da solucao: %zu medidas, %zu com custo assumido.\n",
           arestas_metradas, arestas_assumidas);
    if (arestas_assumidas != 0) {
        puts("A soma total e numerica e mistura unidades; nao deve ser "
             "interpretada como comprimento ou custo fisico.");
        printf("Soma numerica reportada pela AGM: %.9g "
               "(unidades heterogeneas).\n", resultado.custo_total);
        printf("Subtotais separados (sem unidade comum): medidos=%.9g m; "
               "assumidos=%.9g abstratos.\n",
               custo_medido, custo_assumido_selecionado);
    } else {
        printf("Soma dos comprimentos selecionados: %.9g metros.\n", custo_medido);
    }
    printf("Tempo do algoritmo: %.6f ms.\n", tempo);
    if (resultado.quantidade_arestas > 0) {
        size_t exibidas = interativo && resultado.quantidade_arestas > 12 ?
                          12 : resultado.quantidade_arestas;
        puts("Arestas selecionadas (indices internos; limites de exibicao "
             "aplicados):");
        for (size_t indice = 0; indice < exibidas; indice++) {
            const ArestaAGM *aresta = &resultado.arestas[indice];
            printf("  %s (%zu) -- %s (%zu) | %.9g | %s\n",
                   app->dataset->vertices[aresta->origem].id_externo,
                   aresta->origem,
                   app->dataset->vertices[aresta->destino].id_externo,
                   aresta->destino, aresta->peso.valor,
                   aresta->peso.origem == ORIGEM_PESO_COMPRIMENTO_MEDIDO ?
                       "medido (m)" : "assumido (abstrato)");
        }
        if (exibidas < resultado.quantidade_arestas) {
            printf("  ... %zu aresta(s) omitida(s).\n",
                   resultado.quantidade_arestas - exibidas);
        }
    }
    if (interativo) {
        snprintf(app->ultima_metrica, sizeof(app->ultima_metrica),
                 "%s,%s,%zu,%zu,%.6f",
                 algoritmo == 1 ? "Prim" : "Kruskal",
                 representacao == REPRESENTACAO_LISTA ? "lista" : "matriz",
                 resultado.quantidade_arestas,
                 resultado.quantidade_componentes, tempo);
    }
    liberar_resultado_agm(&resultado);
    liberar_grafo_lista(lista_ponderada);
    destruir_matriz(&matriz_ponderada);
    return 1;

erro:
    liberar_resultado_agm(&resultado);
    liberar_grafo_lista(lista_ponderada);
    destruir_matriz(&matriz_ponderada);
    return 0;
}

static int executar_agm_sintetica_demo(void)
{
    static const size_t ARESTAS[][3] = {
        {0, 1, 1}, {0, 2, 4}, {1, 2, 2}, {1, 3, 5}, {2, 3, 1}
    };
    GrafoLista *lista = criar_grafo_lista();
    MatrizAdjacencia *matriz = NULL;
    size_t bytes;
    ResultadoAGM prim_lista = {0};
    ResultadoAGM kruskal_lista = {0};
    ResultadoAGM prim_matriz = {0};
    ResultadoAGM kruskal_matriz = {0};
    int sucesso = 0;

    if (lista == NULL ||
        estimar_memoria_matriz_ponderada(4, &bytes) != MATRIZ_SUCESSO ||
        criar_matriz(4, bytes, &matriz) != MATRIZ_SUCESSO) {
        goto cleanup;
    }
    for (size_t vertice = 0; vertice < 4; vertice++) {
        size_t indice;
        if (inserir_vertice_lista(lista, &indice) != LISTA_SUCESSO ||
            indice != vertice) {
            goto cleanup;
        }
    }
    for (size_t indice = 0; indice < sizeof(ARESTAS) / sizeof(ARESTAS[0]);
         indice++) {
        if (inserir_aresta_ponderada_lista(
                lista, ARESTAS[indice][0], ARESTAS[indice][1],
                (double)ARESTAS[indice][2],
                ORIGEM_PESO_CUSTO_ASSUMIDO) != LISTA_SUCESSO ||
            inserir_aresta_ponderada(
                matriz, ARESTAS[indice][0], ARESTAS[indice][1],
                (double)ARESTAS[indice][2],
                ORIGEM_PESO_CUSTO_ASSUMIDO) != MATRIZ_SUCESSO) {
            goto cleanup;
        }
    }
    if (agm_prim_lista(lista, &prim_lista) != AGM_SUCESSO ||
        agm_kruskal_lista(lista, &kruskal_lista) != AGM_SUCESSO ||
        agm_prim_matriz(matriz, &prim_matriz) != AGM_SUCESSO ||
        agm_kruskal_matriz(matriz, &kruskal_matriz) != AGM_SUCESSO ||
        prim_lista.quantidade_arestas != 3 ||
        kruskal_lista.quantidade_arestas != 3 ||
        prim_matriz.quantidade_arestas != 3 ||
        kruskal_matriz.quantidade_arestas != 3 ||
        fabs(prim_lista.custo_total - 4.0) > 1e-12 ||
        fabs(kruskal_lista.custo_total - 4.0) > 1e-12 ||
        fabs(prim_matriz.custo_total - 4.0) > 1e-12 ||
        fabs(kruskal_matriz.custo_total - 4.0) > 1e-12) {
        goto cleanup;
    }
    puts("AGM sintetica: ciclo de 4 vertices, 5 arestas, custos assumidos "
         "abstratos.");
    puts("Prim/Lista, Kruskal/Lista, Prim/Matriz e Kruskal/Matriz: "
         "3 arestas, custo minimo numerico 4.");
    puts("Exemplo de arestas selecionadas por Prim/Lista:");
    for (size_t indice = 0; indice < prim_lista.quantidade_arestas; indice++) {
        const ArestaAGM *aresta = &prim_lista.arestas[indice];
        printf("  %zu -- %zu | %.0f (assumido, abstrato)\n",
               aresta->origem, aresta->destino, aresta->peso.valor);
    }
    sucesso = 1;

cleanup:
    liberar_resultado_agm(&prim_lista);
    liberar_resultado_agm(&kruskal_lista);
    liberar_resultado_agm(&prim_matriz);
    liberar_resultado_agm(&kruskal_matriz);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
    if (!sucesso) {
        fputs("Falha na demonstracao sintetica de AGM.\n", stderr);
    }
    return sucesso;
}

static void mostrar_estado_fase2(void)
{
    puts("FASE II: funcionalidades disponiveis neste workspace");
    puts("- Prim/Kruskal: biblioteca da AGM e experimentos sinteticos; "
         "menu executa AGM no dataset ponderado.");
    puts("- Experimentos comparativos de AGM: make experimentos-fase2; "
         "arquivos fase2_* em resultados/.");
    puts("- Dijkstra (Issue #33): indisponivel; nao integrado.");
    puts("- Cobertura de Vertices exata/aproximativa (Issue #35): "
         "indisponivel; nao integrada.");
    puts("- Experimentos de Dijkstra/cobertura permanecem pendentes das "
         "implementacoes correspondentes.");
}

static int executar_demo(Aplicacao *app)
{
    puts("DEMONSTRACAO AUTOMATICA - FASE I + FASE II DISPONIVEL");
    app->representacao = REPRESENTACAO_LISTA;
    if (!executar_bfs(app) || !executar_dfs(app) || !executar_componentes(app) ||
        !simular_falha(app, 1) || !executar_agm_sintetica_demo()) return 0;
    app->representacao = REPRESENTACAO_MATRIZ;
    if (!executar_bfs(app) || !executar_dfs(app) ||
        !executar_componentes(app) || !simular_falha(app, 0)) {
        return 0;
    }
    mostrar_estado_fase2();
    return 1;
}

static void menu(Aplicacao *app)
{
    int continuar = 1;
    while (continuar) {
        long opcao;
        printf("\n========================================\n SMART GRID - FASE I/II\n"
               "========================================\n"
               "Dataset: %zu vertices, %zu arestas\nSubestacao: %s (%s)\n"
               "Representacao atual: [%s]\n\n"
               "1 - Alterar representacao\n2 - Executar BFS\n3 - Executar DFS\n"
               "4 - Identificar componentes conexos\n5 - Simular falha real\n"
               "6 - Mostrar ultima metrica\n7 - Exportar simulacao Graphviz\n"
               "8 - Algoritmos complementares\n"
               "9 - Estado e instrucoes da Fase II\n"
               "10 - Arvore/Floresta Geradora Minima (Prim/Kruskal)\n"
               "0 - Sair\n",
               app->dataset->quantidade_vertices, app->dataset->quantidade_arestas,
               app->dataset->vertices[app->dataset->indice_subestacao].subestacao,
               app->dataset->vertices[app->dataset->indice_subestacao].id_externo,
               nome_representacao(app->representacao));
        {
            int leitura = ler_inteiro("Opcao: ", 0, 10, &opcao);
            if (leitura == 0) {
                puts("\nEntrada encerrada; saindo do menu.");
                break;
            }
            if (leitura < 0) {
                puts("Opcao invalida; informe um inteiro entre 0 e 10.");
                continue;
            }
        }
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
        case 9: mostrar_estado_fase2(); break;
        case 10: executar_agm(app, 1); break;
        case 0: break;
        default: puts("Opcao invalida; informe um numero do menu.");
        }
        if (opcao == 0) continuar = 0;
    }
}

int main(int argc, char **argv)
{
    Dataset *dataset = NULL;
    ErroDataset erro;
    Aplicacao app = {0};
    if (argc > 2) {
        fputs("Informe somente uma opcao. Use --help.\n", stderr);
        return EXIT_FAILURE;
    }
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        puts("Uso: smart_grid [--demo | --experimentos | --help]");
        puts("Sem argumentos, inicia o menu interativo. Prim/Kruskal estao "
             "disponiveis; Dijkstra e Cobertura de Vertices nao.");
        puts("Experimentos de AGM: make experimentos-fase2.");
        return EXIT_SUCCESS;
    }
    if (argc > 1 &&
        strcmp(argv[1], "--demo") != 0 &&
        strcmp(argv[1], "--experimentos") != 0) {
        fprintf(stderr, "Opcao desconhecida: %s. Use --help.\n", argv[1]);
        return EXIT_FAILURE;
    }
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
