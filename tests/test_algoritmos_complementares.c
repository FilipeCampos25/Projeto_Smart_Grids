#include "algoritmos_complementares.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static size_t verificacoes;
static size_t falhas;

/* Compara e sempre mostra resultado esperado, obtido e PASSOU/FALHOU. */
static void verificar(const char *descricao, size_t esperado, size_t obtido)
{
    verificacoes++;
    printf("%s: esperado=%zu obtido=%zu %s\n", descricao, esperado, obtido,
           esperado == obtido ? "PASSOU" : "FALHOU");
    if (esperado != obtido) {
        falhas++;
    }
}

/* Cria N vertices isolados na lista. O chamador libera o resultado. */
static GrafoLista *criar_lista(size_t quantidade_vertices)
{
    GrafoLista *grafo = criar_grafo_lista();
    verificar("Criar lista", 1, grafo != NULL);
    if (grafo == NULL) {
        return NULL;
    }
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        size_t indice = SIZE_MAX;
        ResultadoLista resultado = inserir_vertice_lista(grafo, &indice);
        verificar("Inserir vertice", LISTA_SUCESSO, resultado);
        verificar("Indice do vertice", vertice, indice);
        if (resultado != LISTA_SUCESSO) {
            liberar_grafo_lista(grafo);
            return NULL;
        }
    }
    return grafo;
}

/* Constroi uma matriz direcionada pequena, com teto bem acima do necessario. */
static MatrizAdjacencia *criar_digrafo(size_t quantidade_vertices)
{
    MatrizAdjacencia *digrafo = NULL;
    ResultadoMatriz resultado = criar_matriz_direcionada(
        quantidade_vertices, 4096, &digrafo);
    verificar("Criar digrafo", MATRIZ_SUCESSO, resultado);
    verificar("Tipo direcionado", 1, matriz_e_direcionada(digrafo));
    return digrafo;
}

/* Demonstra forte, somente fraca e desconectada em grafos de tres vertices. */
static void testar_conectividade_digrafos(void)
{
    MatrizAdjacencia *forte = criar_digrafo(3);
    MatrizAdjacencia *fraco = criar_digrafo(3);
    MatrizAdjacencia *desconectado = criar_digrafo(3);
    int conectividade_fraca = -1;
    int conectividade_forte = -1;
    int arco = -1;

    puts("\nCONECTIVIDADE EM DIGRAFOS");
    if (forte == NULL || fraco == NULL || desconectado == NULL) {
        destruir_matriz(&forte);
        destruir_matriz(&fraco);
        destruir_matriz(&desconectado);
        return;
    }
    /* Ciclo dirigido: 0 -> 1 -> 2 -> 0. */
    verificar("Arco forte 0->1", MATRIZ_SUCESSO, inserir_aresta(forte, 0, 1));
    verificar("Arco forte 1->2", MATRIZ_SUCESSO, inserir_aresta(forte, 1, 2));
    verificar("Arco forte 2->0", MATRIZ_SUCESSO, inserir_aresta(forte, 2, 0));
    verificar("Analisar ciclo dirigido", ALGORITMO_SUCESSO,
              verificar_conectividade_digrafo(
                  forte, &conectividade_fraca, &conectividade_forte));
    verificar("Ciclo: conectividade fraca", 1, conectividade_fraca);
    verificar("Ciclo: conectividade forte", 1, conectividade_forte);
    consultar_adjacencia(forte, 1, 0, &arco);
    verificar("Insercao nao cria arco inverso", 0, arco);

    /* Caminho 0 -> 1 -> 2: conexo ao ignorar setas, mas nao fortemente. */
    inserir_aresta(fraco, 0, 1);
    inserir_aresta(fraco, 1, 2);
    verificar("Analisar caminho dirigido", ALGORITMO_SUCESSO,
              verificar_conectividade_digrafo(
                  fraco, &conectividade_fraca, &conectividade_forte));
    verificar("Caminho: conectividade fraca", 1, conectividade_fraca);
    verificar("Caminho: conectividade forte", 0, conectividade_forte);

    /* Somente 0 -> 1; o vertice 2 fica separado ate no grafo subjacente. */
    inserir_aresta(desconectado, 0, 1);
    verificar("Analisar digrafo desconectado", ALGORITMO_SUCESSO,
              verificar_conectividade_digrafo(
                  desconectado, &conectividade_fraca, &conectividade_forte));
    verificar("Desconectado: conectividade fraca", 0, conectividade_fraca);
    verificar("Desconectado: conectividade forte", 0, conectividade_forte);
    inserir_aresta(forte, 1, 0);
    remover_aresta(forte, 0, 1);
    consultar_adjacencia(forte, 1, 0, &arco);
    verificar("Remocao direcionada preserva arco inverso", 1, arco);

    destruir_matriz(&forte);
    destruir_matriz(&fraco);
    destruir_matriz(&desconectado);
}

