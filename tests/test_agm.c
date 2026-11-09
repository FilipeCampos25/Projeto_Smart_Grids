#include "agm.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static size_t verificacoes;
static size_t falhas;

static void verificar(const char *descricao, int esperado, int obtido)
{
    verificacoes++;
    if (esperado != obtido) {
        falhas++;
    }
    printf("%s: esperado=%d obtido=%d %s\n", descricao, esperado, obtido,
           esperado == obtido ? "PASSOU" : "FALHOU");
}

static void verificar_status(const char *descricao, StatusAGM esperado,
                             StatusAGM obtido)
{
    verificar(descricao, esperado, obtido);
}

static int custo_proximo(double a, double b)
{
    double escala = fmax(1.0, fmax(fabs(a), fabs(b)));
    return fabs(a - b) <= 1e-12 * escala;
}

static GrafoLista *criar_lista(size_t vertices)
{
    GrafoLista *lista = criar_grafo_lista();
    if (lista == NULL) return NULL;
    for (size_t vertice = 0; vertice < vertices; vertice++) {
        size_t indice = SIZE_MAX;
        if (inserir_vertice_lista(lista, &indice) != LISTA_SUCESSO ||
            indice != vertice) {
            liberar_grafo_lista(lista);
            return NULL;
        }
    }
    return lista;
}

static MatrizAdjacencia *criar_matriz_ponderada(size_t vertices)
{
    MatrizAdjacencia *matriz = NULL;
    size_t bytes = 0;
    if (estimar_memoria_matriz_ponderada(vertices, &bytes) != MATRIZ_SUCESSO ||
        criar_matriz(vertices, bytes, &matriz) != MATRIZ_SUCESSO) {
        destruir_matriz(&matriz);
        return NULL;
    }
    return matriz;
}

static int inserir_ponderada(GrafoLista *lista, MatrizAdjacencia *matriz,
                             size_t origem, size_t destino, double peso,
                             OrigemPesoAresta origem_peso)
{
    return inserir_aresta_ponderada_lista(lista, origem, destino, peso,
                                          origem_peso) == LISTA_SUCESSO &&
           inserir_aresta_ponderada(matriz, origem, destino, peso,
                                    origem_peso) == MATRIZ_SUCESSO;
}

static int inserir_aresta_teste(GrafoLista *lista, MatrizAdjacencia *matriz,
                                size_t origem, size_t destino, double peso)
{
    return inserir_ponderada(lista, matriz, origem, destino, peso,
                             ORIGEM_PESO_COMPRIMENTO_MEDIDO);
}

static size_t raiz(size_t *pais, size_t vertice)
{
    while (pais[vertice] != vertice) {
        pais[vertice] = pais[pais[vertice]];
        vertice = pais[vertice];
    }
    return vertice;
}

static int unir_teste(size_t *pais, size_t a, size_t b)
{
    a = raiz(pais, a);
    b = raiz(pais, b);
    if (a == b) return 0;
    pais[b] = a;
    return 1;
}

