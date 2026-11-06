#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "agm.h"
#include "benchmark_alocacao.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

enum {
    QUANTIDADE_ESCALAS = 3,
    EXTRAS_POR_VERTICE = 2,
    PESO_MAXIMO = 1000,
    REPETICOES_MAXIMAS = 100,
    SEMENTE_PADRAO = 20261106
};

static const size_t ESCALAS[QUANTIDADE_ESCALAS] = {100, 500, 1000};
static const char CENARIO[] = "sintetico_conectado_ciclico_pesos_assumidos";
static const char PARAMETROS[] =
    "arvore_geradora+2N_arestas_extras;pesos_inteiros_1_a_1000;"
    "origem_assumida;mesma_instancia_em_todas_as_comparacoes";

typedef struct {
    GrafoLista *lista;
    MatrizAdjacencia *matriz;
    size_t vertices;
    size_t arestas;
    uint32_t semente;
} Instancia;

typedef struct {
    const char *algoritmo;
    const char *representacao;
    size_t memoria_representacao;
    size_t memoria_auxiliar;
    double tempo_ms;
    double custo;
    const char *status;
} Registro;

static double tempo_atual_milissegundos(void)
{
#ifdef _WIN32
    LARGE_INTEGER contador;
    LARGE_INTEGER frequencia;
    if (!QueryPerformanceFrequency(&frequencia) ||
        !QueryPerformanceCounter(&contador)) {
        return NAN;
    }
    return (double)contador.QuadPart * 1000.0 / (double)frequencia.QuadPart;
#else
    struct timespec instante;
    if (clock_gettime(CLOCK_MONOTONIC, &instante) != 0) {
        return NAN;
    }
    return (double)instante.tv_sec * 1000.0 +
           (double)instante.tv_nsec / 1000000.0;
#endif
}

static uint32_t proximo_aleatorio(uint32_t *estado)
{
    *estado = *estado * UINT32_C(1664525) + UINT32_C(1013904223);
    return *estado;
}

static size_t raiz(size_t *pais, size_t vertice)
{
    size_t atual = vertice;
    while (pais[atual] != atual) {
        atual = pais[atual];
    }
    while (pais[vertice] != vertice) {
        size_t proximo = pais[vertice];
        pais[vertice] = atual;
        vertice = proximo;
    }
    return atual;
}

static int unir(size_t *pais, size_t *rank, size_t primeiro, size_t segundo)
{
    size_t raiz_a = raiz(pais, primeiro);
    size_t raiz_b = raiz(pais, segundo);
    if (raiz_a == raiz_b) {
        return 0;
    }
    if (rank[raiz_a] < rank[raiz_b]) {
        pais[raiz_a] = raiz_b;
    } else if (rank[raiz_a] > rank[raiz_b]) {
        pais[raiz_b] = raiz_a;
    } else {
        pais[raiz_b] = raiz_a;
        rank[raiz_a]++;
    }
    return 1;
}

static void liberar_instancia(Instancia *instancia)
{
    liberar_grafo_lista(instancia->lista);
    destruir_matriz(&instancia->matriz);
    *instancia = (Instancia){0};
}

static int inserir_ponderada(Instancia *instancia, size_t origem,
                             size_t destino, double peso)
{
    if (inserir_aresta_ponderada_lista(
            instancia->lista, origem, destino, peso,
            ORIGEM_PESO_CUSTO_ASSUMIDO) != LISTA_SUCESSO ||
        inserir_aresta_ponderada(
            instancia->matriz, origem, destino, peso,
            ORIGEM_PESO_CUSTO_ASSUMIDO) != MATRIZ_SUCESSO) {
        return 0;
    }
    instancia->arestas++;
    return 1;
}

