#include "bfs.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef BFS_TESTAR_ALOCACAO
#include "alocacao_teste.h"

static size_t blocos_vivos;
static size_t tentativas_alocacao;
static size_t falhar_na_tentativa = SIZE_MAX;

/* Somente o objeto BFS usa este alocador. O teste e os grafos usam a libc.
 * O bloco retornado pertence a BFS, que deve devolve-lo a teste_free. */
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

/* Contabiliza a liberacao dos blocos da BFS; aceita NULL. */
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
static const size_t arestas_exemplo[][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {3, 4}};

/* Compara valores sem assert, acumula erros e mostra esperado/obtido.
 * mostrar=0 resume montagens extensas; divergencias sempre sao exibidas. */
static void conferir(const char *descricao, size_t esperado, size_t obtido, int mostrar)
{
    verificacoes++;
    if (esperado != obtido) {
        falhas++;
    }
    if (mostrar || esperado != obtido) {
        printf("%s: esperado=%zu obtido=%zu %s\n", descricao, esperado, obtido,
               esperado == obtido ? "PASSOU" : "FALHOU");
    }
}

/* Cria as duas representacoes reais com indices identicos, sem conexoes.
 * Em erro libera ambas. Em sucesso, o teste chamador e dono dos grafos. */
static int criar_grafos(size_t quantidade, GrafoLista **lista, MatrizAdjacencia **matriz)
{
    *lista = criar_grafo_lista();
    conferir("Criar lista", 1, *lista != NULL, 0);
    conferir("Criar matriz", MATRIZ_SUCESSO,
             criar_matriz(quantidade, 2 * 1024 * 1024, matriz), 0);
    if (*lista == NULL || *matriz == NULL) {
        liberar_grafo_lista(*lista);
        *lista = NULL;
        destruir_matriz(matriz);
        return 0;
    }
    for (size_t vertice = 0; vertice < quantidade; vertice++) {
        size_t indice = SIZE_MAX;
        ResultadoLista status = inserir_vertice_lista(*lista, &indice);
        conferir("Inserir vertice", LISTA_SUCESSO, status, 0);
        if (status != LISTA_SUCESSO) {
            liberar_grafo_lista(*lista);
            *lista = NULL;
            destruir_matriz(matriz);
            return 0;
        }
        conferir("Indice interno", vertice, indice, 0);
    }
    return 1;
}

/* Insere uma conexao em cada API e verifica os retornos, sem alterar politicas. */
static void inserir_nas_duas(GrafoLista *lista, MatrizAdjacencia *matriz,
                            size_t origem, size_t destino)
{
    conferir("Inserir na lista", LISTA_SUCESSO,
             inserir_aresta_lista(lista, origem, destino), 0);
    conferir("Inserir na matriz", MATRIZ_SUCESSO,
             inserir_aresta(matriz, origem, destino), 0);
}

/* Mostra ordem efetiva e distancias esperado/obtido nos exemplos pequenos.
 * A ordem so e valida se as comparacoes de conjunto/camadas tambem passarem. */
static void imprimir_resultado(const ResultadoBfs *resultado, const size_t *esperadas)
{
    printf("Ordem obtida=[");
    for (size_t posicao = 0; posicao < resultado->quantidade_alcancados; posicao++) {
        printf("%s%zu", posicao == 0 ? "" : ",", resultado->ordem_visita[posicao]);
    }
    puts("]");
    for (size_t vertice = 0; vertice < resultado->quantidade_vertices; vertice++) {
        printf("Distancia de %zu: esperado=", vertice);
        if (esperadas[vertice] == SIZE_MAX) {
            printf("inalcancavel");
        } else {
            printf("%zu", esperadas[vertice]);
        }
        printf(" obtido=");
        if (resultado->distancias[vertice] == SIZE_MAX) {
            puts("inalcancavel");
        } else {
            printf("%zu\n", resultado->distancias[vertice]);
        }
    }
}

/* Usa distancias conhecidas como oraculo INDEPENDENTE da implementacao.
 * Confere conjunto exato, contagem, unicidade e camadas nao decrescentes.
 * Um DFS que alcancasse o mesmo conjunto falharia na verificacao das camadas.
 * O vetor de ocorrencias pertence ao teste e e liberado antes do retorno. */