static void verificar_floresta(const char *nome, const GrafoLista *grafo,
                               const ResultadoAGM *resultado)
{
    size_t vertices = quantidade_vertices_lista(grafo);
    size_t *pais_entrada = malloc(vertices * sizeof(*pais_entrada));
    size_t *pais_saida = malloc(vertices * sizeof(*pais_saida));
    size_t componentes = 0;
    int sem_ciclos = pais_entrada != NULL && pais_saida != NULL;
    int cobertura = sem_ciclos;

    if (!sem_ciclos) {
        falhas++;
        free(pais_entrada);
        free(pais_saida);
        return;
    }
    for (size_t i = 0; i < vertices; i++) {
        pais_entrada[i] = i;
        pais_saida[i] = i;
    }
    for (size_t origem = 0; origem < vertices; origem++) {
        const VizinhoLista *vizinho = NULL;
        buscar_vizinhos_lista(grafo, origem, &vizinho);
        for (; vizinho != NULL; vizinho = proximo_vizinho_lista(vizinho)) {
            size_t destino = indice_vizinho_lista(vizinho);
            if (origem < destino) unir_teste(pais_entrada, origem, destino);
        }
    }
    for (size_t i = 0; i < resultado->quantidade_arestas; i++) {
        if (!unir_teste(pais_saida, resultado->arestas[i].origem,
                        resultado->arestas[i].destino)) {
            sem_ciclos = 0;
        }
    }
    for (size_t i = 0; i < vertices; i++) {
        if (raiz(pais_entrada, i) == i) componentes++;
        for (size_t j = 0; j < vertices; j++) {
            if ((raiz(pais_entrada, i) == raiz(pais_entrada, j)) !=
                (raiz(pais_saida, i) == raiz(pais_saida, j))) {
                cobertura = 0;
            }
        }
    }
    {
        char descricao[96];
        snprintf(descricao, sizeof(descricao), "%s: sem ciclos", nome);
        verificar(descricao, 1, sem_ciclos);
        snprintf(descricao, sizeof(descricao), "%s: cobre componentes", nome);
        verificar(descricao, 1, cobertura);
        snprintf(descricao, sizeof(descricao), "%s: arestas = V - C", nome);
        verificar(descricao, vertices - resultado->quantidade_componentes,
                  resultado->quantidade_arestas);
        snprintf(descricao, sizeof(descricao), "%s: componentes preservados",
                 nome);
        verificar(descricao, componentes, resultado->quantidade_componentes);
    }
    free(pais_entrada);
    free(pais_saida);
}