static int criar_instancia(size_t vertices, uint32_t semente,
                           Instancia *instancia)
{
    size_t bytes_matriz;
    uint32_t estado = semente;

    *instancia = (Instancia){0};
    instancia->vertices = vertices;
    instancia->semente = semente;
    instancia->lista = criar_grafo_lista();
    if (instancia->lista == NULL ||
        estimar_memoria_matriz_ponderada(vertices, &bytes_matriz) !=
            MATRIZ_SUCESSO ||
        criar_matriz(vertices, bytes_matriz, &instancia->matriz) !=
            MATRIZ_SUCESSO) {
        liberar_instancia(instancia);
        return 0;
    }
    for (size_t vertice = 0; vertice < vertices; vertice++) {
        size_t indice = SIZE_MAX;
        if (inserir_vertice_lista(instancia->lista, &indice) !=
                LISTA_SUCESSO ||
            indice != vertice) {
            liberar_instancia(instancia);
            return 0;
        }
    }

    for (size_t destino = 1; destino < vertices; destino++) {
        size_t origem = (size_t)(proximo_aleatorio(&estado) % destino);
        double peso = (double)(proximo_aleatorio(&estado) % PESO_MAXIMO) + 1.0;
        if (!inserir_ponderada(instancia, origem, destino, peso)) {
            liberar_instancia(instancia);
            return 0;
        }
    }
    {
        size_t extras_inseridos = 0;
        size_t quantidade_extras = EXTRAS_POR_VERTICE * vertices;
        while (extras_inseridos < quantidade_extras) {
            size_t origem;
            size_t destino;
            int existe = 1;
            do {
                origem = (size_t)(proximo_aleatorio(&estado) % vertices);
                destino = (size_t)(proximo_aleatorio(&estado) % vertices);
            } while (origem == destino);
            if (existe_aresta_lista(
                    instancia->lista, origem, destino, &existe) !=
                LISTA_SUCESSO) {
                liberar_instancia(instancia);
                return 0;
            }
            if (!existe) {
                double peso =
                    (double)(proximo_aleatorio(&estado) % PESO_MAXIMO) + 1.0;
                if (!inserir_ponderada(instancia, origem, destino, peso)) {
                    liberar_instancia(instancia);
                    return 0;
                }
                extras_inseridos++;
            }
        }
    }
    if (instancia->arestas != 3 * vertices - 1) {
        liberar_instancia(instancia);
        return 0;
    }
    return 1;
}

static int instancias_iguais(const Instancia *a, const Instancia *b)
{
    if (a->vertices != b->vertices || a->arestas != b->arestas ||
        a->semente != b->semente) {
        return 0;
    }
    for (size_t origem = 0; origem < a->vertices; origem++) {
        for (size_t destino = origem + 1; destino < a->vertices; destino++) {
            int adj_a = 0;
            int adj_b = 0;
            PesoAresta peso_a;
            PesoAresta peso_b;
            if (consultar_adjacencia(a->matriz, origem, destino, &adj_a) !=
                    MATRIZ_SUCESSO ||
                consultar_adjacencia(b->matriz, origem, destino, &adj_b) !=
                    MATRIZ_SUCESSO ||
                adj_a != adj_b) {
                return 0;
            }
            if (adj_a &&
                (consultar_peso_aresta(a->matriz, origem, destino, &peso_a) !=
                     MATRIZ_SUCESSO ||
                 consultar_peso_aresta(b->matriz, origem, destino, &peso_b) !=
                     MATRIZ_SUCESSO ||
                 peso_a.possui_peso != peso_b.possui_peso ||
                 peso_a.valor != peso_b.valor ||
                 peso_a.origem != peso_b.origem)) {
                return 0;
            }
        }
    }
    return 1;
}