static void conferir_resultado(const ResultadoBfs *resultado, size_t vertices,
                               const size_t *distancias_esperadas)
{
    size_t quantidade_esperada = 0;
    size_t divergencias = 0;
    size_t repeticoes = 0;
    size_t invalidos = 0;
    size_t quebras_camadas = 0;
    size_t distancia_anterior = 0;
    unsigned char *ocorrencias;

    conferir("Resultado disponivel", 1, resultado != NULL, 1);
    if (resultado == NULL) {
        return;
    }
    conferir("Vertices do resultado", vertices, resultado->quantidade_vertices, 1);
    conferir("Vetores disponiveis", 1,
             resultado->ordem_visita != NULL && resultado->distancias != NULL, 0);
    conferir("Quantidade cabe no grafo", 1, resultado->quantidade_alcancados <= vertices, 0);
    if (resultado->quantidade_vertices != vertices || resultado->ordem_visita == NULL ||
        resultado->distancias == NULL || resultado->quantidade_alcancados > vertices) {
        return;
    }
    ocorrencias = calloc(vertices, sizeof(*ocorrencias));
    conferir("Memoria do verificador", 1, ocorrencias != NULL, 0);
    if (ocorrencias == NULL) {
        return;
    }
    for (size_t vertice = 0; vertice < vertices; vertice++) {
        quantidade_esperada += distancias_esperadas[vertice] != SIZE_MAX;
        divergencias += resultado->distancias[vertice] != distancias_esperadas[vertice];
    }
    for (size_t posicao = 0; posicao < resultado->quantidade_alcancados; posicao++) {
        size_t vertice = resultado->ordem_visita[posicao];
        if (vertice >= vertices || distancias_esperadas[vertice] == SIZE_MAX) {
            invalidos++;
            continue;
        }
        repeticoes += ocorrencias[vertice] != 0;
        ocorrencias[vertice] = 1;
        quebras_camadas += distancias_esperadas[vertice] < distancia_anterior;
        distancia_anterior = distancias_esperadas[vertice];
    }
    for (size_t vertice = 0; vertice < vertices; vertice++) {
        invalidos += (ocorrencias[vertice] != 0) != (distancias_esperadas[vertice] != SIZE_MAX);
    }
    if (vertices <= 6) {
        imprimir_resultado(resultado, distancias_esperadas);
    }
    conferir("Alcancados", quantidade_esperada, resultado->quantidade_alcancados, 1);
    conferir("Divergencias de distancias", 0, divergencias, 1);
    conferir("Repeticoes", 0, repeticoes, 1);
    conferir("Divergencias do conjunto", 0, invalidos, 1);
    conferir("Quebras da ordem por camadas", 0, quebras_camadas, 1);
    free(ocorrencias);
}

/* Compara representacoes por indice/distancia, permitindo desempate diferente.
 * Resultados sao emprestados; a funcao nao aloca nem transfere propriedade. */
static void comparar_resultados(const ResultadoBfs *lista, const ResultadoBfs *matriz)
{
    size_t divergencias = 0;
    if (lista == NULL || matriz == NULL) {
        conferir("Comparacao entre representacoes disponivel", 1, 0, 1);
        return;
    }
    conferir("Mesma quantidade alcancada", lista->quantidade_alcancados,
             matriz->quantidade_alcancados, 1);
    conferir("Mesmo universo de vertices", lista->quantidade_vertices,
             matriz->quantidade_vertices, 0);
    if (lista->quantidade_vertices != matriz->quantidade_vertices) {
        return;
    }
    for (size_t vertice = 0; vertice < lista->quantidade_vertices; vertice++) {
        divergencias += lista->distancias[vertice] != matriz->distancias[vertice];
    }
    conferir("Lista x matriz: divergencias de alcance/camada", 0, divergencias, 1);
}

/* Executa e valida duas buscas novas e libera os resultados, inclusive parciais. */
static void conferir_buscas(const GrafoLista *lista, const MatrizAdjacencia *matriz,
                            size_t origem, size_t vertices, const size_t *esperadas)
{
    ResultadoBfs *resultado_lista = NULL;
    ResultadoBfs *resultado_matriz = NULL;
    puts("LISTA");
    conferir("Executar BFS lista", BFS_SUCESSO,
             executar_bfs_lista(lista, origem, &resultado_lista), 1);
    conferir_resultado(resultado_lista, vertices, esperadas);
    puts("MATRIZ");
    conferir("Executar BFS matriz", BFS_SUCESSO,
             executar_bfs_matriz(matriz, origem, &resultado_matriz), 1);
    conferir_resultado(resultado_matriz, vertices, esperadas);
    comparar_resultados(resultado_lista, resultado_matriz);
    liberar_resultado_bfs(&resultado_lista);
    liberar_resultado_bfs(&resultado_matriz);
    conferir("Liberacao zera donos", 1, resultado_lista == NULL && resultado_matriz == NULL, 0);
}