/* Retorna 1 se o par nao ordenado aparece exatamente entre as pontes. */
static int contem_ponte(const ResultadoCriticidade *resultado,
                        size_t primeiro, size_t segundo)
{
    size_t origem = primeiro < segundo ? primeiro : segundo;
    size_t destino = primeiro < segundo ? segundo : primeiro;
    for (size_t indice = 0; indice < resultado->quantidade_pontes; indice++) {
        if (resultado->pontes[indice].origem == origem &&
            resultado->pontes[indice].destino == destino) {
            return 1;
        }
    }
    return 0;
}

/* Triangulo 0-1-2-0 com ramo 1-3 e folhas 4/5 ligadas a 3.
 * Articulacoes esperadas: 1 e 3. Pontes: 1-3, 3-4 e 3-5. */
static void testar_articulacoes_e_pontes(void)
{
    const size_t arestas[][2] = {
        {0, 1}, {1, 2}, {2, 0}, {1, 3}, {3, 4}, {3, 5}
    };
    const unsigned char articulacoes_esperadas[] = {0, 1, 0, 1, 0, 0};
    GrafoLista *grafo = criar_lista(6);
    ResultadoCriticidade *resultado = NULL;
    size_t divergencias = 0;

    puts("\nARTICULACOES E PONTES");
    if (grafo == NULL) {
        return;
    }
    for (size_t indice = 0; indice < 6; indice++) {
        verificar("Inserir aresta de criticidade", LISTA_SUCESSO,
                  inserir_aresta_lista(grafo, arestas[indice][0],
                                       arestas[indice][1]));
    }
    verificar("Analisar criticidade", ALGORITMO_SUCESSO,
              analisar_criticidade_lista(grafo, &resultado));
    verificar("Resultado de criticidade disponivel", 1, resultado != NULL);
    if (resultado != NULL) {
        for (size_t vertice = 0; vertice < 6; vertice++) {
            printf("Vertice %zu: articulacao esperada=%u obtida=%u\n", vertice,
                   articulacoes_esperadas[vertice],
                   resultado->articulacoes[vertice]);
            divergencias += resultado->articulacoes[vertice] !=
                            articulacoes_esperadas[vertice];
        }
        verificar("Quantidade de articulacoes", 2,
                  resultado->quantidade_articulacoes);
        verificar("Divergencias de articulacoes", 0, divergencias);
        verificar("Quantidade de pontes", 3, resultado->quantidade_pontes);
        verificar("Ponte (1,3)", 1, contem_ponte(resultado, 1, 3));
        verificar("Ponte (3,4)", 1, contem_ponte(resultado, 3, 4));
        verificar("Ponte (3,5)", 1, contem_ponte(resultado, 3, 5));
        verificar("Aresta do ciclo nao e ponte", 0,
                  contem_ponte(resultado, 0, 1));
    }
    liberar_resultado_criticidade(&resultado);
    verificar("Liberacao da criticidade", 1, resultado == NULL);
    liberar_grafo_lista(grafo);
}

/* Um ciclo simples nao tem articulacoes nem pontes. */
static void testar_ciclo_sem_criticidade(void)
{
    GrafoLista *grafo = criar_lista(4);
    ResultadoCriticidade *resultado = NULL;
    puts("\nCICLO SEM ARTICULACOES OU PONTES");
    if (grafo == NULL) {
        return;
    }
    inserir_aresta_lista(grafo, 0, 1);
    inserir_aresta_lista(grafo, 1, 2);
    inserir_aresta_lista(grafo, 2, 3);
    inserir_aresta_lista(grafo, 3, 0);
    verificar("Analisar ciclo", ALGORITMO_SUCESSO,
              analisar_criticidade_lista(grafo, &resultado));
    if (resultado != NULL) {
        verificar("Ciclo: articulacoes", 0,
                  resultado->quantidade_articulacoes);
        verificar("Ciclo: pontes", 0, resultado->quantidade_pontes);
    }
    liberar_resultado_criticidade(&resultado);
    liberar_grafo_lista(grafo);
}

