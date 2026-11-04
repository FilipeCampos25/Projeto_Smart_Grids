#include "dataset.h"
#include "simulacao_falha.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static size_t verificacoes;
static size_t falhas;

static void conferir(const char *descricao, size_t esperado, size_t obtido)
{
    verificacoes++;
    if (esperado != obtido) {
        falhas++;
    }
    printf("%s: esperado=%zu obtido=%zu %s\n", descricao, esperado, obtido,
           esperado == obtido ? "PASSOU" : "FALHOU");
}

static void conferir_double(const char *descricao, double esperado,
                            double obtido)
{
    verificacoes++;
    if (esperado != obtido) {
        falhas++;
    }
    printf("%s: esperado=%.17g obtido=%.17g %s\n", descricao, esperado,
           obtido, esperado == obtido ? "PASSOU" : "FALHOU");
}

static int criar_topologias(size_t n, GrafoLista **lista,
                            MatrizAdjacencia **matriz)
{
    size_t bytes;
    *lista = criar_grafo_lista();
    if (*lista == NULL ||
        estimar_memoria_matriz_ponderada(n, &bytes) != MATRIZ_SUCESSO ||
        criar_matriz(n, bytes, matriz) != MATRIZ_SUCESSO) {
        liberar_grafo_lista(*lista);
        *lista = NULL;
        destruir_matriz(matriz);
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        size_t indice;
        if (inserir_vertice_lista(*lista, &indice) != LISTA_SUCESSO ||
            indice != i) {
            liberar_grafo_lista(*lista);
            *lista = NULL;
            destruir_matriz(matriz);
            return 0;
        }
    }
    return 1;
}