static int validar_resultado(const Instancia *instancia,
                             const ResultadoAGM *resultado, int representacao_lista)
{
    size_t *pais_entrada = NULL;
    size_t *pais_saida = NULL;
    size_t *rank_entrada = NULL;
    size_t *rank_saida = NULL;
    int valido = 0;

    if (instancia->vertices > SIZE_MAX / sizeof(*pais_entrada)) {
        return 0;
    }
    pais_entrada = malloc(instancia->vertices * sizeof(*pais_entrada));
    pais_saida = malloc(instancia->vertices * sizeof(*pais_saida));
    rank_entrada = calloc(instancia->vertices, sizeof(*rank_entrada));
    rank_saida = calloc(instancia->vertices, sizeof(*rank_saida));
    if (pais_entrada == NULL || pais_saida == NULL || rank_entrada == NULL ||
        rank_saida == NULL) {
        goto cleanup;
    }
    for (size_t vertice = 0; vertice < instancia->vertices; vertice++) {
        pais_entrada[vertice] = vertice;
        pais_saida[vertice] = vertice;
    }
    for (size_t origem = 0; origem < instancia->vertices; origem++) {
        const VizinhoLista *vizinho = NULL;
        if (buscar_vizinhos_lista(instancia->lista, origem, &vizinho) !=
            LISTA_SUCESSO) {
            goto cleanup;
        }
        for (; vizinho != NULL; vizinho = proximo_vizinho_lista(vizinho)) {
            size_t destino = indice_vizinho_lista(vizinho);
            if (origem < destino &&
                !unir(pais_entrada, rank_entrada, origem, destino)) {
                goto cleanup;
            }
        }
    }
    if (resultado->quantidade_componentes != 1 ||
        !resultado->arvore_completa ||
        resultado->quantidade_arestas != instancia->vertices - 1) {
        goto cleanup;
    }
    {
        double custo = 0.0;
        for (size_t indice = 0; indice < resultado->quantidade_arestas;
             indice++) {
            const ArestaAGM *aresta = &resultado->arestas[indice];
            PesoAresta peso;
            if (representacao_lista) {
                if (consultar_peso_aresta_lista(
                        instancia->lista, aresta->origem, aresta->destino,
                        &peso) != LISTA_SUCESSO) {
                    goto cleanup;
                }
            } else if (consultar_peso_aresta(
                           instancia->matriz, aresta->origem, aresta->destino,
                           &peso) != MATRIZ_SUCESSO) {
                goto cleanup;
            }
            if (!peso.possui_peso || aresta->peso.origem != peso.origem ||
                aresta->peso.valor != peso.valor ||
                !unir(pais_saida, rank_saida, aresta->origem, aresta->destino)) {
                goto cleanup;
            }
            custo += peso.valor;
            if (!isfinite(custo)) {
                goto cleanup;
            }
        }
        if (fabs(custo - resultado->custo_total) >
            1e-12 * fmax(1.0, fabs(custo))) {
            goto cleanup;
        }
    }
    for (size_t vertice = 1; vertice < instancia->vertices; vertice++) {
        if (raiz(pais_entrada, vertice) != raiz(pais_entrada, 0) ||
            raiz(pais_saida, vertice) != raiz(pais_saida, 0)) {
            goto cleanup;
        }
    }
    valido = 1;

cleanup:
    free(pais_entrada);
    free(pais_saida);
    free(rank_entrada);
    free(rank_saida);
    return valido;
}

static StatusAGM executar_algoritmo(const Instancia *instancia, int algoritmo,
                                    int representacao, ResultadoAGM *resultado)
{
    if (algoritmo == 0) {
        return representacao == 0 ?
            agm_prim_lista(instancia->lista, resultado) :
            agm_prim_matriz(instancia->matriz, resultado);
    }
    return representacao == 0 ?
        agm_kruskal_lista(instancia->lista, resultado) :
        agm_kruskal_matriz(instancia->matriz, resultado);
}

static int custo_equivalente(double primeiro, double segundo)
{
    return fabs(primeiro - segundo) <=
           1e-12 * fmax(1.0, fmax(fabs(primeiro), fabs(segundo)));
}

