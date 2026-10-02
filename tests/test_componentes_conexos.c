#include "componentes_conexos.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static size_t verificacoes;
static size_t falhas;
static const size_t arestas_exemplo[][2] = {{0, 1}, {1, 2}, {3, 4}, {4, 5}, {5, 6}};
/* Rotulos propositalmente fora de [0,C): o oraculo representa uma particao,
 * e nao exige que a implementacao escolha estes mesmos numeros. */
static const size_t particao_original[] = {42, 42, 42, 9, 9, 9, 9, 71};
static const size_t particao_falha[] = {42, 42, 88, 9, 9, 9, 9, 71};
static const size_t particao_conectada[] = {13, 13, 13, 13, 13, 13, 13, 13};

/* Compara valores mesmo com NDEBUG; falhas sempre aparecem e afetam a saida. */
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

#ifdef COMPONENTES_TESTAR_ALOCACAO
#include "alocacao_teste.h"
static size_t tentativas_alocacao;
static size_t falhar_na_tentativa = SIZE_MAX;
static size_t blocos_vivos;

/* Instrumenta somente os objetos de componentes e BFS. Grafos e oraculos usam
 * a libc. Cada bloco retornado deve ser liberado pelo modulo com teste_free. */
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

/* Contabiliza liberacao, inclusive da fila transferida da BFS; aceita NULL. */
void teste_free(void *ponteiro)
{
    if (ponteiro != NULL) {
        conferir("Liberacao com bloco vivo", 1, blocos_vivos > 0, 0);
        blocos_vivos--;
    }
    free(ponteiro);
}
#endif

/* Apenas agrupa os dois grafos reais usados no teste, sem outra representacao. */
typedef struct {
    GrafoLista *lista;
    MatrizAdjacencia *matriz;
} GrafosTeste;

/* Libera as duas representacoes, inclusive em caso de construcao parcial. */
static void liberar_grafos(GrafosTeste *grafos)
{
    liberar_grafo_lista(grafos->lista);
    grafos->lista = NULL;
    destruir_matriz(&grafos->matriz);
}

/* Cria V vertices nas APIs existentes. Em sucesso o chamador possui ambos;
 * retorna 0 apos liberar qualquer construcao parcial em falha. */
static int criar_grafos(size_t quantidade_vertices, GrafosTeste *grafos)
{
    *grafos = (GrafosTeste){0};
    grafos->lista = criar_grafo_lista();
    conferir("Criacao lista", 1, grafos->lista != NULL, 0);
    conferir("Criacao matriz", MATRIZ_SUCESSO,
             criar_matriz(quantidade_vertices, 2 * 1024 * 1024, &grafos->matriz), 0);
    if (grafos->lista == NULL || grafos->matriz == NULL) {
        liberar_grafos(grafos);
        return 0;
    }
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        size_t indice = SIZE_MAX;
        ResultadoLista status = inserir_vertice_lista(grafos->lista, &indice);
        conferir("Insercao vertice", LISTA_SUCESSO, status, 0);
        if (status != LISTA_SUCESSO) {
            liberar_grafos(grafos);
            return 0;
        }
        conferir("Indice interno", vertice, indice, 0);
    }
    return 1;
}

/* Insere ou remove o mesmo par nas duas APIs, conferindo os retornos. */
static void alterar_conexao(GrafosTeste *grafos, size_t origem, size_t destino, int inserir)
{
    conferir("Alteracao lista", LISTA_SUCESSO, inserir
        ? inserir_aresta_lista(grafos->lista, origem, destino)
        : remover_aresta_lista(grafos->lista, origem, destino), 0);
    conferir("Alteracao matriz", MATRIZ_SUCESSO, inserir
        ? inserir_aresta(grafos->matriz, origem, destino)
        : remover_aresta(grafos->matriz, origem, destino), 0);
}

/* Constroi o exemplo obrigatorio. Retorna 0 se a criacao falhar; o chamador
 * libera os grafos quando retorna 1. Cada insercao e verificada. */
static int criar_exemplo(GrafosTeste *grafos)
{
    if (!criar_grafos(8, grafos)) {
        return 0;
    }
    for (size_t aresta = 0; aresta < 5; aresta++) {
        alterar_conexao(grafos, arestas_exemplo[aresta][0], arestas_exemplo[aresta][1], 1);
    }
    return 1;
}

