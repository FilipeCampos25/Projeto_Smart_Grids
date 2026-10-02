#include "dfs.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static size_t verificacoes;
static size_t falhas;

/* Compara mesmo com NDEBUG; divergencias sempre aparecem e afetam o retorno. */
static void conferir_numero(const char *descricao, size_t esperado,
                            size_t obtido, int mostrar)
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

#ifdef DFS_TESTAR_ALOCACAO
#include "alocacao_teste.h"

static size_t tentativas_alocacao;
static size_t falhar_na_tentativa = SIZE_MAX;
static size_t blocos_vivos;

/* Substitui malloc somente em dfs.c; o teste e os grafos usam a libc real.
 * O bloco pertence ao modulo, que deve entrega-lo a teste_free.
 */
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

/* Conta liberacoes de blocos da DFS; aceita NULL. ASan complementa o contador. */
void teste_free(void *ponteiro)
{
    if (ponteiro != NULL) {
        conferir_numero("free com bloco vivo", 1, blocos_vivos > 0, 0);
        blocos_vivos--;
    }
    free(ponteiro);
}
#endif

/* Duas representacoes reais do mesmo caso de teste, sem implementar grafos. */
typedef struct {
    GrafoLista *lista;
    MatrizAdjacencia *matriz;
} GrafosTeste;

/* Libera os dois grafos locais, inclusive em construcao parcial. */
static void liberar_grafos(GrafosTeste *grafos)
{
    liberar_grafo_lista(grafos->lista);
    grafos->lista = NULL;
    destruir_matriz(&grafos->matriz);
}

/* Constroi N vertices nas APIs publicadas, com teto de 2 MiB para a matriz.
 * Retorna 1 em sucesso (o chamador libera ambos), 0 apos liberar em falha.
 */
static int criar_grafos(size_t quantidade_vertices, GrafosTeste *grafos)
{
    *grafos = (GrafosTeste){0};
    grafos->lista = criar_grafo_lista();
    conferir_numero("criar lista", 1, grafos->lista != NULL, 0);
    ResultadoMatriz resultado = criar_matriz(quantidade_vertices, 2 * 1024 * 1024,
                                             &grafos->matriz);
    conferir_numero("criar matriz", MATRIZ_SUCESSO, resultado, 0);
    if (grafos->lista == NULL || resultado != MATRIZ_SUCESSO) {
        liberar_grafos(grafos);
        return 0;
    }
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        size_t indice = SIZE_MAX;
        ResultadoLista insercao = inserir_vertice_lista(grafos->lista, &indice);
        conferir_numero("inserir vertice", LISTA_SUCESSO, insercao, 0);
        if (insercao != LISTA_SUCESSO) {
            liberar_grafos(grafos);
            return 0;
        }
        conferir_numero("indice consecutivo", vertice, indice, 0);
    }
    return 1;
}

/* Altera uma conexao nas duas APIs existentes e registra qualquer erro. */
static void definir_aresta(GrafosTeste *grafos, size_t origem, size_t destino,
                           int presente)
{
    ResultadoLista lista = presente
        ? inserir_aresta_lista(grafos->lista, origem, destino)
        : remover_aresta_lista(grafos->lista, origem, destino);
    ResultadoMatriz matriz = presente
        ? inserir_aresta(grafos->matriz, origem, destino)
        : remover_aresta(grafos->matriz, origem, destino);
    conferir_numero("alterar aresta lista", LISTA_SUCESSO, lista, 0);
    conferir_numero("alterar aresta matriz", MATRIZ_SUCESSO, matriz, 0);
}

/* Imprime toda ordem pequena e os extremos de ordens longas. A comparacao
 * em conferir_percurso percorre todos os elementos, inclusive os omitidos.
 */
static void imprimir_ordem(const size_t *ordem, size_t quantidade)
{
    putchar('[');
    for (size_t posicao = 0; posicao < quantidade; posicao++) {
        if (quantidade > 12 && posicao == 5) {
            printf(",...,");
            posicao = quantidade - 5;
        } else if (posicao != 0) {
            putchar(',');
        }
        printf("%zu", ordem[posicao]);
    }
    putchar(']');
}

/* Verifica ordem, contagens, ausencia de repeticoes e TODO o vetor visitados,
 * incluindo os vertices inalcancaveis. Vetores auxiliares pertencem ao teste.
 */