static int medir(const Instancia *instancia, int algoritmo, int representacao,
                 Registro *registro)
{
    ResultadoAGM resultado = {0};
    double inicio;
    double fim;
    StatusAGM status;

    if (!benchmark_alocacao_reiniciar()) {
        return 0;
    }
    inicio = tempo_atual_milissegundos();
    status = executar_algoritmo(instancia, algoritmo, representacao, &resultado);
    fim = tempo_atual_milissegundos();
    registro->tempo_ms = fim - inicio;
    registro->memoria_auxiliar = benchmark_alocacao_pico();
    registro->custo = status == AGM_SUCESSO ? resultado.custo_total : NAN;
    registro->status = status == AGM_SUCESSO ? "OK" : "ERRO_ALGORITMO";
    if (!isfinite(inicio) || !isfinite(fim) || !isfinite(registro->tempo_ms) ||
        registro->tempo_ms < 0.0 ||
        (status == AGM_SUCESSO &&
         !validar_resultado(instancia, &resultado, representacao == 0))) {
        registro->status = "ERRO_VALIDACAO";
    }
    liberar_resultado_agm(&resultado);
    if (benchmark_alocacao_ativa() != 0) {
        registro->status = "ERRO_ALOCACAO";
        return 0;
    }
    return strcmp(registro->status, "OK") == 0;
}

static int registrar(FILE *arquivo, const Instancia *instancia,
                     const Registro *registro, size_t repeticao)
{
    return fprintf(arquivo,
        "%s,%s,%s,%zu,%zu,%zu,%" PRIu32 ",%s,%.9f,%zu,%zu,%.17g,%s\n",
        CENARIO, registro->algoritmo, registro->representacao,
        instancia->vertices, instancia->arestas, repeticao, instancia->semente,
        PARAMETROS, registro->tempo_ms, registro->memoria_representacao,
        registro->memoria_auxiliar, registro->custo, registro->status) >= 0;
}

static const char *compilador(void)
{
#if defined(__clang__)
    return "Clang " __clang_version__;
#elif defined(__GNUC__)
    return "GCC " __VERSION__;
#elif defined(_MSC_VER)
    return "MSVC";
#else
    return "compilador C11 (identificacao indisponivel)";
#endif
}

static int registrar_ambiente(const char *caminho, size_t repeticoes,
                              uint32_t semente_base)
{
    FILE *arquivo = fopen(caminho, "w");
    if (arquivo == NULL) {
        fprintf(stderr, "Nao foi possivel abrir metadados do protocolo: %s\n",
                caminho);
        return 0;
    }
#ifdef _WIN32
    const char *sistema = "Windows";
    const char *relogio = "QueryPerformanceCounter";
    const char *host = getenv("COMPUTERNAME");
#else
    const char *sistema = "POSIX";
    const char *relogio = "clock_gettime(CLOCK_MONOTONIC)";
    const char *host = getenv("HOSTNAME");
#endif
#if defined(__x86_64__) || defined(_M_X64)
    const char *arquitetura = "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    const char *arquitetura = "ARM64";
#elif defined(__i386__) || defined(_M_IX86)
    const char *arquitetura = "x86";
#else
    const char *arquitetura = "desconhecida";
#endif
#ifdef __STDC_VERSION__
    const unsigned long padrao_c = (unsigned long)__STDC_VERSION__;
#else
    const unsigned long padrao_c = 0;
#endif
    if (fprintf(arquivo,
        "protocolo=Fase II; algoritmos=Prim,Kruskal; "
        "representacoes=lista,matriz\n"
        "compilador=%s\n"
        "padrao_C=%lu\n"
        "sistema=%s\n"
        "host=%s\n"
        "arquitetura=%s\n"
        "relogio=%s\n"
        "repeticoes=%zu\n"
        "semente_base=%" PRIu32 "\n"
        "semente_por_tamanho=semente_base+N em uint32_t\n"
        "tamanhos=100,500,1000\n"
        "arestas_por_instancia=3N-1 (arvore geradora + 2N extras)\n"
        "pesos=inteiros de 1 a 1000; origem assumida; sem unidade fisica\n"
        "validacao=cobertura, ausencia de ciclos, arestas e custos equivalentes\n",
        compilador(), padrao_c, sistema, host == NULL ? "desconhecido" : host,
        arquitetura, relogio, repeticoes, semente_base) < 0) {
        fclose(arquivo);
        return 0;
    }
    return fclose(arquivo) == 0;
}