/* Compara a relacao "pertence a mesma componente" para TODOS os pares,
 * independentemente dos numeros dos rotulos. Retorna divergencias; nao aloca. */
static size_t divergencias_particao(size_t quantidade_vertices,
                                    const size_t *esperada, const size_t *obtida)
{
    size_t divergencias = 0;
    for (size_t primeiro = 0; primeiro < quantidade_vertices; primeiro++) {
        for (size_t segundo = primeiro + 1; segundo < quantidade_vertices; segundo++) {
            if ((esperada[primeiro] == esperada[segundo]) !=
                (obtida[primeiro] == obtida[segundo])) {
                divergencias++;
            }
        }
    }
    return divergencias;
}

/* Verifica limites, cobertura exata, unicidade, tamanhos e agrupamento dos
 * membros. Vetores temporarios pertencem ao teste e sao liberados aqui.
 * Retorna 1 se o resultado e valido; 0 evita acessos posteriores em erro. */
static int conferir_particao(const ComponentesConexos *resultado, size_t vertices,
                             size_t componentes, const size_t *esperada)
{
    size_t falhas_antes = falhas;
    size_t soma = 0;
    size_t *ocorrencias;
    size_t *contagens;
    conferir("Resultado disponivel", 1, resultado != NULL, 1);
    if (resultado == NULL) {
        return 0;
    }
    conferir("Vertices", vertices, resultado->quantidade_vertices, 1);
    conferir("Componentes", componentes, resultado->quantidade_componentes, 1);
    if (resultado->quantidade_vertices != vertices ||
        resultado->quantidade_componentes != componentes) {
        return 0;
    }
    if (vertices == 0) {
        conferir("Vetores vazios NULL", 1, resultado->componente_por_vertice == NULL &&
                 resultado->tamanhos_componentes == NULL && resultado->membros == NULL, 1);
        return falhas == falhas_antes;
    }
    conferir("Vetores presentes", 1, resultado->componente_por_vertice != NULL &&
             resultado->tamanhos_componentes != NULL && resultado->membros != NULL, 1);
    if (falhas != falhas_antes) {
        return 0;
    }
    ocorrencias = calloc(vertices, sizeof(*ocorrencias));
    contagens = calloc(componentes, sizeof(*contagens));
    conferir("Memoria do oraculo", 1, ocorrencias != NULL && contagens != NULL, 0);
    if (ocorrencias == NULL || contagens == NULL) {
        free(ocorrencias);
        free(contagens);
        return 0;
    }
    for (size_t vertice = 0; vertice < vertices; vertice++) {
        size_t rotulo = resultado->componente_por_vertice[vertice];
        conferir("Rotulo valido", 1, rotulo < componentes, 0);
        if (rotulo < componentes) {
            contagens[rotulo]++;
        }
    }
    for (size_t componente = 0; componente < componentes; componente++) {
        size_t tamanho = resultado->tamanhos_componentes[componente];
        conferir("Tamanho contado pelos rotulos", contagens[componente], tamanho, vertices <= 8);
        conferir("Componente nao vazia", 1, tamanho > 0, 0);
        conferir("Tamanho cabe no restante", 1, tamanho <= vertices - soma, 0);
        if (tamanho > vertices - soma) {
            break;
        }
        for (size_t posicao = soma; posicao < soma + tamanho; posicao++) {
            size_t vertice = resultado->membros[posicao];
            conferir("Membro valido", 1, vertice < vertices, 0);
            if (vertice < vertices) {
                ocorrencias[vertice]++;
                conferir("Membro no grupo correto", componente,
                         resultado->componente_por_vertice[vertice], 0);
            }
        }
        soma += tamanho;
    }
    conferir("Soma dos tamanhos", vertices, soma, 1);
    size_t omitidos = 0;
    size_t repetidos = 0;
    for (size_t vertice = 0; vertice < vertices; vertice++) {
        omitidos += ocorrencias[vertice] == 0;
        repetidos += ocorrencias[vertice] > 1;
    }
    conferir("Vertices omitidos", 0, omitidos, 1);
    conferir("Vertices repetidos", 0, repetidos, 1);
    conferir("Pares divergentes da particao esperada", 0,
             divergencias_particao(vertices, esperada, resultado->componente_por_vertice), 1);
    free(ocorrencias);
    free(contagens);
    return falhas == falhas_antes;
}