static void conferir_percurso(const char *descricao, const PercursoDFS *percurso,
                              size_t quantidade_vertices, const size_t *esperada,
                              size_t quantidade_esperada)
{
    unsigned char *vistos = calloc(quantidade_vertices, sizeof(*vistos));
    unsigned char *esperados = calloc(quantidade_vertices, sizeof(*esperados));
    size_t divergencias = 0;
    size_t repeticoes = 0;

    printf("\n%s\n", descricao);
    conferir_numero("resultado publicado", 1, percurso != NULL, 1);
    conferir_numero("memoria do verificador", 1,
                    vistos != NULL && esperados != NULL, 0);
    if (percurso == NULL || vistos == NULL || esperados == NULL) {
        free(vistos);
        free(esperados);
        return;
    }
    conferir_numero("vertices do grafo", quantidade_vertices,
                    percurso->quantidade_vertices, 1);
    conferir_numero("alcancados", quantidade_esperada,
                    percurso->quantidade_alcancados, 1);
    conferir_numero("vetores publicados", 1,
                    percurso->ordem != NULL && percurso->visitados != NULL, 1);
    if (percurso->quantidade_vertices != quantidade_vertices ||
        percurso->quantidade_alcancados > quantidade_vertices ||
        percurso->ordem == NULL || percurso->visitados == NULL) {
        conferir_numero("estrutura valida para comparacao", 1, 0, 1);
        free(vistos);
        free(esperados);
        return;
    }
    for (size_t posicao = 0; posicao < quantidade_esperada; posicao++) {
        esperados[esperada[posicao]] = 1;
    }
    for (size_t posicao = 0; posicao < percurso->quantidade_alcancados; posicao++) {
        size_t vertice = percurso->ordem[posicao];
        if (posicao >= quantidade_esperada || vertice != esperada[posicao]) {
            divergencias++;
        }
        if (vertice >= quantidade_vertices) {
            divergencias++;
        } else {
            repeticoes += vistos[vertice] != 0;
            vistos[vertice] = 1;
        }
    }
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        if (percurso->visitados[vertice] != esperados[vertice] ||
            vistos[vertice] != esperados[vertice]) {
            divergencias++;
        }
    }
    printf("ordem: esperado=");
    imprimir_ordem(esperada, quantidade_esperada);
    printf(" obtido=");
    imprimir_ordem(percurso->ordem, percurso->quantidade_alcancados);
    printf(" %s\n", divergencias == 0 &&
           percurso->quantidade_alcancados == quantidade_esperada ? "PASSOU" : "FALHOU");
    conferir_numero("divergencias na ordem e nos conjuntos", 0, divergencias, 1);
    conferir_numero("repeticoes", 0, repeticoes, 1);
    free(vistos);
    free(esperados);
}

/* Executa as duas DFS e compara conjuntos sem exigir ordens iguais.
 * Cada resultado e liberado antes do retorno, inclusive em falha.
 */
static void conferir_buscas(const GrafosTeste *grafos, size_t origem,
                            const size_t *ordem_lista, const size_t *ordem_matriz,
                            size_t quantidade)
{
    PercursoDFS *lista = NULL;
    PercursoDFS *matriz = NULL;
    size_t vertices = quantidade_vertices_lista(grafos->lista);
    conferir_numero("status DFS lista", DFS_SUCESSO,
                    executar_dfs_lista(grafos->lista, origem, &lista), 1);
    conferir_numero("status DFS matriz", DFS_SUCESSO,
                    executar_dfs_matriz(grafos->matriz, origem, &matriz), 1);
    conferir_percurso("LISTA", lista, vertices, ordem_lista, quantidade);
    conferir_percurso("MATRIZ", matriz, vertices, ordem_matriz, quantidade);
    if (lista != NULL && matriz != NULL &&
        lista->quantidade_vertices == vertices &&
        matriz->quantidade_vertices == vertices &&
        lista->visitados != NULL && matriz->visitados != NULL) {
        size_t diferencas = 0;
        for (size_t vertice = 0; vertice < vertices; vertice++) {
            diferencas += lista->visitados[vertice] != matriz->visitados[vertice];
        }
        conferir_numero("diferencas entre conjuntos lista/matriz", 0, diferencas, 1);
    }
    liberar_percurso_dfs(&lista);
    liberar_percurso_dfs(&matriz);
    conferir_numero("donos zerados apos liberar", 1, lista == NULL && matriz == NULL, 0);
    liberar_percurso_dfs(&lista);
    liberar_percurso_dfs(NULL);
}

/* Confere todas as 36 adjacencias contra o oraculo independente do caso
 * pequeno. Detecta alteracoes de topologia, inclusive em nao alcancados.
 */