static void testar_grafo_com_ciclos(void)
{
    const size_t arestas[][3] = {
        {0, 1, 1}, {0, 2, 4}, {1, 2, 2}, {1, 3, 5}, {2, 3, 1}
    };
    GrafoLista *lista = criar_lista(4);
    MatrizAdjacencia *matriz = criar_matriz_ponderada(4);
    ResultadoAGM prim_lista = {0}, kruskal_lista = {0};
    ResultadoAGM prim_matriz = {0}, kruskal_matriz = {0};
    PesoAresta peso_original;

    puts("\nGRAFO CONECTADO COM CICLOS");
    verificar("Criar grafo de ciclo", 1, lista != NULL && matriz != NULL);
    if (lista == NULL || matriz == NULL) goto cleanup;
    for (size_t i = 0; i < sizeof(arestas) / sizeof(arestas[0]); i++) {
        verificar("Inserir aresta ponderada",
                  1, inserir_aresta_teste(lista, matriz, arestas[i][0],
                                          arestas[i][1], arestas[i][2]));
    }
    verificar_status("Prim lista", AGM_SUCESSO,
                     agm_prim_lista(lista, &prim_lista));
    verificar_status("Kruskal lista", AGM_SUCESSO,
                     agm_kruskal_lista(lista, &kruskal_lista));
    verificar_status("Prim matriz", AGM_SUCESSO,
                     agm_prim_matriz(matriz, &prim_matriz));
    verificar_status("Kruskal matriz", AGM_SUCESSO,
                     agm_kruskal_matriz(matriz, &kruskal_matriz));
    {
        ResultadoAGM *resultados[] = {
            &prim_lista, &kruskal_lista, &prim_matriz, &kruskal_matriz
        };
        for (size_t i = 0; i < 4; i++) {
            verificar("Quantidade AGM conhecida", 3,
                      (int)resultados[i]->quantidade_arestas);
            verificar("Custo AGM conhecido", 1, custo_proximo(4.0,
                      resultados[i]->custo_total));
            verificar("Arvore completa", 1, resultados[i]->arvore_completa);
            verificar_floresta("AGM conectada", lista, resultados[i]);
        }
    }
    verificar("Prim/Kruskal mesma lista", 1,
              custo_proximo(prim_lista.custo_total, kruskal_lista.custo_total));
    verificar("Prim/Kruskal mesma matriz", 1,
              custo_proximo(prim_matriz.custo_total,
                            kruskal_matriz.custo_total));
    verificar("Lista/matriz equivalentes", 1,
              custo_proximo(prim_lista.custo_total, prim_matriz.custo_total));
    verificar("Entrada continua sem mudanca", 5,
              (int)quantidade_arestas_lista(lista));
    verificar("Peso original preservado", LISTA_SUCESSO,
              consultar_peso_aresta_lista(lista, 0, 2, &peso_original));
    verificar("Origem medida preservada", ORIGEM_PESO_COMPRIMENTO_MEDIDO,
              peso_original.origem);

cleanup:
    liberar_resultado_agm(&prim_lista);
    liberar_resultado_agm(&kruskal_lista);
    liberar_resultado_agm(&prim_matriz);
    liberar_resultado_agm(&kruskal_matriz);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

static void testar_floresta_desconectada(void)
{
    GrafoLista *lista = criar_lista(6);
    MatrizAdjacencia *matriz = criar_matriz_ponderada(6);
    ResultadoAGM resultados[4] = {0};
    StatusAGM (*algoritmos_lista[])(const GrafoLista *, ResultadoAGM *) = {
        agm_prim_lista, agm_kruskal_lista
    };
    StatusAGM (*algoritmos_matriz[])(const MatrizAdjacencia *, ResultadoAGM *) = {
        agm_prim_matriz, agm_kruskal_matriz
    };

    puts("\nFLORESTA DESCONEXA E VERTICE ISOLADO");
    verificar("Criar grafo desconectado", 1, lista != NULL && matriz != NULL);
    if (lista == NULL || matriz == NULL) goto cleanup;
    inserir_aresta_teste(lista, matriz, 0, 1, 2);
    inserir_aresta_teste(lista, matriz, 1, 2, 1);
    inserir_aresta_teste(lista, matriz, 0, 2, 5);
    inserir_ponderada(lista, matriz, 3, 4, 0, ORIGEM_PESO_CUSTO_ASSUMIDO);
    for (size_t i = 0; i < 2; i++) {
        verificar_status("Floresta Prim/Kruskal lista", AGM_SUCESSO,
                         algoritmos_lista[i](lista, &resultados[i]));
        verificar_status("Floresta Prim/Kruskal matriz", AGM_SUCESSO,
                         algoritmos_matriz[i](matriz, &resultados[i + 2]));
    }
    for (size_t i = 0; i < 4; i++) {
        verificar("Floresta com tres componentes", 3,
                  (int)resultados[i].quantidade_componentes);
        verificar("Floresta com tres arestas", 3,
                  (int)resultados[i].quantidade_arestas);
        verificar("Custo da floresta", 1,
                  custo_proximo(3.0, resultados[i].custo_total));
        verificar("Nao e arvore completa", 0, resultados[i].arvore_completa);
        {
            int preservou_origem_assumida = 0;
            for (size_t j = 0; j < resultados[i].quantidade_arestas; j++) {
                if (resultados[i].arestas[j].origem == 3 &&
                    resultados[i].arestas[j].destino == 4 &&
                    resultados[i].arestas[j].peso.origem ==
                        ORIGEM_PESO_CUSTO_ASSUMIDO) {
                    preservou_origem_assumida = 1;
                }
            }
            verificar("Origem assumida preservada", 1,
                      preservou_origem_assumida);
        }
        verificar_floresta("Floresta desconexa", lista, &resultados[i]);
    }

cleanup:
    for (size_t i = 0; i < 4; i++) liberar_resultado_agm(&resultados[i]);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

static void testar_arvore_e_empates(void)
{
    GrafoLista *lista = criar_lista(4);
    MatrizAdjacencia *matriz = criar_matriz_ponderada(4);
    ResultadoAGM prim = {0}, kruskal = {0};

    puts("\nARVORE EXISTENTE E EMPATES");
    verificar("Criar arvore", 1, lista != NULL && matriz != NULL);
    if (lista == NULL || matriz == NULL) goto cleanup;
    inserir_aresta_teste(lista, matriz, 0, 1, 2);
    inserir_aresta_teste(lista, matriz, 1, 2, 3);
    inserir_aresta_teste(lista, matriz, 1, 3, 4);
    verificar_status("Prim na arvore", AGM_SUCESSO, agm_prim_lista(lista, &prim));
    verificar_status("Kruskal na arvore", AGM_SUCESSO,
                     agm_kruskal_matriz(matriz, &kruskal));
    verificar("Arvore retorna todas as arestas", 3,
              (int)prim.quantidade_arestas);
    verificar("Custo da arvore existente", 1, custo_proximo(9, prim.custo_total));
    verificar_floresta("Arvore existente", lista, &prim);
    liberar_resultado_agm(&prim);
    liberar_resultado_agm(&kruskal);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);

    lista = criar_lista(4);
    matriz = criar_matriz_ponderada(4);
    if (lista == NULL || matriz == NULL) goto cleanup;
    for (size_t origem = 0; origem < 4; origem++) {
        for (size_t destino = origem + 1; destino < 4; destino++) {
            inserir_aresta_teste(lista, matriz, origem, destino, 1);
        }
    }
    verificar_status("Empates Prim", AGM_SUCESSO, agm_prim_lista(lista, &prim));
    verificar_status("Empates Kruskal", AGM_SUCESSO,
                     agm_kruskal_matriz(matriz, &kruskal));
    verificar("Multiplas solucoes otimas", 1,
              custo_proximo(prim.custo_total, kruskal.custo_total));
    verificar("Empates mantem custo otimo", 1,
              custo_proximo(3, prim.custo_total));
    verificar_floresta("Empates", lista, &prim);

cleanup:
    liberar_resultado_agm(&prim);
    liberar_resultado_agm(&kruskal);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

static void testar_pesos_e_erros(void)
{
    GrafoLista *lista = criar_lista(2);
    MatrizAdjacencia *matriz = criar_matriz_ponderada(2);
    ResultadoAGM resultado = {0};
    MatrizAdjacencia *digrafo = NULL;
    size_t bytes = 0;

    puts("\nPESO ZERO, AUSENTE E ENTRADAS INVALIDAS");
    verificar("Criar grafo", 1, lista != NULL && matriz != NULL);
    if (lista == NULL || matriz == NULL) goto cleanup;
    inserir_aresta_teste(lista, matriz, 0, 1, 0);
    verificar_status("Zero e peso valido", AGM_SUCESSO,
                     agm_prim_lista(lista, &resultado));
    verificar("Zero continua zero", 1, custo_proximo(0, resultado.custo_total));
    verificar("Origem do zero preservada",
              ORIGEM_PESO_COMPRIMENTO_MEDIDO,
              resultado.arestas[0].peso.origem);
    {
        ArestaAGM *saida_existente = resultado.arestas;
        verificar_status("Rejeitar saida ainda ocupada", AGM_ARGUMENTO_INVALIDO,
                         agm_kruskal_lista(lista, &resultado));
        verificar("Erro preserva resultado anterior", 1,
                  resultado.arestas == saida_existente);
    }
    liberar_resultado_agm(&resultado);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
    lista = criar_lista(2);
    matriz = criar_matriz_ponderada(2);
    verificar("Criar aresta sem peso", 1,
              lista != NULL && matriz != NULL &&
              inserir_aresta_lista(lista, 0, 1) == LISTA_SUCESSO &&
              inserir_aresta(matriz, 0, 1) == MATRIZ_SUCESSO);
    if (lista == NULL || matriz == NULL) goto cleanup;
    resultado.quantidade_arestas = 7;
    resultado.custo_total = 11.5;
    resultado.quantidade_componentes = 3;
    resultado.arvore_completa = 1;
    verificar_status("Lista rejeita peso ausente", AGM_PESO_AUSENTE,
                     agm_kruskal_lista(lista, &resultado));
    verificar_status("Matriz rejeita peso ausente", AGM_PESO_AUSENTE,
                     agm_prim_matriz(matriz, &resultado));
    verificar("Erro preserva saida", 1,
              resultado.arestas == NULL &&
              resultado.quantidade_arestas == 7 &&
              resultado.custo_total == 11.5 &&
              resultado.quantidade_componentes == 3 &&
              resultado.arvore_completa == 1);
    verificar_status("Lista nula", AGM_ARGUMENTO_INVALIDO,
                     agm_prim_lista(NULL, &resultado));
    verificar_status("Saida nula", AGM_ARGUMENTO_INVALIDO,
                     agm_prim_matriz(matriz, NULL));

    verificar("Estimar digrafo", MATRIZ_SUCESSO,
              estimar_memoria_matriz_ponderada(2, &bytes));
    verificar("Criar digrafo", MATRIZ_SUCESSO,
              criar_matriz_direcionada(2, bytes, &digrafo));
    if (digrafo != NULL) {
        inserir_aresta_ponderada(digrafo, 0, 1, 1,
                                 ORIGEM_PESO_CUSTO_ASSUMIDO);
        verificar_status("Rejeitar matriz direcionada", AGM_GRAFO_DIRECIONADO,
                         agm_prim_matriz(digrafo, &resultado));
    }
    destruir_matriz(&digrafo);

cleanup:
    liberar_resultado_agm(&resultado);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

static void testar_overflow(void)
{
    GrafoLista *lista = criar_lista(3);
    MatrizAdjacencia *matriz = criar_matriz_ponderada(3);
    ResultadoAGM resultado = {0};
    puts("\nOVERFLOW DE CUSTO");
    verificar("Criar grafo extremo", 1, lista != NULL && matriz != NULL);
    if (lista == NULL || matriz == NULL) goto cleanup;
    inserir_ponderada(lista, matriz, 0, 1, DBL_MAX,
                      ORIGEM_PESO_CUSTO_ASSUMIDO);
    inserir_ponderada(lista, matriz, 1, 2, DBL_MAX,
                      ORIGEM_PESO_CUSTO_ASSUMIDO);
    verificar_status("Prim detecta overflow", AGM_OVERFLOW_CUSTO,
                     agm_prim_lista(lista, &resultado));
    verificar("Prim nao publica resultado parcial", 1,
              resultado.arestas == NULL);
    verificar_status("Kruskal detecta overflow", AGM_OVERFLOW_CUSTO,
                     agm_kruskal_matriz(matriz, &resultado));
    verificar("Kruskal nao publica resultado parcial", 1,
              resultado.arestas == NULL);

cleanup:
    liberar_resultado_agm(&resultado);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

static void testar_grafo_vazio(void)
{
    GrafoLista *lista = criar_lista(0);
    MatrizAdjacencia *matriz = criar_matriz_ponderada(0);
    ResultadoAGM resultado = {0};
    puts("\nGRAFO VAZIO");
    verificar_status("Prim vazio", AGM_SUCESSO, agm_prim_lista(lista, &resultado));
    verificar("Vazio sem componentes", 0,
              (int)resultado.quantidade_componentes);
    verificar("Vazio nao e arvore", 0, resultado.arvore_completa);
    liberar_resultado_agm(&resultado);
    verificar_status("Kruskal matriz vazia", AGM_SUCESSO,
                     agm_kruskal_matriz(matriz, &resultado));
    verificar("Matriz vazia sem arestas", 0,
              (int)resultado.quantidade_arestas);
    liberar_resultado_agm(&resultado);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);

    lista = criar_lista(1);
    matriz = criar_matriz_ponderada(1);
    verificar("Criar grafo unitario", 1, lista != NULL && matriz != NULL);
    if (lista != NULL && matriz != NULL) {
        verificar_status("Prim unitario", AGM_SUCESSO,
                         agm_prim_lista(lista, &resultado));
        verificar("Unitario tem uma componente", 1,
                  (int)resultado.quantidade_componentes);
        verificar("Unitario e arvore completa", 1,
                  resultado.arvore_completa);
        verificar("Unitario nao tem arestas", 0,
                  (int)resultado.quantidade_arestas);
    }
    liberar_resultado_agm(&resultado);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

int main(void)
{
    testar_grafo_com_ciclos();
    testar_floresta_desconectada();
    testar_arvore_e_empates();
    testar_pesos_e_erros();
    testar_overflow();
    testar_grafo_vazio();
    printf("\n%zu verificacoes, %zu falhas.\n", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
