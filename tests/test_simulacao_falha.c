#include "simulacao_falha.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef SIMULACAO_TESTAR_ALOCACAO
static size_t blocos_vivos;
static size_t tentativas_alocacao;
static size_t falhar_na_tentativa = SIZE_MAX;

void *teste_malloc(size_t tamanho)
{
    void *bloco;
    if (tentativas_alocacao++ == falhar_na_tentativa) {
        return NULL;
    }
    bloco = malloc(tamanho);
    if (bloco != NULL) {
        blocos_vivos++;
    }
    return bloco;
}

void teste_free(void *ponteiro)
{
    if (ponteiro != NULL) {
        blocos_vivos--;
    }
    free(ponteiro);
}
#endif

static size_t verificacoes;
static size_t falhas;
static const size_t arestas[][2] = {
    {0, 1}, {1, 2}, {2, 3}, {2, 4}, {4, 5}
};

static void conferir(const char *descricao, size_t esperado, size_t obtido)
{
    verificacoes++;
    if (esperado != obtido) {
        falhas++;
    }
    printf("%s: esperado=%zu obtido=%zu %s\n", descricao, esperado, obtido,
           esperado == obtido ? "PASSOU" : "FALHOU");
}

static int criar_grafos(GrafoLista **lista, MatrizAdjacencia **matriz)
{
    *lista = criar_grafo_lista();
    if (*lista == NULL ||
        criar_matriz(6, 1024 * 1024, matriz) != MATRIZ_SUCESSO) {
        liberar_grafo_lista(*lista);
        *lista = NULL;
        destruir_matriz(matriz);
        return 0;
    }
    for (size_t vertice = 0; vertice < 6; vertice++) {
        size_t indice = SIZE_MAX;
        if (inserir_vertice_lista(*lista, &indice) != LISTA_SUCESSO ||
            indice != vertice) {
            liberar_grafo_lista(*lista);
            *lista = NULL;
            destruir_matriz(matriz);
            return 0;
        }
    }
    for (size_t indice = 0; indice < sizeof(arestas) / sizeof(arestas[0]); indice++) {
        if (inserir_aresta_lista(*lista, arestas[indice][0], arestas[indice][1])
                != LISTA_SUCESSO ||
            inserir_aresta(*matriz, arestas[indice][0], arestas[indice][1])
                != MATRIZ_SUCESSO) {
            liberar_grafo_lista(*lista);
            *lista = NULL;
            destruir_matriz(matriz);
            return 0;
        }
    }
    return 1;
}

static void conferir_resultado(const char *representacao,
                               const ResultadoSimulacaoFalha *resultado)
{
    printf("\nRESULTADO OBTIDO - %s\n", representacao);
    if (resultado == NULL) {
        conferir("Resultado disponivel", 1, 0);
        return;
    }
    imprimir_resultado_simulacao(resultado, stdout);
    conferir("Componentes antes", 1,
             resultado->componentes_antes->quantidade_componentes);
    conferir("Componentes depois", 2,
             resultado->componentes_depois->quantidade_componentes);
    conferir("Vertices na regiao principal", 2,
             resultado->vertices_componente_principal);
    conferir("Vertices isolados", 4, resultado->quantidade_isolados);
    conferir("Novos vertices isolados", 4,
             resultado->quantidade_novos_isolados);
    conferir("Houve separacao", 1, resultado->houve_separacao);
    conferir("Componentes apos restauracao", 1,
             resultado->componentes_apos_restauracao->quantidade_componentes);
    conferir("Topologia restaurada", 1, resultado->topologia_restaurada);
    conferir("Subestacao com vertice 1", 1,
             resultado->componentes_depois->componente_por_vertice[0] ==
             resultado->componentes_depois->componente_por_vertice[1]);
    for (size_t vertice = 2; vertice < 6; vertice++) {
        conferir("Vertice fora da componente principal", 1,
                 resultado->componentes_depois->componente_por_vertice[vertice] !=
                 resultado->componente_principal_depois);
    }
}

static void conferir_dot(const GrafoLista *lista,
                         const ResultadoSimulacaoFalha *resultado)
{
    FILE *arquivo = tmpfile();
    char texto[4096] = {0};
    size_t lidos = 0;
    conferir("Abrir arquivo DOT temporario", 1, arquivo != NULL);
    if (arquivo == NULL) {
        return;
    }
    conferir("Exportar DOT pos-falha", 1,
             exportar_simulacao_dot_lista(lista, resultado,
                                          REDE_DEPOIS_FALHA, arquivo));
    rewind(arquivo);
    lidos = fread(texto, 1, sizeof(texto) - 1, arquivo);
    texto[lidos] = '\0';
    conferir("DOT identifica falha", 1,
             strstr(texto, "label=\"falha\"") != NULL);
    conferir("DOT identifica componente principal", 1,
             strstr(texto, "fillcolor=palegreen") != NULL);
    conferir("DOT identifica ilha", 1,
             strstr(texto, "fillcolor=lightcoral") != NULL);
    conferir("DOT identifica subestacao", 1,
             strstr(texto, "shape=doublecircle") != NULL);
    fclose(arquivo);
}