static void conferir_topologia(const GrafosTeste *grafos,
                               const int esperada[6][6], size_t arestas)
{
    size_t divergencias = 0;
    for (size_t origem = 0; origem < 6; origem++) {
        for (size_t destino = 0; destino < 6; destino++) {
            int lista = -1;
            int matriz = -1;
            conferir_numero("consulta lista", LISTA_SUCESSO,
                            existe_aresta_lista(grafos->lista, origem, destino, &lista), 0);
            conferir_numero("consulta matriz", MATRIZ_SUCESSO,
                            consultar_adjacencia(grafos->matriz, origem, destino, &matriz), 0);
            divergencias += lista != esperada[origem][destino];
            divergencias += matriz != esperada[origem][destino];
        }
    }
    conferir_numero("topologia preservada: divergencias", 0, divergencias, 1);
    conferir_numero("arestas preservadas", arestas,
                    quantidade_arestas_lista(grafos->lista), 1);
}

/* Caso da issue: ordens derivadas antes da execucao. A lista insere na
 * cabeca (0: 2,1; 1: 4,3,0), a matriz examina indices crescentes.
 */
static void testar_pequeno(void)
{
    GrafosTeste grafos;
    const size_t arestas[][2] = {{0, 1}, {0, 2}, {1, 3}, {1, 4}};
    const int inicial[6][6] = {
        {0,1,1,0,0,0}, {1,0,0,1,1,0}, {1,0,0,0,0,0},
        {0,1,0,0,0,0}, {0,1,0,0,0,0}, {0,0,0,0,0,0}
    };
    const int ciclo[6][6] = {
        {0,1,1,1,0,0}, {1,0,0,1,1,0}, {1,0,0,0,0,0},
        {1,1,0,0,0,0}, {0,1,0,0,0,0}, {0,0,0,0,0,0}
    };
    const int removida[6][6] = {
        {0,1,1,1,0,0}, {1,0,0,1,0,0}, {1,0,0,0,0,0},
        {1,1,0,0,0,0}, {0,0,0,0,0,0}, {0,0,0,0,0,0}
    };
    if (!criar_grafos(6, &grafos)) {
        return;
    }
    for (size_t indice = 0; indice < 4; indice++) {
        definir_aresta(&grafos, arestas[indice][0], arestas[indice][1], 1);
    }
    puts("PEQUENO: origem 0, quatro arestas, vertice 5 isolado");
    conferir_topologia(&grafos, inicial, 4);
    conferir_buscas(&grafos, 0, (size_t[]){0,2,1,4,3}, (size_t[]){0,1,3,4,2}, 5);
    conferir_topologia(&grafos, inicial, 4);
    puts("PEQUENO: origem isolada 5");
    conferir_buscas(&grafos, 5, (size_t[]){5}, (size_t[]){5}, 1);
    conferir_topologia(&grafos, inicial, 4);
    puts("PEQUENO: ciclo com (3,0)");
    definir_aresta(&grafos, 3, 0, 1);
    conferir_buscas(&grafos, 0, (size_t[]){0,3,1,4,2}, (size_t[]){0,1,3,4,2}, 5);
    conferir_topologia(&grafos, ciclo, 5);
    puts("PEQUENO: desativar (1,4), isolando tambem o vertice 4");
    definir_aresta(&grafos, 1, 4, 0);
    conferir_buscas(&grafos, 0, (size_t[]){0,3,1,2}, (size_t[]){0,1,3,2}, 4);
    conferir_buscas(&grafos, 4, (size_t[]){4}, (size_t[]){4}, 1);
    conferir_topologia(&grafos, removida, 4);
    puts("PEQUENO: restaurar (1,4)");
    definir_aresta(&grafos, 1, 4, 1);
    conferir_buscas(&grafos, 0, (size_t[]){0,3,1,4,2}, (size_t[]){0,1,3,4,2}, 5);
    conferir_topologia(&grafos, ciclo, 5);
    liberar_grafos(&grafos);
}

/* Cadeia profunda nos dois sentidos e corte em duas componentes de 500.
 * Os 1.000 indices da ordem sao comparados; nao e um dataset real.
 */
