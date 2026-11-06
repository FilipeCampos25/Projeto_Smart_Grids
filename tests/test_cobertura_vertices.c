#include "cobertura_vertices.h"
#ifdef COBERTURA_TESTAR_DATASET
#include "dataset.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { size_t u, v; } ArestaTeste;

static size_t verificacoes;
static size_t falhas;

static void conferir(const char *nome, size_t esperado, size_t obtido)
{
    verificacoes++;
    if (esperado != obtido) {
        falhas++;
        fprintf(stderr, "FALHA: %s (esperado %zu, obtido %zu)\n",
                nome, esperado, obtido);
    }
}

static int criar_grafos(size_t n, const ArestaTeste *arestas, size_t m,
                        GrafoLista **lista, MatrizAdjacencia **matriz)
{
    size_t bytes;
    *lista = criar_grafo_lista();
    if (*lista == NULL || estimar_memoria_matriz(n, &bytes) != MATRIZ_SUCESSO ||
        criar_matriz(n, bytes, matriz) != MATRIZ_SUCESSO) return 0;
    for (size_t i = 0; i < n; i++) {
        size_t indice;
        if (inserir_vertice_lista(*lista, &indice) != LISTA_SUCESSO) return 0;
    }
    for (size_t i = 0; i < m; i++) {
        if (inserir_aresta_lista(*lista, arestas[i].u, arestas[i].v) !=
                LISTA_SUCESSO ||
            inserir_aresta(*matriz, arestas[i].u, arestas[i].v) !=
                MATRIZ_SUCESSO) return 0;
    }
    return 1;
}

static void destruir_grafos(GrafoLista *lista, MatrizAdjacencia **matriz)
{
    liberar_grafo_lista(lista);
    destruir_matriz(matriz);
}

static int subconjunto_cobre(size_t mascara, const ArestaTeste *arestas,
                             size_t m)
{
    for (size_t i = 0; i < m; i++)
        if (!(mascara & ((size_t)1 << arestas[i].u)) &&
            !(mascara & ((size_t)1 << arestas[i].v))) return 0;
    return 1;
}

static size_t bits_ligados(size_t valor)
{
    size_t total = 0;
    while (valor != 0) {
        total += valor & 1u;
        valor >>= 1;
    }
    return total;
}

static size_t minimo_enumeracao(size_t n, const ArestaTeste *arestas, size_t m)
{
    size_t melhor = n;
    size_t total = (size_t)1 << n;
    for (size_t mascara = 0; mascara < total; mascara++) {
        size_t cardinalidade = bits_ligados(mascara);
        if (cardinalidade < melhor && subconjunto_cobre(mascara, arestas, m))
            melhor = cardinalidade;
    }
    return melhor;
}

static void testar_caso(const char *nome, size_t n,
                        const ArestaTeste *arestas, size_t m,
                        size_t minimo_esperado)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoCoberturaVertices *exata_lista = NULL;
    ResultadoCoberturaVertices *exata_matriz = NULL;
    ResultadoCoberturaVertices *aprox_lista = NULL;
    ResultadoCoberturaVertices *aprox_matriz = NULL;
    int valida = 0;
    printf("\nCASO: %s\n", nome);
    conferir("criacao dos grafos", 1,
             criar_grafos(n, arestas, m, &lista, &matriz));
    conferir("enumeracao independente", minimo_esperado,
             minimo_enumeracao(n, arestas, m));
    conferir("exata lista status", COBERTURA_SUCESSO,
             cobertura_vertices_exata_lista(lista, NULL, &exata_lista));
    conferir("exata matriz status", COBERTURA_SUCESSO,
             cobertura_vertices_exata_matriz(matriz, NULL, &exata_matriz));
    conferir("aproximada lista status", COBERTURA_SUCESSO,
             cobertura_vertices_aproximada_lista(lista, &aprox_lista));
    conferir("aproximada matriz status", COBERTURA_SUCESSO,
             cobertura_vertices_aproximada_matriz(matriz, &aprox_matriz));
    if (exata_lista && exata_matriz && aprox_lista && aprox_matriz) {
        conferir("minimo lista", minimo_esperado, exata_lista->cardinalidade);
        conferir("minimo matriz", minimo_esperado, exata_matriz->cardinalidade);
        conferir("otimo lista", 1, exata_lista->otima_comprovada);
        conferir("otimo matriz", 1, exata_matriz->otima_comprovada);
        conferir("equivalencia exata", exata_lista->cardinalidade,
                 exata_matriz->cardinalidade);
        conferir("aproximacao valida lista", 1, aprox_lista->cobertura_valida);
        conferir("aproximacao valida matriz", 1, aprox_matriz->cobertura_valida);
        conferir("fator 2 lista", 1,
                 aprox_lista->cardinalidade <= 2 * minimo_esperado);
        conferir("fator 2 matriz", 1,
                 aprox_matriz->cardinalidade <= 2 * minimo_esperado);
        conferir("acessor cardinalidade", exata_lista->cardinalidade,
                 cardinalidade_cobertura(exata_lista));
        conferir("acessor vertices", 1,
                 vertices_selecionados_cobertura(exata_lista) ==
                 exata_lista->selecionados);
        conferir("validacao publica", COBERTURA_SUCESSO,
                 validar_cobertura_lista(lista, aprox_lista->selecionados,
                                         n, &valida));
        conferir("validacao publica resultado", 1, valida);
        conferir("memoria resultado informada", 1,
                 exata_lista->memoria_resultado_bytes >=
                 sizeof(ResultadoCoberturaVertices) + n);
    }
    liberar_resultado_cobertura(&exata_lista);
    liberar_resultado_cobertura(&exata_matriz);
    liberar_resultado_cobertura(&aprox_lista);
    liberar_resultado_cobertura(&aprox_matriz);
    destruir_grafos(lista, &matriz);
}