static void testar_contrato_basico(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    PesoAresta peso_lista = {1, 77.0, ORIGEM_PESO_CUSTO_ASSUMIDO};
    PesoAresta peso_matriz = peso_lista;
    int existe = 0;

    puts("\nCONTRATO BASICO DE PESOS");
    conferir("Criar topologias", 1, criar_topologias(5, &lista, &matriz));
    if (lista == NULL || matriz == NULL) return;

    conferir("Inserir zero na lista", LISTA_SUCESSO,
        inserir_aresta_ponderada_lista(lista, 0, 1, 0.0,
                                       ORIGEM_PESO_COMPRIMENTO_MEDIDO));
    conferir("Inserir zero na matriz", MATRIZ_SUCESSO,
        inserir_aresta_ponderada(matriz, 0, 1, 0.0,
                                 ORIGEM_PESO_COMPRIMENTO_MEDIDO));
    conferir("Consultar zero na lista", LISTA_SUCESSO,
             consultar_peso_aresta_lista(lista, 0, 1, &peso_lista));
    conferir("Consultar zero na matriz", MATRIZ_SUCESSO,
             consultar_peso_aresta(matriz, 0, 1, &peso_matriz));
    conferir("Zero possui peso na lista", 1, peso_lista.possui_peso);
    conferir("Zero possui peso na matriz", 1, peso_matriz.possui_peso);
    conferir_double("Valor zero na lista", 0.0, peso_lista.valor);
    conferir_double("Valor zero na matriz", 0.0, peso_matriz.valor);
    conferir("Origem medida na lista", ORIGEM_PESO_COMPRIMENTO_MEDIDO,
             peso_lista.origem);
    conferir("Origem medida na matriz", ORIGEM_PESO_COMPRIMENTO_MEDIDO,
             peso_matriz.origem);

    conferir("Inserir sem peso na lista", LISTA_SUCESSO,
             inserir_aresta_lista(lista, 1, 2));
    conferir("Inserir sem peso na matriz", MATRIZ_SUCESSO,
             inserir_aresta(matriz, 1, 2));
    conferir("Consultar sem peso na lista", LISTA_SUCESSO,
             consultar_peso_aresta_lista(lista, 1, 2, &peso_lista));
    conferir("Consultar sem peso na matriz", MATRIZ_SUCESSO,
             consultar_peso_aresta(matriz, 1, 2, &peso_matriz));
    conferir("Lista distingue sem peso", 0, peso_lista.possui_peso);
    conferir("Matriz distingue sem peso", 0, peso_matriz.possui_peso);

    conferir("Ausente na lista", LISTA_ARESTA_INEXISTENTE,
             consultar_peso_aresta_lista(lista, 3, 4, &peso_lista));
    conferir("Ausente na matriz", MATRIZ_ARESTA_INEXISTENTE,
             consultar_peso_aresta(matriz, 3, 4, &peso_matriz));
    conferir_double("Erro preserva saida da lista", 0.0, peso_lista.valor);
    conferir_double("Erro preserva saida da matriz", 0.0, peso_matriz.valor);

    conferir("Negativo rejeitado na lista", LISTA_PESO_INVALIDO,
        inserir_aresta_ponderada_lista(lista, 2, 3, -1.0,
                                       ORIGEM_PESO_CUSTO_ASSUMIDO));
    conferir("NaN rejeitado na matriz", MATRIZ_PESO_INVALIDO,
        inserir_aresta_ponderada(matriz, 2, 3, NAN,
                                 ORIGEM_PESO_CUSTO_ASSUMIDO));
    conferir("Infinito rejeitado na lista", LISTA_PESO_INVALIDO,
        inserir_aresta_ponderada_lista(lista, 2, 3, INFINITY,
                                       ORIGEM_PESO_CUSTO_ASSUMIDO));
    conferir("Origem ausente rejeitada", MATRIZ_PESO_INVALIDO,
        inserir_aresta_ponderada(matriz, 2, 3, 1.0,
                                 ORIGEM_PESO_NAO_INFORMADA));
    conferir("Invalidos nao criam aresta na lista", LISTA_SUCESSO,
             existe_aresta_lista(lista, 2, 3, &existe));
    conferir("Aresta invalida ausente na lista", 0, existe);
    conferir("Invalidos nao criam aresta na matriz", MATRIZ_SUCESSO,
             consultar_adjacencia(matriz, 2, 3, &existe));
    conferir("Aresta invalida ausente na matriz", 0, existe);

    conferir("Extremo finito na lista", LISTA_SUCESSO,
        inserir_aresta_ponderada_lista(lista, 3, 4, DBL_MAX,
                                       ORIGEM_PESO_CUSTO_ASSUMIDO));
    conferir("Extremo finito na matriz", MATRIZ_SUCESSO,
        inserir_aresta_ponderada(matriz, 3, 4, DBL_MAX,
                                 ORIGEM_PESO_CUSTO_ASSUMIDO));
    conferir("Consultar extremo na lista", LISTA_SUCESSO,
             consultar_peso_aresta_lista(lista, 3, 4, &peso_lista));
    conferir("Consultar extremo na matriz", MATRIZ_SUCESSO,
             consultar_peso_aresta(matriz, 3, 4, &peso_matriz));
    conferir_double("Extremo preservado na lista", DBL_MAX, peso_lista.valor);
    conferir_double("Extremo preservado na matriz", DBL_MAX, peso_matriz.valor);

    conferir("Remover peso da lista", LISTA_SUCESSO,
             remover_aresta_lista(lista, 3, 4));
    conferir("Remover peso da matriz", MATRIZ_SUCESSO,
             remover_aresta(matriz, 3, 4));
    conferir("Peso removido da lista", LISTA_ARESTA_INEXISTENTE,
             consultar_peso_aresta_lista(lista, 3, 4, &peso_lista));
    conferir("Peso removido da matriz", MATRIZ_ARESTA_INEXISTENTE,
             consultar_peso_aresta(matriz, 3, 4, &peso_matriz));

    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

static void testar_limite_matriz(void)
{
    MatrizAdjacencia *matriz = NULL;
    size_t bytes;
    int existe = 7;
    conferir("Estimar matriz sem pesos", MATRIZ_SUCESSO,
             estimar_memoria_matriz(4, &bytes));
    conferir("Criar matriz com limite topologico", MATRIZ_SUCESSO,
             criar_matriz(4, bytes, &matriz));
    if (matriz == NULL) return;
    conferir("Peso respeita limite", MATRIZ_LIMITE_MEMORIA,
        inserir_aresta_ponderada(matriz, 0, 1, 2.0,
                                 ORIGEM_PESO_CUSTO_ASSUMIDO));
    conferir("Falha de limite preserva topologia", MATRIZ_SUCESSO,
             consultar_adjacencia(matriz, 0, 1, &existe));
    conferir("Falha de limite nao cria aresta", 0, existe);
    destruir_matriz(&matriz);
}

static void testar_restauracao_ponderada(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoSimulacaoFalha *resultado_lista = NULL;
    ResultadoSimulacaoFalha *resultado_matriz = NULL;
    PesoAresta peso_lista = {0};
    PesoAresta peso_matriz = {0};
    conferir("Criar topologias para simulacao", 1,
             criar_topologias(3, &lista, &matriz));
    if (lista == NULL || matriz == NULL) return;
    inserir_aresta_lista(lista, 0, 1);
    inserir_aresta(matriz, 0, 1);
    inserir_aresta_ponderada_lista(lista, 1, 2, 12.5,
                                   ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    inserir_aresta_ponderada(matriz, 1, 2, 12.5,
                             ORIGEM_PESO_COMPRIMENTO_MEDIDO);
    conferir("Simular lista ponderada", SIMULACAO_SUCESSO,
             simular_falha_aresta_lista(lista, 1, 2, 0, &resultado_lista));
    conferir("Simular matriz ponderada", SIMULACAO_SUCESSO,
             simular_falha_aresta_matriz(matriz, 1, 2, 0, &resultado_matriz));
    conferir("Consultar peso restaurado lista", LISTA_SUCESSO,
             consultar_peso_aresta_lista(lista, 1, 2, &peso_lista));
    conferir("Consultar peso restaurado matriz", MATRIZ_SUCESSO,
             consultar_peso_aresta(matriz, 1, 2, &peso_matriz));
    conferir_double("Valor restaurado lista", 12.5, peso_lista.valor);
    conferir_double("Valor restaurado matriz", 12.5, peso_matriz.valor);
    conferir("Origem restaurada lista", ORIGEM_PESO_COMPRIMENTO_MEDIDO,
             peso_lista.origem);
    conferir("Origem restaurada matriz", ORIGEM_PESO_COMPRIMENTO_MEDIDO,
             peso_matriz.origem);
    liberar_resultado_simulacao(&resultado_lista);
    liberar_resultado_simulacao(&resultado_matriz);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

static void testar_dataset_real(void)
{
    Dataset *dataset = NULL;
    GrafoLista *lista = NULL;
    GrafoLista *lista_sem_assuncao = NULL;
    MatrizAdjacencia *matriz = NULL;
    ErroDataset erro;
    ConfiguracaoPesosDataset assumida = {
        PESO_AUSENTE_USAR_CUSTO_ASSUMIDO, 0.25
    };
    ConfiguracaoPesosDataset ausente = {
        PESO_AUSENTE_MANTER_SEM_PESO, NAN
    };
    ConfiguracaoPesosDataset invalida = {
        PESO_AUSENTE_USAR_CUSTO_ASSUMIDO, INFINITY
    };
    size_t medidos = 0;
    size_t assumidos = 0;
    size_t sem_peso = 0;
    size_t divergencias = 0;

    puts("\nDATASET REAL PONDERADO");
    conferir("Carregar dataset real", DATASET_SUCESSO,
        carregar_dataset("data/forcel_2025_circuito_1/vertices.csv",
                         "data/forcel_2025_circuito_1/arestas.csv",
                         &dataset, &erro));
    if (dataset == NULL) return;
    conferir("Continuam 1838 vertices", 1838, dataset->quantidade_vertices);
    conferir("Continuam 1837 arestas", 1837, dataset->quantidade_arestas);
    conferir("Configuracao infinita rejeitada", DATASET_ARGUMENTO_INVALIDO,
             construir_lista_ponderada_dataset(dataset, &invalida, &lista));
    conferir("Erro nao entrega lista", 1, lista == NULL);
    conferir("Construir lista ponderada", DATASET_SUCESSO,
             construir_lista_ponderada_dataset(dataset, &assumida, &lista));
    conferir("Construir matriz ponderada", DATASET_SUCESSO,
             construir_matriz_ponderada_dataset(dataset, &assumida, &matriz));
    conferir("Construir lista sem assuncao", DATASET_SUCESSO,
             construir_lista_ponderada_dataset(dataset, &ausente,
                                                &lista_sem_assuncao));
    if (lista == NULL || matriz == NULL || lista_sem_assuncao == NULL) goto fim;

    for (size_t i = 0; i < dataset->quantidade_arestas; i++) {
        const ArestaDataset *aresta = &dataset->arestas[i];
        PesoAresta pl = {0};
        PesoAresta pm = {0};
        PesoAresta ps = {0};
        if (consultar_peso_aresta_lista(lista, aresta->origem,
                                        aresta->destino, &pl) != LISTA_SUCESSO ||
            consultar_peso_aresta(matriz, aresta->origem,
                                  aresta->destino, &pm) != MATRIZ_SUCESSO ||
            consultar_peso_aresta_lista(lista_sem_assuncao, aresta->origem,
                                        aresta->destino, &ps) != LISTA_SUCESSO) {
            divergencias++;
            continue;
        }
        divergencias += pl.possui_peso != pm.possui_peso ||
                        pl.valor != pm.valor || pl.origem != pm.origem;
        if (aresta->possui_comprimento) {
            medidos += pl.possui_peso &&
                pl.origem == ORIGEM_PESO_COMPRIMENTO_MEDIDO &&
                pl.valor == aresta->comprimento_m;
        } else {
            assumidos += pl.possui_peso &&
                pl.origem == ORIGEM_PESO_CUSTO_ASSUMIDO && pl.valor == 0.25;
            sem_peso += !ps.possui_peso &&
                ps.origem == ORIGEM_PESO_NAO_INFORMADA;
        }
    }
    conferir("Pesos equivalentes lista/matriz", 0, divergencias);
    conferir("Comprimentos medidos preservados", 1533, medidos);
    conferir("Chaves marcadas como assumidas", 304, assumidos);
    conferir("Modo sem assuncao preserva ausencias", 304, sem_peso);

fim:
    liberar_grafo_lista(lista);
    liberar_grafo_lista(lista_sem_assuncao);
    destruir_matriz(&matriz);
    liberar_dataset(&dataset);
}

int main(void)
{
    testar_contrato_basico();
    testar_limite_matriz();
    testar_restauracao_ponderada();
    testar_dataset_real();
    printf("\nRESULTADO PESOS: %s (%zu verificacoes, %zu falhas)\n",
           falhas == 0 ? "PASSOU" : "FALHOU", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