static void testar_escala(void)
{
    GrafosTeste grafos;
    size_t crescente[1000];
    size_t decrescente[1000];
    if (!criar_grafos(1000, &grafos)) {
        return;
    }
    for (size_t vertice = 0; vertice < 1000; vertice++) {
        crescente[vertice] = vertice;
        decrescente[vertice] = 999 - vertice;
        if (vertice != 0) {
            definir_aresta(&grafos, vertice - 1, vertice, 1);
        }
    }
    puts("ESCALA SINTETICA: cadeia de 1.000 vertices / 999 arestas");
    conferir_numero("arestas cadeia", 999, quantidade_arestas_lista(grafos.lista), 1);
    conferir_buscas(&grafos, 0, crescente, crescente, 1000);
    conferir_buscas(&grafos, 999, decrescente, decrescente, 1000);
    puts("ESCALA SINTETICA: corte (499,500)");
    definir_aresta(&grafos, 499, 500, 0);
    conferir_buscas(&grafos, 0, crescente, crescente, 500);
    conferir_buscas(&grafos, 999, decrescente, decrescente, 500);
    definir_aresta(&grafos, 499, 500, 1);
    conferir_buscas(&grafos, 0, crescente, crescente, 1000);
    liberar_grafos(&grafos);
}

/* Uma aresta entre ramos deve levar ao ramo vizinho antes de retornar ao pai.
 * Detecta o erro de marcar todos os vizinhos do pai antes de explorar o filho.
 */
static void testar_ramos_conectados(void)
{
    GrafosTeste grafos;
    const size_t arestas[][2] = {{0,1}, {0,2}, {1,3}, {2,4}, {1,2}};
    if (!criar_grafos(5, &grafos)) {
        return;
    }
    for (size_t indice = 0; indice < 5; indice++) {
        definir_aresta(&grafos, arestas[indice][0], arestas[indice][1], 1);
    }
    puts("RAMOS CONECTADOS: descoberta deve acontecer ao descer");
    conferir_buscas(&grafos, 0, (size_t[]){0,2,1,3,4}, (size_t[]){0,1,2,4,3}, 5);
    liberar_grafos(&grafos);
}

/* Entradas invalidas preservam saidas; laco da matriz e aceito sem repeticao.
 * Resultados continuam legiveis apos liberar os grafos que os originaram.
 */
static void testar_limites(void)
{
    GrafosTeste grafos;
    PercursoDFS *lista = NULL;
    PercursoDFS *matriz = NULL;
    if (!criar_grafos(0, &grafos)) {
        return;
    }
    puts("LIMITES: NULL, vazio, origem invalida, dono ocupado e laco");
    conferir_numero("lista vazia", DFS_ARGUMENTO_INVALIDO,
                    executar_dfs_lista(grafos.lista, 0, &lista), 1);
    conferir_numero("matriz vazia", DFS_ARGUMENTO_INVALIDO,
                    executar_dfs_matriz(grafos.matriz, 0, &matriz), 1);
    conferir_numero("lista NULL", DFS_ARGUMENTO_INVALIDO,
                    executar_dfs_lista(NULL, 0, &lista), 1);
    conferir_numero("matriz NULL", DFS_ARGUMENTO_INVALIDO,
                    executar_dfs_matriz(NULL, 0, &matriz), 1);
    conferir_numero("saidas preservadas", 1, lista == NULL && matriz == NULL, 1);
    liberar_grafos(&grafos);
    if (!criar_grafos(1, &grafos)) {
        return;
    }
    const size_t invalidos[] = {1, SIZE_MAX};
    for (size_t indice = 0; indice < 2; indice++) {
        conferir_numero("origem invalida lista", DFS_ARGUMENTO_INVALIDO,
                        executar_dfs_lista(grafos.lista, invalidos[indice], &lista), 1);
        conferir_numero("origem invalida matriz", DFS_ARGUMENTO_INVALIDO,
                        executar_dfs_matriz(grafos.matriz, invalidos[indice], &matriz), 1);
        conferir_numero("saidas preservadas", 1, lista == NULL && matriz == NULL, 1);
    }
    conferir_numero("saida NULL lista", DFS_ARGUMENTO_INVALIDO,
                    executar_dfs_lista(grafos.lista, 0, NULL), 1);
    conferir_numero("saida NULL matriz", DFS_ARGUMENTO_INVALIDO,
                    executar_dfs_matriz(grafos.matriz, 0, NULL), 1);
    conferir_numero("laco matriz", MATRIZ_SUCESSO, inserir_aresta(grafos.matriz, 0, 0), 0);
    conferir_numero("laco duplicado", MATRIZ_SUCESSO, inserir_aresta(grafos.matriz, 0, 0), 0);
    conferir_numero("DFS unitaria lista", DFS_SUCESSO,
                    executar_dfs_lista(grafos.lista, 0, &lista), 1);
    conferir_numero("DFS unitaria matriz com laco", DFS_SUCESSO,
                    executar_dfs_matriz(grafos.matriz, 0, &matriz), 1);
    PercursoDFS *dono_lista = lista;
    PercursoDFS *dono_matriz = matriz;
    if (lista != NULL && matriz != NULL) {
        conferir_numero("dono ocupado lista", DFS_ARGUMENTO_INVALIDO,
                        executar_dfs_lista(grafos.lista, 0, &lista), 1);
        conferir_numero("dono ocupado matriz", DFS_ARGUMENTO_INVALIDO,
                        executar_dfs_matriz(grafos.matriz, 0, &matriz), 1);
        conferir_numero("donos preservados", 1,
                        lista == dono_lista && matriz == dono_matriz, 1);
    }
    liberar_grafos(&grafos);
    conferir_percurso("LISTA: resultado independente do grafo", lista, 1, (size_t[]){0}, 1);
    conferir_percurso("MATRIZ: resultado independente do grafo", matriz, 1, (size_t[]){0}, 1);
    liberar_percurso_dfs(&lista);
    liberar_percurso_dfs(&matriz);
}