static void testar_cenario_controlado(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoSimulacaoFalha *resultado_lista = NULL;
    ResultadoSimulacaoFalha *resultado_matriz = NULL;
    int existe = 0;

    puts("RESULTADO ESPERADO");
    puts("Antes: 1 componente {0,1,2,3,4,5}");
    puts("Falha: aresta 1 -- 2; subestacao: 0");
    puts("Depois: componente principal {0,1}; ilha topologica {2,3,4,5}");
    puts("Depois: 2 componentes; 4 vertices isolados");
    puts("Restauracao: 1 componente e aresta 1 -- 2 presente\n");
    conferir("Criar grafo controlado", 1, criar_grafos(&lista, &matriz));
    if (lista == NULL || matriz == NULL) {
        return;
    }
    conferir("Simular lista", SIMULACAO_SUCESSO,
             simular_falha_aresta_lista(lista, 1, 2, 0, &resultado_lista));
    conferir("Simular matriz", SIMULACAO_SUCESSO,
             simular_falha_aresta_matriz(matriz, 1, 2, 0, &resultado_matriz));
    conferir_resultado("LISTA DE ADJACENCIA", resultado_lista);
    conferir_resultado("MATRIZ DE ADJACENCIA", resultado_matriz);
    if (resultado_lista != NULL && resultado_matriz != NULL) {
        for (size_t vertice = 0; vertice < 6; vertice++) {
            conferir("Lista e matriz: mesma particao pos-falha", 1,
                     resultado_lista->componentes_depois->componente_por_vertice[vertice] ==
                     resultado_matriz->componentes_depois->componente_por_vertice[vertice]);
        }
        conferir_dot(lista, resultado_lista);
    }
    conferir("Aresta restaurada na lista", LISTA_SUCESSO,
             existe_aresta_lista(lista, 1, 2, &existe));
    conferir("Aresta presente na lista", 1, existe);
    conferir("Consultar restauracao na matriz", MATRIZ_SUCESSO,
             consultar_adjacencia(matriz, 1, 2, &existe));
    conferir("Aresta presente na matriz", 1, existe);
    liberar_resultado_simulacao(&resultado_lista);
    liberar_resultado_simulacao(&resultado_matriz);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

static void testar_erros(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoSimulacaoFalha *resultado = NULL;
    if (!criar_grafos(&lista, &matriz)) {
        conferir("Criar grafo para erros", 1, 0);
        return;
    }
    puts("\nERROS CONTROLADOS");
    conferir("Subestacao ausente", SIMULACAO_SUBESTACAO_NAO_INFORMADA,
             simular_falha_aresta_lista(lista, 1, 2, SIZE_MAX, &resultado));
    conferir("Vertice da aresta inexistente", SIMULACAO_VERTICE_INEXISTENTE,
             simular_falha_aresta_lista(lista, 1, 6, 0, &resultado));
    conferir("Subestacao inexistente", SIMULACAO_VERTICE_INEXISTENTE,
             simular_falha_aresta_matriz(matriz, 1, 2, 6, &resultado));
    conferir("Aresta inexistente", SIMULACAO_ARESTA_INEXISTENTE,
             simular_falha_aresta_matriz(matriz, 0, 5, 0, &resultado));
    conferir("Desativar manualmente", MATRIZ_SUCESSO, remover_aresta(matriz, 1, 2));
    conferir("Aresta ja desativada", SIMULACAO_ARESTA_INEXISTENTE,
             simular_falha_aresta_matriz(matriz, 1, 2, 0, &resultado));
    conferir("Restaurar apos teste de erro", MATRIZ_SUCESSO,
             inserir_aresta(matriz, 1, 2));
    conferir("Saida preservada nos erros", 1, resultado == NULL);
    conferir("Mensagem de erro compreensivel", 1,
             strstr(descricao_status_simulacao(SIMULACAO_ARESTA_INEXISTENTE),
                    "desativada") != NULL);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

#ifdef SIMULACAO_TESTAR_ALOCACAO
static void testar_falhas_alocacao(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoSimulacaoFalha *resultado = NULL;
    int existe = 0;
    size_t total_alocacoes;
    if (!criar_grafos(&lista, &matriz)) {
        conferir("Criar grafo para alocacao", 1, 0);
        return;
    }
    tentativas_alocacao = 0;
    conferir("Referencia de alocacao", SIMULACAO_SUCESSO,
             simular_falha_aresta_lista(lista, 1, 2, 0, &resultado));
    total_alocacoes = tentativas_alocacao;
    liberar_resultado_simulacao(&resultado);
    conferir("Blocos vivos apos referencia", 0, blocos_vivos);
    conferir("Ha pontos de alocacao", 1, total_alocacoes > 0);
    for (size_t tentativa = 0; tentativa < total_alocacoes; tentativa++) {
        tentativas_alocacao = 0;
        falhar_na_tentativa = tentativa;
        conferir("Falha de alocacao propagada", SIMULACAO_SEM_MEMORIA,
                 simular_falha_aresta_lista(lista, 1, 2, 0, &resultado));
        falhar_na_tentativa = SIZE_MAX;
        conferir("Saida NULL apos falha", 1, resultado == NULL);
        conferir("Sem vazamentos apos falha", 0, blocos_vivos);
        existe = 0;
        conferir("Consultar aresta apos falha", LISTA_SUCESSO,
                 existe_aresta_lista(lista, 1, 2, &existe));
        conferir("Rede restaurada apos falha de memoria", 1, existe);
    }
    printf("ALOCACAO: %zu pontos exercitados\n", total_alocacoes);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}
#endif

int main(void)
{
    testar_cenario_controlado();
    testar_erros();
#ifdef SIMULACAO_TESTAR_ALOCACAO
    testar_falhas_alocacao();
    conferir("Blocos vivos finais", 0, blocos_vivos);
#endif
    puts("DATASET REAL: NAO EXECUTADO / PENDENTE (arquivos e carregador #4 ausentes)");
    printf("RESULTADO SIMULACAO: %s (%zu verificacoes, %zu falhas)\n",
           falhas == 0 ? "PASSOU" : "FALHOU", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