/* Calcula e verifica nas duas representacoes, comparando tambem entre elas.
 * Imprime membros dos pequenos; escala usa resumos. Libera ambos os resultados. */
static void conferir_cenario(const char *nome, const GrafosTeste *grafos,
                             size_t componentes, const size_t *esperada)
{
    ComponentesConexos *resultados[2] = {NULL, NULL};
    int validos[2] = {0, 0};
    size_t vertices = quantidade_vertices_lista(grafos->lista);
    printf("\nCENARIO: %s\n", nome);
    for (int usar_matriz = 0; usar_matriz <= 1; usar_matriz++) {
#ifdef COMPONENTES_TESTAR_ALOCACAO
        tentativas_alocacao = 0;
#endif
        StatusComponentes status = usar_matriz
            ? identificar_componentes_conexos_matriz(grafos->matriz, &resultados[usar_matriz])
            : identificar_componentes_conexos_lista(grafos->lista, &resultados[usar_matriz]);
        puts(usar_matriz ? "MATRIZ" : "LISTA");
        conferir("Calculo", COMPONENTES_SUCESSO, status, 1);
#ifdef COMPONENTES_TESTAR_ALOCACAO
        conferir("Alocacoes independentes do numero de componentes", vertices == 0 ? 1 : 6,
                 tentativas_alocacao, 1);
#endif
        validos[usar_matriz] = conferir_particao(resultados[usar_matriz], vertices, componentes, esperada);
        if (validos[usar_matriz] && vertices <= 8) {
            conferir("Impressao", 1, imprimir_componentes_conexos(resultados[usar_matriz], stdout), 1);
        }
    }
    if (validos[0] && validos[1]) {
        conferir("Equivalencia lista/matriz", 0, divergencias_particao(vertices,
                 resultados[0]->componente_por_vertice, resultados[1]->componente_por_vertice), 1);
    }
    liberar_componentes_conexos(&resultados[0]);
    liberar_componentes_conexos(&resultados[1]);
}

/* Confere todas as adjacencias do original, com/sem (1,2), e as contagens.
 * O oraculo vem das cinco arestas declaradas, nao do resultado da travessia. */
static void conferir_topologia(const GrafosTeste *grafos, int conexao_presente)
{
    conferir("Arestas preservadas", conexao_presente ? 5 : 4,
             quantidade_arestas_lista(grafos->lista), 1);
    size_t divergencias = 0;
    for (size_t origem = 0; origem < 8; origem++) {
        for (size_t destino = 0; destino < 8; destino++) {
            int esperado = 0;
            int na_lista = -1;
            int na_matriz = -1;
            for (size_t aresta = 0; aresta < 5; aresta++) {
                if (aresta == 1 && !conexao_presente) {
                    continue;
                }
                size_t primeiro = arestas_exemplo[aresta][0];
                size_t segundo = arestas_exemplo[aresta][1];
                esperado |= (origem == primeiro && destino == segundo) ||
                            (origem == segundo && destino == primeiro);
            }
            conferir("Consulta lista", LISTA_SUCESSO,
                     existe_aresta_lista(grafos->lista, origem, destino, &na_lista), 0);
            conferir("Consulta matriz", MATRIZ_SUCESSO,
                     consultar_adjacencia(grafos->matriz, origem, destino, &na_matriz), 0);
            divergencias += na_lista != esperado || na_matriz != esperado;
        }
    }
    conferir("Adjacencias preservadas", 0, divergencias, 1);
}

/* Exemplo obrigatorio, uniao, reconstrucao, falha, restauracao e ciclos.
 * Resultados antigos sao relidos depois de alterar e liberar os grafos. */