static int validar_gerador_deterministico(void)
{
    Instancia primeira = {0};
    Instancia segunda = {0};
    uint32_t semente = UINT32_C(20261106) + UINT32_C(100);
    int sucesso = criar_instancia(100, semente, &primeira) &&
                  criar_instancia(100, semente, &segunda) &&
                  instancias_iguais(&primeira, &segunda);
    liberar_instancia(&primeira);
    liberar_instancia(&segunda);
    return sucesso;
}

static int interpretar_positivo(const char *texto, size_t *valor)
{
    char *fim = NULL;
    unsigned long long numero;
    if (texto == NULL || *texto == '\0' || *texto == '-') {
        return 0;
    }
    errno = 0;
    numero = strtoull(texto, &fim, 10);
    if (errno != 0 || fim == texto || *fim != '\0' || numero == 0) {
        return 0;
    }
    *valor = (size_t)numero;
    return (unsigned long long)*valor == numero;
}

static int interpretar_semente(const char *texto, uint32_t *semente)
{
    size_t valor;
    if (!interpretar_positivo(texto, &valor) || valor > UINT32_MAX) {
        return 0;
    }
    *semente = (uint32_t)valor;
    return 1;
}

static int executar_experimentos(const char *caminho,
                                 const char *caminho_ambiente,
                                 size_t repeticoes, uint32_t semente_base)
{
    static const char *const CABECALHO =
        "cenario,algoritmo,representacao,N,arestas,repeticao,semente,"
        "parametros,tempo_ms,memoria_representacao_bytes,"
        "memoria_auxiliar_pico_bytes,valor_solucao,status\n";
    FILE *arquivo = fopen(caminho, "w");
    if (arquivo == NULL) {
        fprintf(stderr, "Nao foi possivel abrir CSV de saida: %s\n", caminho);
        return 0;
    }
    if (fputs(CABECALHO, arquivo) < 0) {
        fclose(arquivo);
        return 0;
    }
    for (size_t escala = 0; escala < QUANTIDADE_ESCALAS; escala++) {
        size_t vertices = ESCALAS[escala];
        uint32_t semente = semente_base + (uint32_t)vertices;
        Instancia instancia = {0};
        if (!criar_instancia(vertices, semente, &instancia)) {
            fprintf(stderr, "Falha ao gerar instancia deterministica N=%zu.\n",
                    vertices);
            fclose(arquivo);
            return 0;
        }
        for (size_t repeticao = 1; repeticao <= repeticoes; repeticao++) {
            Registro registros[2][2] = {
                {
                    {"Prim", "lista", memoria_lista(instancia.lista), 0,
                     0.0, 0.0, "NAO_EXECUTADO"},
                    {"Prim", "matriz", memoria_matriz(instancia.matriz), 0,
                     0.0, 0.0, "NAO_EXECUTADO"}
                },
                {
                    {"Kruskal", "lista", memoria_lista(instancia.lista), 0,
                     0.0, 0.0, "NAO_EXECUTADO"},
                    {"Kruskal", "matriz", memoria_matriz(instancia.matriz), 0,
                     0.0, 0.0, "NAO_EXECUTADO"}
                }
            };
            int valido = 1;
            for (size_t ordem = 0; ordem < 4; ordem++) {
                size_t indice = repeticao % 2 == 0 ? ordem : 3 - ordem;
                int algoritmo = (int)(indice / 2);
                int representacao = (int)(indice % 2);
                if (!medir(&instancia, algoritmo, representacao,
                           &registros[algoritmo][representacao])) {
                    valido = 0;
                }
            }
            for (int primeiro = 0; primeiro < 4; primeiro++) {
                for (int segundo = primeiro + 1; segundo < 4; segundo++) {
                    Registro *a = &registros[primeiro / 2][primeiro % 2];
                    Registro *b = &registros[segundo / 2][segundo % 2];
                    if (strcmp(a->status, "OK") == 0 &&
                        strcmp(b->status, "OK") == 0 &&
                        !custo_equivalente(a->custo, b->custo)) {
                        a->status = "ERRO_EQUIVALENCIA";
                        b->status = "ERRO_EQUIVALENCIA";
                        valido = 0;
                    }
                }
            }
            for (int algoritmo = 0; algoritmo < 2; algoritmo++) {
                for (int representacao = 0; representacao < 2; representacao++) {
                    if (!registrar(arquivo, &instancia,
                                   &registros[algoritmo][representacao],
                                   repeticao)) {
                        valido = 0;
                    }
                }
            }
            if (!valido) {
                fprintf(stderr,
                    "Resultado invalido: N=%zu repeticao=%zu; consulte status "
                    "no CSV.\n", vertices, repeticao);
            }
        }
        liberar_instancia(&instancia);
    }
    if (fclose(arquivo) != 0) {
        return 0;
    }
    return registrar_ambiente(caminho_ambiente, repeticoes, semente_base);
}