static void testar_grafos_conhecidos(void)
{
    static const ArestaTeste unica[] = {{0, 1}};
    static const ArestaTeste triangulo[] = {{0, 1}, {1, 2}, {2, 0}};
    static const ArestaTeste caminho[] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}};
    static const ArestaTeste estrela[] = {{0, 1}, {0, 2}, {0, 3}, {0, 4}};
    static const ArestaTeste ciclo[] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}};
    static const ArestaTeste desconexo[] = {{0, 1}, {2, 3}};
    static const ArestaTeste k4[] = {
        {0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}
    };
    testar_caso("grafo vazio", 0, NULL, 0, 0);
    testar_caso("vertices sem arestas", 4, NULL, 0, 0);
    testar_caso("uma aresta", 2, unica, 1, 1);
    testar_caso("triangulo", 3, triangulo, 3, 2);
    testar_caso("caminho simples", 5, caminho, 4, 2);
    testar_caso("estrela", 5, estrela, 4, 1);
    testar_caso("ciclo impar", 5, ciclo, 5, 3);
    testar_caso("componentes desconexos", 5, desconexo, 2, 2);
    testar_caso("K4 com minimo conhecido", 4, k4, 6, 3);
}

static void testar_limites_e_invalidos(void)
{
    static const ArestaTeste triangulo[] = {{0, 1}, {1, 2}, {2, 0}};
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    MatrizAdjacencia *direcionada = NULL;
    ResultadoCoberturaVertices *resultado = NULL;
    ResultadoCoberturaVertices sentinela = {0};
    ResultadoCoberturaVertices *ocupado = &sentinela;
    ConfiguracaoCoberturaExata limite_vertices = {2, SIZE_MAX};
    ConfiguracaoCoberturaExata limite_nos = {SIZE_MAX, 1};
    size_t bytes;
    int valida = 7;
    puts("\nLIMITES E ENTRADAS INVALIDAS");
    criar_grafos(3, triangulo, 3, &lista, &matriz);
    conferir("limite de vertices", COBERTURA_LIMITE_ATINGIDO,
             cobertura_vertices_exata_lista(lista, &limite_vertices,
                                             &resultado));
    if (resultado) {
        conferir("limitado nao e otimo", 0, resultado->otima_comprovada);
        conferir("limitado ainda e cobertura", 1, resultado->cobertura_valida);
    }
    liberar_resultado_cobertura(&resultado);
    conferir("limite de nos", COBERTURA_LIMITE_ATINGIDO,
             cobertura_vertices_exata_matriz(matriz, &limite_nos, &resultado));
    if (resultado) {
        conferir("um no explorado", 1, resultado->nos_explorados);
        conferir("interrompido nao e otimo", 0, resultado->otima_comprovada);
    }
    liberar_resultado_cobertura(&resultado);
    conferir("grafo nulo", COBERTURA_ARGUMENTO_INVALIDO,
             cobertura_vertices_aproximada_lista(NULL, &resultado));
    conferir("saida nula", COBERTURA_ARGUMENTO_INVALIDO,
             cobertura_vertices_aproximada_lista(lista, NULL));
    conferir("saida ocupada", COBERTURA_ARGUMENTO_INVALIDO,
             cobertura_vertices_aproximada_lista(lista, &ocupado));
    conferir("validacao tamanho errado", COBERTURA_ARGUMENTO_INVALIDO,
             validar_cobertura_lista(lista, (const unsigned char *)"abc", 2,
                                     &valida));
    conferir("saida de validacao preservada", 7, valida);
    estimar_memoria_matriz(3, &bytes);
    criar_matriz_direcionada(3, bytes, &direcionada);
    inserir_aresta(direcionada, 0, 1);
    conferir("digrafo rejeitado", COBERTURA_GRAFO_DIRECIONADO,
             cobertura_vertices_aproximada_matriz(direcionada, &resultado));
    destruir_matriz(&direcionada);
    destruir_grafos(lista, &matriz);
}