static void testar_pequeno(void)
{
    GrafosTeste grafos;
    ComponentesConexos *antes_lista = NULL;
    ComponentesConexos *antes_matriz = NULL;
    if (!criar_exemplo(&grafos)) {
        return;
    }
    conferir_cenario("original: esperado {0,1,2} {3,4,5,6} {7}; tamanhos 3,4,1",
                     &grafos, 3, particao_original);
    conferir_topologia(&grafos, 1);
    alterar_conexao(&grafos, 2, 3, 1);
    alterar_conexao(&grafos, 6, 7, 1);
    conferir_cenario("uniao: esperado {0,1,2,3,4,5,6,7}; tamanho 8",
                     &grafos, 1, particao_conectada);
    liberar_grafos(&grafos);

    if (!criar_exemplo(&grafos)) {
        return;
    }
    conferir("Retrato anterior lista", COMPONENTES_SUCESSO,
             identificar_componentes_conexos_lista(grafos.lista, &antes_lista), 0);
    conferir("Retrato anterior matriz", COMPONENTES_SUCESSO,
             identificar_componentes_conexos_matriz(grafos.matriz, &antes_matriz), 0);
    alterar_conexao(&grafos, 1, 2, 0);
    conferir_cenario("falha (1,2): esperado {0,1} {2} {3,4,5,6} {7}; tamanhos 2,1,4,1",
                     &grafos, 4, particao_falha);
    conferir_topologia(&grafos, 0);
    alterar_conexao(&grafos, 1, 2, 1);
    conferir_cenario("restauracao: esperado {0,1,2} {3,4,5,6} {7}; tamanhos 3,4,1",
                     &grafos, 3, particao_original);
    conferir_topologia(&grafos, 1);
    alterar_conexao(&grafos, 0, 2, 1);
    alterar_conexao(&grafos, 3, 6, 1);
    conferir_cenario("ciclos sem repetir vertices: esperado tamanhos 3,4,1",
                     &grafos, 3, particao_original);
    conferir("Laco somente na matriz", MATRIZ_SUCESSO, inserir_aresta(grafos.matriz, 7, 7), 0);
    conferir("Laco duplicado idempotente", MATRIZ_SUCESSO, inserir_aresta(grafos.matriz, 7, 7), 0);
    conferir_cenario("laco no isolado da matriz: esperado tamanhos 3,4,1",
                     &grafos, 3, particao_original);
    liberar_grafos(&grafos);
    puts("RETRATOS ANTERIORES: esperado particao original apos alterar/liberar grafos");
    conferir_particao(antes_lista, 8, 3, particao_original);
    conferir_particao(antes_matriz, 8, 3, particao_original);
    liberar_componentes_conexos(&antes_lista);
    liberar_componentes_conexos(&antes_matriz);
}

/* Escala exclusivamente sintetica: 1.000 isolados, cadeia, corte ao meio e
 * restauracao. Oraculos por formula; nenhuma expectativa de dados reais. */
static void testar_escala(void)
{
    GrafosTeste grafos;
    size_t esperada[1000];
    if (!criar_grafos(1000, &grafos)) {
        return;
    }
    for (size_t vertice = 0; vertice < 1000; vertice++) {
        esperada[vertice] = vertice;
    }
    conferir_cenario("escala sintetica V=1000 E=0: esperado 1000 componentes de tamanho 1",
                     &grafos, 1000, esperada);
    for (size_t vertice = 1; vertice < 1000; vertice++) {
        alterar_conexao(&grafos, vertice - 1, vertice, 1);
    }
    for (size_t vertice = 0; vertice < 1000; vertice++) {
        esperada[vertice] = 0;
    }
    conferir_cenario("escala sintetica V=1000 E=999: esperado tamanho 1000", &grafos, 1, esperada);
    alterar_conexao(&grafos, 499, 500, 0);
    for (size_t vertice = 0; vertice < 1000; vertice++) {
        esperada[vertice] = vertice < 500 ? 15 : 72;
    }
    conferir_cenario("escala sintetica V=1000 E=998: esperado tamanhos 500,500", &grafos, 2, esperada);
    alterar_conexao(&grafos, 499, 500, 1);
    for (size_t vertice = 0; vertice < 1000; vertice++) {
        esperada[vertice] = 0;
    }
    conferir_cenario("escala restaurada V=1000 E=999: esperado tamanho 1000", &grafos, 1, esperada);
    liberar_grafos(&grafos);
}

/* NULL nao representa grafo vazio. Saida ocupada deve permanecer intacta;
 * liberacao aceita NULL/repeticao. Testa tambem grafo com um unico vertice. */