#ifdef DFS_TESTAR_ALOCACAO
/* Falha cada alocacao real da DFS e tenta novamente. Verifica saida NULL,
 * blocos zerados, sucesso posterior e grafo ainda integro em ambas as APIs.
 */
static void testar_falhas_alocacao(void)
{
    GrafosTeste grafos;
    if (!criar_grafos(6, &grafos)) {
        return;
    }
    definir_aresta(&grafos, 0, 1, 1);
    definir_aresta(&grafos, 0, 2, 1);
    definir_aresta(&grafos, 1, 3, 1);
    definir_aresta(&grafos, 1, 4, 1);
    const int topologia[6][6] = {
        {0,1,1,0,0,0}, {1,0,0,1,1,0}, {1,0,0,0,0,0},
        {0,1,0,0,0,0}, {0,1,0,0,0,0}, {0,0,0,0,0,0}
    };
    for (int usar_matriz = 0; usar_matriz <= 1; usar_matriz++) {
        PercursoDFS *percurso = NULL;
        tentativas_alocacao = 0;
        ResultadoDFS resultado = usar_matriz
            ? executar_dfs_matriz(grafos.matriz, 0, &percurso)
            : executar_dfs_lista(grafos.lista, 0, &percurso);
        size_t total_alocacoes = tentativas_alocacao;
        conferir_numero("referencia sem falhas", DFS_SUCESSO, resultado, 1);
        liberar_percurso_dfs(&percurso);
        conferir_numero("blocos apos referencia", 0, blocos_vivos, 1);
        conferir_numero("houve alocacoes para instrumentar", 1, total_alocacoes > 0, 1);
        for (size_t tentativa = 0; tentativa < total_alocacoes; tentativa++) {
            printf("ALOCACAO %s: falhar tentativa %zu\n",
                   usar_matriz ? "MATRIZ" : "LISTA", tentativa + 1);
            tentativas_alocacao = 0;
            falhar_na_tentativa = tentativa;
            resultado = usar_matriz
                ? executar_dfs_matriz(grafos.matriz, 0, &percurso)
                : executar_dfs_lista(grafos.lista, 0, &percurso);
            conferir_numero("falha propagada", DFS_SEM_MEMORIA, resultado, 1);
            conferir_numero("saida preservada NULL", 1, percurso == NULL, 1);
            conferir_numero("blocos apos falha", 0, blocos_vivos, 1);
            liberar_percurso_dfs(&percurso);
            falhar_na_tentativa = SIZE_MAX;
            conferir_buscas(&grafos, 0, (size_t[]){0,2,1,4,3}, (size_t[]){0,1,3,4,2}, 5);
            conferir_topologia(&grafos, topologia, 4);
            conferir_numero("blocos apos recuperacao", 0, blocos_vivos, 1);
        }
        printf("ALOCACAO %s: %zu pontos exercitados\n",
               usar_matriz ? "MATRIZ" : "LISTA", total_alocacoes);
    }
    liberar_grafos(&grafos);
}
#endif

/* Todos os casos sao sinteticos. Qualquer comparacao falha retorna nao zero. */
int main(void)
{
    testar_pequeno();
    testar_ramos_conectados();
    testar_escala();
    testar_limites();
#ifdef DFS_TESTAR_ALOCACAO
    conferir_numero("blocos apos testes funcionais", 0, blocos_vivos, 1);
    testar_falhas_alocacao();
    conferir_numero("blocos vivos finais DFS", 0, blocos_vivos, 1);
#endif
    puts("DATASET REAL: NAO EXECUTADO / PENDENTE (sem arquivos e carregador #4)");
    printf("RESULTADO DFS: %s (%zu verificacoes, %zu falhas)\n",
           falhas == 0 ? "PASSOU" : "FALHOU", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
