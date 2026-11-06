#include "agm.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    void *endereco;
} Alocacao;

static Alocacao alocacoes[64];
static size_t chamadas;
static size_t falhar_na_chamada;
static size_t blocos_ativos;
static size_t falhas;

static void verificar(const char *descricao, int correto)
{
    printf("%s: %s\n", descricao, correto ? "PASSOU" : "FALHOU");
    if (!correto) falhas++;
}

void *teste_malloc(size_t tamanho)
{
    void *endereco;
    chamadas++;
    if (chamadas == falhar_na_chamada) return NULL;
    endereco = malloc(tamanho);
    if (endereco == NULL) return NULL;
    for (size_t i = 0; i < sizeof(alocacoes) / sizeof(alocacoes[0]); i++) {
        if (alocacoes[i].endereco == NULL) {
            alocacoes[i].endereco = endereco;
            blocos_ativos++;
            return endereco;
        }
    }
    free(endereco);
    return NULL;
}

void teste_free(void *endereco)
{
    if (endereco == NULL) return;
    for (size_t i = 0; i < sizeof(alocacoes) / sizeof(alocacoes[0]); i++) {
        if (alocacoes[i].endereco == endereco) {
            alocacoes[i].endereco = NULL;
            blocos_ativos--;
            free(endereco);
            return;
        }
    }
    falhas++;
}

static GrafoLista *criar_grafo(void)
{
    GrafoLista *grafo = criar_grafo_lista();
    if (grafo == NULL) return NULL;
    for (size_t vertice = 0; vertice < 4; vertice++) {
        size_t indice;
        if (inserir_vertice_lista(grafo, &indice) != LISTA_SUCESSO) {
            liberar_grafo_lista(grafo);
            return NULL;
        }
    }
    if (inserir_aresta_ponderada_lista(grafo, 0, 1, 1,
            ORIGEM_PESO_COMPRIMENTO_MEDIDO) != LISTA_SUCESSO ||
        inserir_aresta_ponderada_lista(grafo, 0, 2, 4,
            ORIGEM_PESO_COMPRIMENTO_MEDIDO) != LISTA_SUCESSO ||
        inserir_aresta_ponderada_lista(grafo, 1, 2, 2,
            ORIGEM_PESO_COMPRIMENTO_MEDIDO) != LISTA_SUCESSO ||
        inserir_aresta_ponderada_lista(grafo, 1, 3, 5,
            ORIGEM_PESO_COMPRIMENTO_MEDIDO) != LISTA_SUCESSO ||
        inserir_aresta_ponderada_lista(grafo, 2, 3, 1,
            ORIGEM_PESO_COMPRIMENTO_MEDIDO) != LISTA_SUCESSO) {
        liberar_grafo_lista(grafo);
        return NULL;
    }
    return grafo;
}

static void testar_falhas_algoritmo(const GrafoLista *grafo, int usar_prim)
{
    ResultadoAGM resultado = {0};
    StatusAGM status;
    size_t total_alocacoes;
    const char *nome = usar_prim ? "Prim" : "Kruskal";

    chamadas = 0;
    falhar_na_chamada = 0;
    status = usar_prim ? agm_prim_lista(grafo, &resultado) :
                         agm_kruskal_lista(grafo, &resultado);
    verificar("Execucao preliminar bem-sucedida", status == AGM_SUCESSO);
    total_alocacoes = chamadas;
    liberar_resultado_agm(&resultado);
    verificar("Execucao preliminar sem vazamentos", blocos_ativos == 0);

    for (size_t etapa = 1; etapa <= total_alocacoes; etapa++) {
        chamadas = 0;
        falhar_na_chamada = etapa;
        status = usar_prim ? agm_prim_lista(grafo, &resultado) :
                             agm_kruskal_lista(grafo, &resultado);
        verificar(nome, status == AGM_SEM_MEMORIA);
        verificar("Falha nao publica saida", resultado.arestas == NULL &&
                  resultado.quantidade_arestas == 0);
        verificar("Falha libera todas as alocacoes", blocos_ativos == 0);
    }
    falhar_na_chamada = 0;
}

int main(void)
{
    GrafoLista *grafo = criar_grafo();
    if (grafo == NULL) {
        puts("Falha ao criar entrada do teste.");
        return EXIT_FAILURE;
    }
    testar_falhas_algoritmo(grafo, 1);
    testar_falhas_algoritmo(grafo, 0);
    liberar_grafo_lista(grafo);
    printf("\nFalhas de alocacao: %zu falhas.\n", falhas);
    return falhas == 0 && blocos_ativos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