/* Compara todos os pares com o exemplo conhecido apos a BFS, comprovando que
 * a busca preservou a topologia (incluindo ausencias e conexoes removidas). */
static void conferir_topologia(const GrafoLista *lista, const MatrizAdjacencia *matriz,
                               int conexao_presente)
{
    size_t divergencias = 0;
    for (size_t origem = 0; origem < 6; origem++) {
        for (size_t destino = 0; destino < 6; destino++) {
            int esperado = 0;
            int na_lista = -1;
            int na_matriz = -1;
            for (size_t indice = 0; indice < (conexao_presente ? 5u : 4u); indice++) {
                size_t primeiro = arestas_exemplo[indice][0];
                size_t segundo = arestas_exemplo[indice][1];
                esperado |= (origem == primeiro && destino == segundo) ||
                            (origem == segundo && destino == primeiro);
            }
            divergencias += existe_aresta_lista(lista, origem, destino, &na_lista) != LISTA_SUCESSO;
            divergencias += consultar_adjacencia(matriz, origem, destino, &na_matriz) != MATRIZ_SUCESSO;
            divergencias += na_lista != esperado || na_matriz != esperado;
        }
    }
    conferir("Topologia: divergencias nos 36 pares", 0, divergencias, 1);
    conferir("Arestas preservadas na lista", conexao_presente ? 5 : 4,
             quantidade_arestas_lista(lista), 0);
}

/* Testa o exemplo obrigatorio, isolado, remocao, restauracao e outra origem. */
static void testar_pequeno(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    const size_t inicial[] = {0, 1, 1, 2, 3, SIZE_MAX};
    const size_t isolado[] = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX, 0};
    const size_t removida[] = {0, 1, 1, 2, SIZE_MAX, SIZE_MAX};
    const size_t origem_4[] = {3, 2, 2, 1, 0, SIZE_MAX};
    if (!criar_grafos(6, &lista, &matriz)) {
        return;
    }
    for (size_t indice = 0; indice < 5; indice++) {
        inserir_nas_duas(lista, matriz, arestas_exemplo[indice][0], arestas_exemplo[indice][1]);
    }
    puts("\nPEQUENO: origem 0, esperado=[0,{1,2},3,4], quantidade=5");
    conferir_buscas(lista, matriz, 0, 6, inicial);
    conferir_topologia(lista, matriz, 1);
    puts("\nISOLADO: origem 5, esperado=[5], quantidade=1");
    conferir_buscas(lista, matriz, 5, 6, isolado);
    conferir_topologia(lista, matriz, 1);
    conferir("Remover lista (3,4)", LISTA_SUCESSO, remover_aresta_lista(lista, 3, 4), 0);
    conferir("Remover matriz (3,4)", MATRIZ_SUCESSO, remover_aresta(matriz, 3, 4), 0);
    puts("\nREMOVIDA (3,4): origem 0, esperado=[0,{1,2},3], quantidade=4");
    conferir_buscas(lista, matriz, 0, 6, removida);
    conferir_topologia(lista, matriz, 0);
    inserir_nas_duas(lista, matriz, 4, 3);
    puts("\nRESTAURADA: origem 0, esperado=[0,{1,2},3,4], quantidade=5");
    conferir_buscas(lista, matriz, 0, 6, inicial);
    puts("\nOUTRA ORIGEM: origem 4, esperado=[4,3,{1,2},0], quantidade=5");
    conferir_buscas(lista, matriz, 4, 6, origem_4);
    conferir_topologia(lista, matriz, 1);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

/* Valida cadeia profunda e estrela com fila larga, ambas com 1000 vertices.
 * Os oraculos sao formulas do grafo conhecido, sem usar BFS para gera-los. */
