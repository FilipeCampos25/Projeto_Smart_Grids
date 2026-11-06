#include "dijkstra.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

static size_t verificacoes;
static size_t falhas;

static void conferir(const char *descricao, size_t esperado, size_t obtido)
{
    verificacoes++;
    if (esperado != obtido) falhas++;
    printf("%s: esperado=%zu obtido=%zu %s\n", descricao, esperado, obtido,
           esperado == obtido ? "PASSOU" : "FALHOU");
}

static void conferir_double(const char *descricao, double esperado, double obtido)
{
    verificacoes++;
    if (esperado != obtido) falhas++;
    printf("%s: esperado=%.17g obtido=%.17g %s\n", descricao, esperado, obtido,
           esperado == obtido ? "PASSOU" : "FALHOU");
}

static int criar_grafos(size_t n, GrafoLista **lista, MatrizAdjacencia **matriz)
{
    size_t bytes;
    *lista = criar_grafo_lista();
    if (*lista == NULL ||
        estimar_memoria_matriz_ponderada(n, &bytes) != MATRIZ_SUCESSO ||
        criar_matriz(n, bytes, matriz) != MATRIZ_SUCESSO) return 0;
    for (size_t i = 0; i < n; i++) {
        size_t indice;
        if (inserir_vertice_lista(*lista, &indice) != LISTA_SUCESSO || indice != i)
            return 0;
    }
    return 1;
}

static int inserir_peso(GrafoLista *lista, MatrizAdjacencia *matriz,
                        size_t a, size_t b, double peso,
                        OrigemPesoAresta origem)
{
    return inserir_aresta_ponderada_lista(lista, a, b, peso, origem) ==
               LISTA_SUCESSO &&
           inserir_aresta_ponderada(matriz, a, b, peso, origem) ==
               MATRIZ_SUCESSO;
}

static void destruir_grafos(GrafoLista *lista, MatrizAdjacencia **matriz)
{
    liberar_grafo_lista(lista);
    destruir_matriz(matriz);
}