static void testar_contratos(void)
{
    GrafosTeste grafos;
    ComponentesConexos *resultado = NULL;
    const size_t esperada[] = {91};
    if (!criar_grafos(0, &grafos)) {
        return;
    }
    conferir_cenario("vazio V=0 E=0: esperado zero componentes e vetores NULL", &grafos, 0, NULL);
#ifdef COMPONENTES_TESTAR_ALOCACAO
    tentativas_alocacao = 0;
#endif
    conferir("Lista NULL", COMPONENTES_ARGUMENTO_INVALIDO,
             identificar_componentes_conexos_lista(NULL, &resultado), 1);
    conferir("Matriz NULL", COMPONENTES_ARGUMENTO_INVALIDO,
             identificar_componentes_conexos_matriz(NULL, &resultado), 1);
    conferir("Saida NULL lista", COMPONENTES_ARGUMENTO_INVALIDO,
             identificar_componentes_conexos_lista(grafos.lista, NULL), 1);
    conferir("Saida NULL matriz", COMPONENTES_ARGUMENTO_INVALIDO,
             identificar_componentes_conexos_matriz(grafos.matriz, NULL), 1);
    conferir("Saida preservada em erro", 1, resultado == NULL, 1);
#ifdef COMPONENTES_TESTAR_ALOCACAO
    conferir("Argumentos invalidos nao alocam", 0, tentativas_alocacao, 1);
#endif
    conferir("Resultado vazio", COMPONENTES_SUCESSO,
             identificar_componentes_conexos_lista(grafos.lista, &resultado), 0);
    if (resultado != NULL) {
        ComponentesConexos *dono = resultado;
#ifdef COMPONENTES_TESTAR_ALOCACAO
        tentativas_alocacao = 0;
#endif
        conferir("Dono ocupado lista", COMPONENTES_ARGUMENTO_INVALIDO,
                 identificar_componentes_conexos_lista(grafos.lista, &resultado), 1);
        conferir("Dono ocupado matriz", COMPONENTES_ARGUMENTO_INVALIDO,
                 identificar_componentes_conexos_matriz(grafos.matriz, &resultado), 1);
        conferir("Dono preservado", 1, resultado == dono, 1);
#ifdef COMPONENTES_TESTAR_ALOCACAO
        conferir("Dono ocupado nao aloca", 0, tentativas_alocacao, 1);
#endif
        conferir_particao(resultado, 0, 0, NULL);
    }
    conferir("Impressao NULL", 0, imprimir_componentes_conexos(NULL, stdout), 1);
    conferir("Arquivo NULL", 0, imprimir_componentes_conexos(resultado, NULL), 1);
    liberar_componentes_conexos(&resultado);
    conferir("Dono liberado", 1, resultado == NULL, 1);
    liberar_componentes_conexos(&resultado);
    liberar_componentes_conexos(NULL);
    liberar_grafos(&grafos);
    if (criar_grafos(1, &grafos)) {
        conferir_cenario("unitario V=1 E=0: esperado {0}, tamanho 1", &grafos, 1, esperada);
        liberar_grafos(&grafos);
    }
}

/* Verifica o relatorio como texto, incluindo a indisponibilidade de subestacao.
 * O arquivo temporario e o resultado sao liberados dentro desta funcao. */
static void testar_relatorio(void)
{
    GrafosTeste grafos;
    ComponentesConexos *resultado = NULL;
    if (!criar_exemplo(&grafos)) {
        return;
    }
    conferir("Calculo do relatorio", COMPONENTES_SUCESSO,
             identificar_componentes_conexos_lista(grafos.lista, &resultado), 0);
    FILE *arquivo = tmpfile();
    conferir("Arquivo temporario", 1, arquivo != NULL, 1);
    if (arquivo != NULL && resultado != NULL) {
        char texto[1024];
        conferir("Gravar relatorio", 1, imprimir_componentes_conexos(resultado, arquivo), 1);
        conferir("Flush relatorio", 0, fflush(arquivo), 1);
        rewind(arquivo);
        size_t lidos = fread(texto, 1, sizeof(texto) - 1, arquivo);
        texto[lidos] = '\0';
        conferir("Leitura sem erro", 0, ferror(arquivo) != 0, 1);
        conferir("Total no relatorio", 1, strstr(texto, "Total de componentes: 3; vertices: 8") != NULL, 1);
        conferir("Subestacao indisponivel", 1, strstr(texto, "Subestacao: indisponivel") != NULL, 1);
        /* O texto tambem nao exige uma numeracao especifica dos rotulos. */
        conferir("Membros do grupo de 0", 1, strstr(texto, "tamanho=3; membros={0,1,2}") != NULL, 1);
        conferir("Membros do grupo de 3", 1, strstr(texto, "tamanho=4; membros={3,4,5,6}") != NULL, 1);
        conferir("Membro isolado", 1, strstr(texto, "tamanho=1; membros={7}") != NULL, 1);
    }
    if (arquivo != NULL) {
        conferir("Fechar relatorio", 0, fclose(arquivo), 1);
    }
    liberar_componentes_conexos(&resultado);
    liberar_grafos(&grafos);
}