/* Confere a formula, um conjunto incoerente e a limitacao da desigualdade. */
static void testar_euler(void)
{
    ResultadoEuler resultado = {-1, -1, -1};
    puts("\nFORMULA DE EULER E CONDICAO NECESSARIA");
    /* Cubo planar: 8 - 12 + 6 = 2. */
    verificar("Euler do cubo", ALGORITMO_SUCESSO,
              verificar_formula_euler(8, 12, 6, &resultado));
    verificar("Cubo satisfaz Euler", 1, resultado.satisfaz_formula);
    verificar("Limite aplicavel ao cubo", 1,
              resultado.limite_arestas_aplicavel);
    verificar("Cubo satisfaz E <= 3V-6", 1,
              resultado.satisfaz_limite_arestas);

    /* Estes valores dariam 4 - 4 + 3 = 3, portanto nao satisfazem Euler. */
    verificar("Euler inconsistente", ALGORITMO_SUCESSO,
              verificar_formula_euler(4, 4, 3, &resultado));
    verificar("Valores inconsistentes nao satisfazem Euler", 0,
              resultado.satisfaz_formula);

    /* K5 viola a condicao necessaria: 10 > 3*5-6 = 9. */
    verificar("Limite para K5", ALGORITMO_SUCESSO,
              verificar_formula_euler(5, 10, 0, &resultado));
    verificar("K5 viola E <= 3V-6", 0,
              resultado.satisfaz_limite_arestas);

    /* K3,3 satisfaz 9 <= 12, mas e nao planar: a desigualdade nao prova. */
    verificar("Limite para K3,3", ALGORITMO_SUCESSO,
              verificar_formula_euler(6, 9, 0, &resultado));
    verificar("K3,3 passa somente na condicao necessaria", 1,
              resultado.satisfaz_limite_arestas);
}

/* Ciclo impar de cinco vertices exige tres cores na ordem gulosa 0..4. */
static void testar_coloracao(void)
{
    const size_t arestas[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}};
    const size_t cores_esperadas[] = {0, 1, 0, 1, 2};
    GrafoLista *grafo = criar_lista(5);
    ResultadoColoracao *resultado = NULL;
    size_t divergencias = 0;
    size_t conflitos = 0;

    puts("\nCOLORACAO GULOSA");
    if (grafo == NULL) {
        return;
    }
    for (size_t indice = 0; indice < 5; indice++) {
        inserir_aresta_lista(grafo, arestas[indice][0], arestas[indice][1]);
    }
    verificar("Colorir ciclo impar", ALGORITMO_SUCESSO,
              colorir_grafo_guloso(grafo, &resultado));
    verificar("Resultado de coloracao disponivel", 1, resultado != NULL);
    if (resultado != NULL) {
        for (size_t vertice = 0; vertice < 5; vertice++) {
            printf("Vertice %zu: cor esperada=%zu cor obtida=%zu\n", vertice,
                   cores_esperadas[vertice], resultado->cores[vertice]);
            divergencias += resultado->cores[vertice] !=
                            cores_esperadas[vertice];
        }
        for (size_t indice = 0; indice < 5; indice++) {
            conflitos += resultado->cores[arestas[indice][0]] ==
                         resultado->cores[arestas[indice][1]];
        }
        verificar("Quantidade de cores", 3, resultado->quantidade_cores);
        verificar("Divergencias de cores", 0, divergencias);
        verificar("Arestas com cores iguais", 0, conflitos);
    }
    liberar_resultado_coloracao(&resultado);
    verificar("Liberacao da coloracao", 1, resultado == NULL);
    liberar_grafo_lista(grafo);
}

/* Exercita entradas invalidas sem alterar os ponteiros de saida. */
static void testar_entradas_invalidas(void)
{
    ResultadoCriticidade *criticidade = NULL;
    ResultadoColoracao *coloracao = NULL;
    ResultadoEuler euler = {7, 7, 7};
    int fraca = 7;
    int forte = 7;
    MatrizAdjacencia *matriz = NULL;
    size_t bytes = 0;

    puts("\nENTRADAS INVALIDAS");
    verificar("Criticidade NULL", ALGORITMO_ARGUMENTO_INVALIDO,
              analisar_criticidade_lista(NULL, &criticidade));
    verificar("Coloracao NULL", ALGORITMO_ARGUMENTO_INVALIDO,
              colorir_grafo_guloso(NULL, &coloracao));
    verificar("Euler sem saida", ALGORITMO_ARGUMENTO_INVALIDO,
              verificar_formula_euler(1, 0, 1, NULL));
    verificar("Euler detecta overflow", ALGORITMO_LIMITE_EXCEDIDO,
              verificar_formula_euler(SIZE_MAX, 0, 1, &euler));
    verificar("Euler preserva saida no erro", 7, (size_t)euler.satisfaz_formula);
    estimar_memoria_matriz(2, &bytes);
    criar_matriz(2, bytes, &matriz);
    verificar("Conectividade rejeita matriz nao direcionada",
              ALGORITMO_ARGUMENTO_INVALIDO,
              verificar_conectividade_digrafo(matriz, &fraca, &forte));
    verificar("Conectividade preserva saida no erro", 7, (size_t)fraca);
    destruir_matriz(&matriz);
    liberar_resultado_criticidade(NULL);
    liberar_resultado_coloracao(NULL);
}

int main(void)
{
    testar_conectividade_digrafos();
    testar_articulacoes_e_pontes();
    testar_ciclo_sem_criticidade();
    testar_euler();
    testar_coloracao();
    testar_entradas_invalidas();
    printf("\nRESULTADO ALGORITMOS COMPLEMENTARES: %s "
           "(%zu verificacoes, %zu falhas)\n",
           falhas == 0 ? "PASSOU" : "FALHOU", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