int main(int argc, char **argv)
{
    const char *caminho = "resultados/fase2_bruto.csv";
    const char *caminho_ambiente = "resultados/fase2_ambiente.txt";
    size_t repeticoes = 7;
    uint32_t semente = SEMENTE_PADRAO;
    int validar_gerador = 0;

    for (int indice = 1; indice < argc; indice++) {
        if (strcmp(argv[indice], "--validar-gerador") == 0) {
            validar_gerador = 1;
        } else if (strcmp(argv[indice], "--saida") == 0 && indice + 1 < argc) {
            caminho = argv[++indice];
        } else if (strcmp(argv[indice], "--ambiente") == 0 &&
                   indice + 1 < argc) {
            caminho_ambiente = argv[++indice];
        } else if (strcmp(argv[indice], "--repeticoes") == 0 &&
                   indice + 1 < argc) {
            if (!interpretar_positivo(argv[++indice], &repeticoes)) {
                fputs("Quantidade de repeticoes invalida.\n", stderr);
                return EXIT_FAILURE;
            }
            if (repeticoes > REPETICOES_MAXIMAS) {
                fprintf(stderr, "Limite de repeticoes: %d.\n",
                        REPETICOES_MAXIMAS);
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[indice], "--semente") == 0 &&
                   indice + 1 < argc) {
            if (!interpretar_semente(argv[++indice], &semente)) {
                fputs("Semente invalida: esperado inteiro em [1, UINT32_MAX].\n",
                      stderr);
                return EXIT_FAILURE;
            }
        } else {
            fprintf(stderr, "Argumento desconhecido ou sem valor: %s\n",
                    argv[indice]);
            return EXIT_FAILURE;
        }
    }
    if (validar_gerador) {
        if (!validar_gerador_deterministico()) {
            fputs("Gerador nao foi deterministico para a mesma semente.\n",
                  stderr);
            return EXIT_FAILURE;
        }
        puts("Geracao deterministica validada (N=100).");
        return EXIT_SUCCESS;
    }
    if (!executar_experimentos(caminho, caminho_ambiente, repeticoes,
                               semente)) {
        return EXIT_FAILURE;
    }
    printf("CSV bruto da Fase II gerado: %s (%zu repeticoes); ambiente: %s.\n",
           caminho, repeticoes, caminho_ambiente);
    return EXIT_SUCCESS;
}