#ifdef COMPONENTES_TESTAR_ALOCACAO
/* Forca cada ponto de alocacao, incluindo os da BFS, nas duas representacoes.
 * Confere saida NULL, limpeza parcial, topologia e recuperacao posterior. */
static void testar_falhas_alocacao(int vazio)
{
    GrafosTeste grafos;
    if (!(vazio ? criar_grafos(0, &grafos) : criar_exemplo(&grafos))) {
        return;
    }
    size_t alocacoes = vazio ? 1 : 6;
    for (int usar_matriz = 0; usar_matriz <= 1; usar_matriz++) {
        for (size_t etapa = 0; etapa < alocacoes; etapa++) {
            ComponentesConexos *resultado = NULL;
            tentativas_alocacao = 0;
            falhar_na_tentativa = etapa;
            printf("\nFALHA DE ALOCACAO: %s, vazio=%d, etapa=%zu\n",
                   usar_matriz ? "matriz" : "lista", vazio, etapa);
            StatusComponentes status = usar_matriz
                ? identificar_componentes_conexos_matriz(grafos.matriz, &resultado)
                : identificar_componentes_conexos_lista(grafos.lista, &resultado);
            conferir("Retorno sem memoria", COMPONENTES_SEM_MEMORIA, status, 1);
            conferir("Saida NULL em erro", 1, resultado == NULL, 1);
            conferir("Tentativas ate a falha", etapa + 1, tentativas_alocacao, 1);
            conferir("Blocos vivos apos erro", 0, blocos_vivos, 1);
            liberar_componentes_conexos(&resultado);
            if (!vazio) {
                conferir_topologia(&grafos, 1);
            }
            falhar_na_tentativa = SIZE_MAX;
            tentativas_alocacao = 0;
            status = usar_matriz
                ? identificar_componentes_conexos_matriz(grafos.matriz, &resultado)
                : identificar_componentes_conexos_lista(grafos.lista, &resultado);
            conferir("Recuperacao", COMPONENTES_SUCESSO, status, 1);
            conferir("Alocacoes na recuperacao", alocacoes, tentativas_alocacao, 1);
            conferir_particao(resultado, vazio ? 0 : 8, vazio ? 0 : 3, particao_original);
            conferir("Blocos do resultado", vazio ? 1 : 4, blocos_vivos, 1);
            liberar_componentes_conexos(&resultado);
            conferir("Blocos vivos apos recuperacao", 0, blocos_vivos, 1);
        }
    }
    liberar_grafos(&grafos);
}
#endif

/* Executa todos os testes; divergencias produzem EXIT_FAILURE mesmo com NDEBUG. */
int main(void)
{
    testar_pequeno();
    testar_escala();
    testar_contratos();
    testar_relatorio();
#ifdef COMPONENTES_TESTAR_ALOCACAO
    conferir("Blocos vivos apos funcionais", 0, blocos_vivos, 1);
    testar_falhas_alocacao(0);
    testar_falhas_alocacao(1);
    conferir("Blocos vivos finais componentes/BFS", 0, blocos_vivos, 1);
#endif
    puts("DATASET REAL: NAO EXECUTADO / PENDENTE (sem arquivos e carregador #4).");
    printf("RESULTADO COMPONENTES: %s (%zu verificacoes, %zu falhas)\n",
           falhas == 0 ? "PASSOU" : "FALHOU", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