static void testar_solucao_e_equivalencia(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoDijkstra *rl = NULL, *rm = NULL;
    const size_t caminho[] = {0, 2, 1, 3, 4};
    puts("\nSOLUCAO CONHECIDA, ALTERNATIVAS E EQUIVALENCIA");
    conferir("Criar grafos", 1, criar_grafos(5, &lista, &matriz));
    if (lista == NULL || matriz == NULL) return;
    inserir_peso(lista, matriz, 0, 1, 4.0, ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    inserir_peso(lista, matriz, 0, 2, 1.0, ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    inserir_peso(lista, matriz, 2, 1, 2.0, ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    inserir_peso(lista, matriz, 1, 3, 1.0, ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    inserir_peso(lista, matriz, 2, 3, 5.0, ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    inserir_peso(lista, matriz, 3, 4, 3.0, ORIGEM_PESO_CUSTO_ASSUMIDO);
    conferir("Dijkstra lista", DIJKSTRA_SUCESSO,
        executar_dijkstra_lista(lista, 0, 4,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &rl));
    conferir("Dijkstra matriz", DIJKSTRA_SUCESSO,
        executar_dijkstra_matriz(matriz, 0, 4,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &rm));
    if (rl != NULL && rm != NULL) {
        conferir("Lista alcancavel", 1, rl->alcancavel);
        conferir("Matriz alcancavel", 1, rm->alcancavel);
        conferir_double("Custo conhecido lista", 7.0, rl->custo_total);
        conferir_double("Custos equivalentes", rl->custo_total, rm->custo_total);
        conferir("Tamanho do caminho", 5, rl->quantidade_vertices_caminho);
        conferir("Tamanhos equivalentes", rl->quantidade_vertices_caminho,
                 rm->quantidade_vertices_caminho);
        for (size_t i = 0; i < 5; i++) {
            conferir("Vertice ordenado lista", caminho[i], rl->caminho[i]);
            conferir("Caminho equivalente", rl->caminho[i], rm->caminho[i]);
        }
        conferir("Custo assumido sinalizado lista", 1, rl->utiliza_custo_assumido);
        conferir("Custo assumido sinalizado matriz", 1, rm->utiliza_custo_assumido);
        conferir("Processados equivalentes", rl->quantidade_vertices_processados,
                 rm->quantidade_vertices_processados);
    }
    liberar_resultado_dijkstra(&rl);
    liberar_resultado_dijkstra(&rm);
    destruir_grafos(lista, &matriz);
}

static void testar_zero_e_mesma_origem(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoDijkstra *resultado = NULL;
    puts("\nPESO ZERO E ORIGEM IGUAL AO DESTINO");
    criar_grafos(3, &lista, &matriz);
    inserir_peso(lista, matriz, 0, 1, 0.0, ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    inserir_peso(lista, matriz, 1, 2, 1.0, ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    inserir_peso(lista, matriz, 0, 2, 5.0, ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    conferir("Zero aceito", DIJKSTRA_SUCESSO,
        executar_dijkstra_lista(lista, 0, 2,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &resultado));
    if (resultado != NULL) {
        conferir_double("Custo com peso zero", 1.0, resultado->custo_total);
        conferir("Caminho usa zero", 1, resultado->caminho[1]);
    }
    liberar_resultado_dijkstra(&resultado);
    conferir("Origem igual ao destino", DIJKSTRA_SUCESSO,
        executar_dijkstra_matriz(matriz, 1, 1,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &resultado));
    if (resultado != NULL) {
        conferir("Origem alcancavel", 1, resultado->alcancavel);
        conferir("Caminho unitario", 1, resultado->quantidade_vertices_caminho);
        conferir("Vertice unitario", 1, resultado->caminho[0]);
        conferir_double("Custo da origem", 0.0, resultado->custo_total);
        conferir("Um vertice processado", 1,
                 resultado->quantidade_vertices_processados);
    }
    liberar_resultado_dijkstra(&resultado);
    destruir_grafos(lista, &matriz);
}

static void testar_desconectado_e_sem_peso(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoDijkstra *resultado = NULL;
    puts("\nDESCONEXAO, CAMINHO INEXISTENTE E PESO AUSENTE");
    criar_grafos(4, &lista, &matriz);
    inserir_peso(lista, matriz, 0, 1, 2.0, ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    inserir_aresta_lista(lista, 1, 2);
    inserir_aresta(matriz, 1, 2);
    conferir("Lista rejeita aresta sem peso", DIJKSTRA_ARESTA_SEM_PESO,
        executar_dijkstra_lista(lista, 0, 3,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &resultado));
    conferir("Matriz rejeita aresta sem peso", DIJKSTRA_ARESTA_SEM_PESO,
        executar_dijkstra_matriz(matriz, 0, 3,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &resultado));
    conferir("Modo ignorar executa", DIJKSTRA_SUCESSO,
        executar_dijkstra_lista(lista, 0, 2,
            DIJKSTRA_IGNORAR_ARESTA_SEM_PESO, &resultado));
    if (resultado != NULL) {
        conferir("Caminho via aresta ignorada inexiste", 0, resultado->alcancavel);
        conferir("Sem caminho vazio", 0, resultado->quantidade_vertices_caminho);
        conferir("Processa componente ponderada", 2,
                 resultado->quantidade_vertices_processados);
    }
    liberar_resultado_dijkstra(&resultado);
    conferir("Vertice isolado inalcançavel", DIJKSTRA_SUCESSO,
        executar_dijkstra_matriz(matriz, 0, 3,
            DIJKSTRA_IGNORAR_ARESTA_SEM_PESO, &resultado));
    if (resultado != NULL) conferir("Desconectado", 0, resultado->alcancavel);
    liberar_resultado_dijkstra(&resultado);
    destruir_grafos(lista, &matriz);
}

static void testar_invalidos_e_overflow(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoDijkstra *resultado = NULL;
    ResultadoDijkstra sentinela = {0};
    ResultadoDijkstra *ocupado = &sentinela;
    puts("\nARGUMENTOS INVALIDOS E OVERFLOW");
    criar_grafos(3, &lista, &matriz);
    inserir_peso(lista, matriz, 0, 1, DBL_MAX,
                 ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    inserir_peso(lista, matriz, 1, 2, DBL_MAX,
                 ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    conferir("Origem invalida", DIJKSTRA_ARGUMENTO_INVALIDO,
        executar_dijkstra_lista(lista, 3, 0,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &resultado));
    conferir("Destino invalido", DIJKSTRA_ARGUMENTO_INVALIDO,
        executar_dijkstra_matriz(matriz, 0, 3,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &resultado));
    conferir("Saida ocupada", DIJKSTRA_ARGUMENTO_INVALIDO,
        executar_dijkstra_lista(lista, 0, 1,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &ocupado));
    conferir("Politica invalida", DIJKSTRA_ARGUMENTO_INVALIDO,
        executar_dijkstra_lista(lista, 0, 1, (PoliticaPesoDijkstra)99,
                                &resultado));
    conferir("Overflow lista", DIJKSTRA_OVERFLOW,
        executar_dijkstra_lista(lista, 0, 2,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &resultado));
    conferir("Overflow matriz", DIJKSTRA_OVERFLOW,
        executar_dijkstra_matriz(matriz, 0, 2,
            DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &resultado));
    conferir("Overflow nao entrega resultado", 1, resultado == NULL);
    destruir_grafos(lista, &matriz);
}

#ifdef DIJKSTRA_TESTAR_ALOCACAO
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
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoDijkstra *resultado = NULL;
    size_t total_alocacoes;
    puts("\nFALHAS DE ALOCACAO");
    criar_grafos(2, &lista, &matriz);
    inserir_peso(lista, matriz, 0, 1, 1.0,
                 ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    falhar_na_chamada = 0;
    chamada_alocacao = 0;
    executar_dijkstra_lista(lista, 0, 1,
        DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &resultado);
    total_alocacoes = chamada_alocacao;
    liberar_resultado_dijkstra(&resultado);
    conferir("Baseline sem vazamento", 0, blocos_vivos);
    for (size_t i = 1; i <= total_alocacoes; i++) {
        falhar_na_chamada = i;
        chamada_alocacao = 0;
        conferir("Falha retorna sem memoria", DIJKSTRA_SEM_MEMORIA,
            executar_dijkstra_lista(lista, 0, 1,
                DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &resultado));
        conferir("Falha nao entrega resultado", 1, resultado == NULL);
        conferir("Falha nao vaza memoria", 0, blocos_vivos);
    }
    falhar_na_chamada = 0;
    destruir_grafos(lista, &matriz);
}
#endif

int main(void)
{
    testar_solucao_e_equivalencia();
    testar_zero_e_mesma_origem();
    testar_desconectado_e_sem_peso();
    testar_invalidos_e_overflow();
#ifdef DIJKSTRA_TESTAR_ALOCACAO
    testar_falhas_alocacao();
#endif
    printf("\nRESULTADO DIJKSTRA: %s (%zu verificacoes, %zu falhas)\n",
           falhas == 0 ? "PASSOU" : "FALHOU", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