#ifdef COBERTURA_TESTAR_DATASET
static void testar_dataset_real(void)
{
    Dataset *dataset = NULL;
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoCoberturaVertices *aprox_lista = NULL;
    ResultadoCoberturaVertices *aprox_matriz = NULL;
    ResultadoCoberturaVertices *exata = NULL;
    ErroDataset erro;
    puts("\nDATASET REAL COMPLETO");
    conferir("carregar dataset", DATASET_SUCESSO,
             carregar_dataset("data/forcel_2025_circuito_1/vertices.csv",
                              "data/forcel_2025_circuito_1/arestas.csv",
                              &dataset, &erro));
    if (dataset == NULL) return;
    conferir("1838 vertices", 1838, dataset->quantidade_vertices);
    conferir("1837 arestas", 1837, dataset->quantidade_arestas);
    conferir("construir lista real", DATASET_SUCESSO,
             construir_lista_dataset(dataset, &lista));
    conferir("construir matriz real", DATASET_SUCESSO,
             construir_matriz_dataset(dataset, &matriz));
    if (lista != NULL && matriz != NULL) {
        conferir("aproximacao lista completa", COBERTURA_SUCESSO,
                 cobertura_vertices_aproximada_lista(lista, &aprox_lista));
        conferir("aproximacao matriz completa", COBERTURA_SUCESSO,
                 cobertura_vertices_aproximada_matriz(matriz, &aprox_matriz));
        if (aprox_lista && aprox_matriz) {
            printf("Cardinalidades aproximadas reais: lista=%zu, matriz=%zu\n",
                   aprox_lista->cardinalidade, aprox_matriz->cardinalidade);
            conferir("cobertura real valida lista", 1,
                     aprox_lista->cobertura_valida);
            conferir("cobertura real valida matriz", 1,
                     aprox_matriz->cobertura_valida);
        }
        conferir("exata protegida por limite", COBERTURA_LIMITE_ATINGIDO,
                 cobertura_vertices_exata_lista(lista, NULL, &exata));
        if (exata != NULL) {
            conferir("exata real incompleta", 0, exata->otima_comprovada);
            conferir("nenhum no real explorado", 0, exata->nos_explorados);
            conferir("melhor viavel real valida", 1, exata->cobertura_valida);
        }
    }
    liberar_resultado_cobertura(&aprox_lista);
    liberar_resultado_cobertura(&aprox_matriz);
    liberar_resultado_cobertura(&exata);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
    liberar_dataset(&dataset);
}
#endif

#ifdef COBERTURA_TESTAR_ALOCACAO
static size_t chamada_alocacao;
static size_t falhar_na_chamada;
static size_t blocos_vivos;

void *teste_malloc(size_t tamanho)
{
    void *ponteiro;
    chamada_alocacao++;
    if (chamada_alocacao == falhar_na_chamada) return NULL;
    ponteiro = malloc(tamanho);
    if (ponteiro != NULL) blocos_vivos++;
    return ponteiro;
}

void *teste_calloc(size_t quantidade, size_t tamanho)
{
    void *ponteiro;
    chamada_alocacao++;
    if (chamada_alocacao == falhar_na_chamada) return NULL;
    ponteiro = calloc(quantidade, tamanho);
    if (ponteiro != NULL) blocos_vivos++;
    return ponteiro;
}

void teste_free(void *ponteiro)
{
    if (ponteiro != NULL) blocos_vivos--;
    free(ponteiro);
}

static void testar_falhas_alocacao(void)
{
    static const ArestaTeste triangulo[] = {{0, 1}, {1, 2}, {2, 0}};
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoCoberturaVertices *resultado = NULL;
    size_t total;
    puts("\nFALHAS DE ALOCACAO");
    criar_grafos(3, triangulo, 3, &lista, &matriz);
    chamada_alocacao = 0;
    falhar_na_chamada = 0;
    cobertura_vertices_exata_lista(lista, NULL, &resultado);
    total = chamada_alocacao;
    liberar_resultado_cobertura(&resultado);
    conferir("baseline sem vazamento", 0, blocos_vivos);
    for (size_t i = 1; i <= total; i++) {
        chamada_alocacao = 0;
        falhar_na_chamada = i;
        conferir("falha de alocacao", COBERTURA_SEM_MEMORIA,
                 cobertura_vertices_exata_lista(lista, NULL, &resultado));
        conferir("falha nao entrega resultado", 1, resultado == NULL);
        conferir("falha nao vaza", 0, blocos_vivos);
    }
    falhar_na_chamada = 0;
    destruir_grafos(lista, &matriz);
}
#endif

int main(void)
{
    testar_grafos_conhecidos();
    testar_limites_e_invalidos();
#ifdef COBERTURA_TESTAR_DATASET
    testar_dataset_real();
#endif
#ifdef COBERTURA_TESTAR_ALOCACAO
    testar_falhas_alocacao();
#endif
    printf("\nRESULTADO COBERTURA: %s (%zu verificacoes, %zu falhas)\n",
           falhas == 0 ? "PASSOU" : "FALHOU", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