static void testar_escala(int estrela)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    size_t esperadas[1000];
    if (!criar_grafos(1000, &lista, &matriz)) {
        return;
    }
    esperadas[0] = 0;
    for (size_t destino = 1; destino < 1000; destino++) {
        inserir_nas_duas(lista, matriz, estrela ? 0 : destino - 1, destino);
        esperadas[destino] = estrela ? 1 : destino;
    }
    printf("\nESCALA SINTETICA: %s, esperado=1000 alcancados, 999 arestas\n",
           estrela ? "estrela (fila larga)" : "cadeia");
    conferir("Arestas sinteticas", 999, quantidade_arestas_lista(lista), 1);
    conferir_buscas(lista, matriz, 0, 1000, esperadas);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

/* Prova que nao ha reinicio automatico em outra componente com arestas. */
static void testar_componentes(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    const size_t esperadas[] = {SIZE_MAX, SIZE_MAX, SIZE_MAX, 1, 0, 1};
    if (!criar_grafos(6, &lista, &matriz)) {
        return;
    }
    inserir_nas_duas(lista, matriz, 0, 1);
    inserir_nas_duas(lista, matriz, 1, 2);
    inserir_nas_duas(lista, matriz, 3, 4);
    inserir_nas_duas(lista, matriz, 4, 5);
    puts("\nDESCONECTADO: origem 4, esperado=[4,{3,5}], quantidade=3");
    conferir_buscas(lista, matriz, 4, 6, esperadas);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

/* Preserva politicas diferentes: lista rejeita lacos/duplicatas; matriz aceita
 * laco e duplicata idempotente. Nenhum deles deve repetir a visita da origem. */
static void testar_politicas_e_vida_util(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoBfs *resultado_lista = NULL;
    ResultadoBfs *resultado_matriz = NULL;
    const size_t esperadas[] = {0};
    if (!criar_grafos(1, &lista, &matriz)) {
        return;
    }
    conferir("Lista rejeita laco", LISTA_LACO_NAO_PERMITIDO, inserir_aresta_lista(lista, 0, 0), 1);
    conferir("Matriz aceita laco", MATRIZ_SUCESSO, inserir_aresta(matriz, 0, 0), 1);
    conferir("Duplicata idempotente", MATRIZ_SUCESSO, inserir_aresta(matriz, 0, 0), 1);
    conferir("BFS unitaria lista", BFS_SUCESSO, executar_bfs_lista(lista, 0, &resultado_lista), 1);
    conferir("BFS unitaria matriz", BFS_SUCESSO, executar_bfs_matriz(matriz, 0, &resultado_matriz), 1);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
    puts("\nRESULTADOS PERMANECEM VALIDOS APOS LIBERAR OS GRAFOS");
    conferir_resultado(resultado_lista, 1, esperadas);
    conferir_resultado(resultado_matriz, 1, esperadas);
    comparar_resultados(resultado_lista, resultado_matriz);
    liberar_resultado_bfs(&resultado_lista);
    liberar_resultado_bfs(&resultado_matriz);
    liberar_resultado_bfs(&resultado_lista);
    liberar_resultado_bfs(NULL);
}

/* Testa NULL, vazio, indices fora da faixa e recusa de sobrescrever um dono. */
static void testar_entradas_invalidas(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    ResultadoBfs *resultado = NULL;
    if (!criar_grafos(0, &lista, &matriz)) {
        return;
    }
    puts("\nENTRADAS INVALIDAS: esperado=BFS_ARGUMENTO_INVALIDO (1)");
    conferir("Lista vazia", BFS_ARGUMENTO_INVALIDO, executar_bfs_lista(lista, 0, &resultado), 1);
    conferir("Matriz vazia", BFS_ARGUMENTO_INVALIDO, executar_bfs_matriz(matriz, 0, &resultado), 1);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
    if (!criar_grafos(1, &lista, &matriz)) {
        return;
    }
    const size_t invalidos[] = {1, SIZE_MAX};
    for (size_t indice = 0; indice < 2; indice++) {
        conferir("Origem invalida lista", BFS_ARGUMENTO_INVALIDO,
                 executar_bfs_lista(lista, invalidos[indice], &resultado), 1);
        conferir("Origem invalida matriz", BFS_ARGUMENTO_INVALIDO,
                 executar_bfs_matriz(matriz, invalidos[indice], &resultado), 1);
    }
    conferir("Lista NULL", BFS_ARGUMENTO_INVALIDO, executar_bfs_lista(NULL, 0, &resultado), 1);
    conferir("Matriz NULL", BFS_ARGUMENTO_INVALIDO, executar_bfs_matriz(NULL, 0, &resultado), 1);
    conferir("Saida NULL lista", BFS_ARGUMENTO_INVALIDO, executar_bfs_lista(lista, 0, NULL), 1);
    conferir("Saida NULL matriz", BFS_ARGUMENTO_INVALIDO, executar_bfs_matriz(matriz, 0, NULL), 1);
    conferir("Erros preservam saida NULL", 1, resultado == NULL, 1);
    conferir("Criar resultado valido", BFS_SUCESSO, executar_bfs_lista(lista, 0, &resultado), 0);
    ResultadoBfs *preservado = resultado;
    conferir("Saida ocupada lista", BFS_ARGUMENTO_INVALIDO,
             executar_bfs_lista(lista, 0, &resultado), 1);
    conferir("Saida ocupada matriz", BFS_ARGUMENTO_INVALIDO,
             executar_bfs_matriz(matriz, 0, &resultado), 1);
    conferir("Preserva dono existente", 1, resultado == preservado, 1);
    conferir_resultado(resultado, 1, (const size_t[]){0});
    liberar_resultado_bfs(&resultado);
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}

#ifdef BFS_TESTAR_ALOCACAO
/* Falha cada uma das tres alocacoes da BFS, em cada representacao.
 * Verifica saida, limpeza parcial, topologia e recuperacao posterior. */
static void testar_falhas_alocacao(void)
{
    GrafoLista *lista = NULL;
    MatrizAdjacencia *matriz = NULL;
    const size_t esperadas[] = {0, 1, 1, 2, 3, SIZE_MAX};
    if (!criar_grafos(6, &lista, &matriz)) {
        return;
    }
    for (size_t indice = 0; indice < 5; indice++) {
        inserir_nas_duas(lista, matriz, arestas_exemplo[indice][0], arestas_exemplo[indice][1]);
    }
    for (int usar_matriz = 0; usar_matriz <= 1; usar_matriz++) {
        for (size_t etapa = 0; etapa < 3; etapa++) {
            ResultadoBfs *resultado = NULL;
            tentativas_alocacao = 0;
            falhar_na_tentativa = etapa;
            printf("\nFALHA DE ALOCACAO: %s, etapa=%zu\n", usar_matriz ? "matriz" : "lista", etapa);
            StatusBfs status = usar_matriz ? executar_bfs_matriz(matriz, 0, &resultado)
                                          : executar_bfs_lista(lista, 0, &resultado);
            conferir("Retorno sem memoria", BFS_SEM_MEMORIA, status, 1);
            conferir("Saida preservada NULL", 1, resultado == NULL, 1);
            conferir("Alocacoes ate a falha", etapa + 1, tentativas_alocacao, 1);
            conferir("Blocos restantes apos erro", 0, blocos_vivos, 1);
            liberar_resultado_bfs(&resultado);
            conferir_topologia(lista, matriz, 1);
            falhar_na_tentativa = SIZE_MAX;
            tentativas_alocacao = 0;
            status = usar_matriz ? executar_bfs_matriz(matriz, 0, &resultado)
                                : executar_bfs_lista(lista, 0, &resultado);
            conferir("Recuperacao", BFS_SUCESSO, status, 1);
            conferir("Alocacoes em sucesso", 3, tentativas_alocacao, 1);
            conferir_resultado(resultado, 6, esperadas);
            liberar_resultado_bfs(&resultado);
            conferir("Blocos restantes apos recuperacao", 0, blocos_vivos, 1);
        }
    }
    liberar_grafo_lista(lista);
    destruir_matriz(&matriz);
}
#endif

/* Executa a suite; qualquer comparacao divergente resulta em EXIT_FAILURE. */
int main(void)
{
    testar_pequeno();
    testar_escala(0);
    testar_escala(1);
    testar_componentes();
    testar_politicas_e_vida_util();
    testar_entradas_invalidas();
#ifdef BFS_TESTAR_ALOCACAO
    conferir("Blocos restantes apos testes funcionais", 0, blocos_vivos, 1);
    testar_falhas_alocacao();
    conferir("Blocos vivos finais BFS", 0, blocos_vivos, 1);
#endif
    printf("RESULTADO BFS: %s (%zu verificacoes, %zu falhas)\n",
           falhas == 0 ? "PASSOU" : "FALHOU", verificacoes, falhas);
    puts("DATASET REAL: PENDENTE (carregador e arquivos ausentes).");
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
