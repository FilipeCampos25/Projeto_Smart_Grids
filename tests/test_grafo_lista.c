#include "grafo_lista.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef GRAFO_LISTA_TESTAR_ALOCACAO
#include "alocacao_teste.h"

static size_t blocos_vivos;
static size_t tentativas_alocacao;
static size_t falhar_na_tentativa = SIZE_MAX;

/* Falha somente na tentativa selecionada, sem afetar a libc do teste. */
static int deve_falhar(void)
{
    return tentativas_alocacao++ == falhar_na_tentativa;
}

/* Bloco devolvido pertence ao chamador, que deve usar teste_free. */
void *teste_malloc(size_t tamanho)
{
    void *bloco;
    if (deve_falhar()) {
        return NULL;
    }
    bloco = malloc(tamanho);
    if (bloco != NULL) {
        blocos_vivos++;
    }
    return bloco;
}

/* Preserva o bloco anterior em falha; o modulo nunca pede tamanho zero. */
void *teste_realloc(void *ponteiro, size_t tamanho)
{
    int novo_bloco = ponteiro == NULL;
    void *bloco;
    if (deve_falhar()) {
        return NULL;
    }
    bloco = realloc(ponteiro, tamanho);
    if (bloco != NULL && novo_bloco) {
        blocos_vivos++;
    }
    return bloco;
}

/* Libera o bloco e atualiza a contagem; aceita NULL como free. */
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

/* Compara numeros, registra falhas e mostra esperado/obtido quando pedido
 * ou quando houver divergencia. Nao usa assert: funciona tambem com NDEBUG. */
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

/* Compara conjuntos sem impor ordem. Confere tamanho e uma ocorrencia de
 * cada elemento esperado, detectando tambem vizinhos repetidos/inesperados.
 * Limita a iteracao ao numero de vertices para denunciar eventuais ciclos. */
static void conferir_vizinhos(const GrafoLista *grafo, size_t vertice,
                              const size_t *esperados, size_t quantidade,
                              int mostrar)
{
    const VizinhoLista *primeiro = NULL;
    const VizinhoLista *vizinho;
    size_t quantidade_obtida = 0;
    size_t limite = quantidade_vertices_lista(grafo);
    int correto = buscar_vizinhos_lista(grafo, vertice, &primeiro) == LISTA_SUCESSO;

    for (vizinho = primeiro; vizinho != NULL && quantidade_obtida <= limite;
         vizinho = proximo_vizinho_lista(vizinho)) {
        quantidade_obtida++;
    }
    correto = correto && vizinho == NULL && quantidade_obtida == quantidade;
    for (size_t esperado = 0; esperado < quantidade; esperado++) {
        size_t ocorrencias = 0;
        size_t percorridos = 0;
        for (vizinho = primeiro; vizinho != NULL && percorridos <= limite;
             vizinho = proximo_vizinho_lista(vizinho)) {
            if (indice_vizinho_lista(vizinho) == esperados[esperado]) {
                ocorrencias++;
            }
            percorridos++;
        }
        correto = correto && ocorrencias == 1;
    }
    verificacoes++;
    if (!correto) {
        falhas++;
    }
    if (mostrar || !correto) {
        printf("vertice %zu: esperado={", vertice);
        for (size_t posicao = 0; posicao < quantidade; posicao++) {
            printf("%s%zu", posicao == 0 ? "" : ",", esperados[posicao]);
        }
        printf("} obtido={");
        size_t percorridos = 0;
        for (vizinho = primeiro; vizinho != NULL && percorridos <= limite;
             vizinho = proximo_vizinho_lista(vizinho)) {
            printf("%s%zu", percorridos == 0 ? "" : ",",
                   indice_vizinho_lista(vizinho));
            percorridos++;
        }
        printf("} %s\n", correto ? "PASSOU" : "FALHOU");
    }
}

/* Cria vertices isolados com indices consecutivos. Em falha libera tudo
 * e retorna NULL; em sucesso o chamador e responsavel pelo grafo. */
static GrafoLista *criar_vertices(size_t quantidade)
{
    GrafoLista *grafo = criar_grafo_lista();
    conferir_numero("criacao do grafo", 1, grafo != NULL, 0);
    if (grafo == NULL) {
        return NULL;
    }
    for (size_t vertice = 0; vertice < quantidade; vertice++) {
        size_t indice = SIZE_MAX;
        ResultadoLista resultado = inserir_vertice_lista(grafo, &indice);
        conferir_numero("insercao de vertice", LISTA_SUCESSO, resultado, 0);
        if (resultado != LISTA_SUCESSO) {
            liberar_grafo_lista(grafo);
            return NULL;
        }
        conferir_numero("indice consecutivo", vertice, indice, 0);
    }
    return grafo;
}

/* Confere o exemplo da issue, com a conexao (3,4) presente ou removida. */
static void conferir_exemplo(const GrafoLista *grafo, int conexao_presente)
{
    const size_t vizinhos_0[] = {1, 2};
    const size_t vizinhos_1[] = {0, 3};
    const size_t vizinhos_2[] = {0, 3};
    const size_t vizinhos_3[] = {1, 2, 4};
    const size_t vizinhos_4[] = {3};
    conferir_numero("vertices", 6, quantidade_vertices_lista(grafo), 1);
    conferir_numero("arestas", conexao_presente ? 5 : 4,
                    quantidade_arestas_lista(grafo), 1);
    conferir_vizinhos(grafo, 0, vizinhos_0, 2, 1);
    conferir_vizinhos(grafo, 1, vizinhos_1, 2, 1);
    conferir_vizinhos(grafo, 2, vizinhos_2, 2, 1);
    conferir_vizinhos(grafo, 3, vizinhos_3, conexao_presente ? 3 : 2, 1);
    conferir_vizinhos(grafo, 4, vizinhos_4, conexao_presente ? 1 : 0, 1);
    conferir_vizinhos(grafo, 5, NULL, 0, 1);
}

/* Executa o caso obrigatorio, incluindo consulta, remocao e restauracao. */
static void testar_exemplo(void)
{
    const size_t arestas[][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {3, 4}};
    GrafoLista *grafo = criar_vertices(6);
    int existe = -1;
    if (grafo == NULL) {
        return;
    }
    puts("EXEMPLO PEQUENO: estado inicial");
    for (size_t indice = 0; indice < 5; indice++) {
        conferir_numero("insercao de aresta", LISTA_SUCESSO,
                        inserir_aresta_lista(grafo, arestas[indice][0],
                                             arestas[indice][1]), 0);
    }
    conferir_exemplo(grafo, 1);
    conferir_numero("identificar (4,3)", LISTA_SUCESSO,
                    existe_aresta_lista(grafo, 4, 3, &existe), 1);
    conferir_numero("conexao (4,3) presente", 1, (size_t)existe, 1);
    puts("EXEMPLO PEQUENO: remocao de (3,4)");
    conferir_numero("remocao", LISTA_SUCESSO,
                    remover_aresta_lista(grafo, 3, 4), 1);
    conferir_exemplo(grafo, 0);
    for (size_t origem = 3; origem <= 4; origem++) {
        conferir_numero("consulta apos remocao", LISTA_SUCESSO,
                        existe_aresta_lista(grafo, origem, 7 - origem, &existe), 0);
        conferir_numero("conexao ausente nos dois sentidos", 0, (size_t)existe, 1);
    }
    conferir_numero("remocao repetida", LISTA_ARESTA_INEXISTENTE,
                    remover_aresta_lista(grafo, 4, 3), 1);
    puts("EXEMPLO PEQUENO: restauracao por reinsercao invertida (4,3)");
    conferir_numero("reinsercao", LISTA_SUCESSO,
                    inserir_aresta_lista(grafo, 4, 3), 1);
    conferir_exemplo(grafo, 1);
    liberar_grafo_lista(grafo);
}

/* Confere cada vizinhanca de uma cadeia, inclusive as duas extremidades. */
static void conferir_cadeia(const GrafoLista *grafo, size_t vertices,
                            size_t arestas)
{
    conferir_numero("vertices da cadeia", vertices, quantidade_vertices_lista(grafo), 0);
    conferir_numero("arestas da cadeia", arestas, quantidade_arestas_lista(grafo), 0);
    for (size_t vertice = 0; vertice < vertices; vertice++) {
        size_t esperados[2];
        size_t quantidade = 0;
        if (vertice > 0 && vertice <= arestas) {
            esperados[quantidade++] = vertice - 1;
        }
        if (vertice < arestas) {
            esperados[quantidade++] = vertice + 1;
        }
        conferir_vizinhos(grafo, vertice, esperados, quantidade, 0);
    }
}

/* Teste de escala sintetico; nao substitui a validacao com dataset real. */
static void testar_escala(void)
{
    GrafoLista *grafo = criar_vertices(1000);
    size_t falhas_antes = falhas;
    if (grafo == NULL) {
        return;
    }
    for (size_t vertice = 1; vertice < 1000; vertice++) {
        conferir_numero("insercao na cadeia", LISTA_SUCESSO,
                        inserir_aresta_lista(grafo, vertice - 1, vertice), 0);
    }
    conferir_cadeia(grafo, 1000, 999);
    conferir_numero("CADEIA SINTETICA: vertices", 1000,
                    quantidade_vertices_lista(grafo), 1);
    conferir_numero("CADEIA SINTETICA: arestas", 999,
                    quantidade_arestas_lista(grafo), 1);
    conferir_numero("CADEIA SINTETICA: divergencias em todas as vizinhancas",
                    0, falhas - falhas_antes, 1);
    liberar_grafo_lista(grafo);
}

/* Valida erros, saidas preservadas e politica provisoria de grafo simples. */
static void testar_limites(void)
{
    GrafoLista *grafo = criar_vertices(0);
    const VizinhoLista *vizinhos = NULL;
    size_t indice = SIZE_MAX;
    int existe = -1;
    if (grafo == NULL) {
        return;
    }
    puts("VALIDACAO DE ENTRADAS E POLITICAS");
    conferir_numero("grafo vazio", 0, quantidade_vertices_lista(grafo), 1);
    conferir_numero("arestas no grafo vazio", 0, quantidade_arestas_lista(grafo), 0);
    conferir_numero("consulta no grafo vazio", LISTA_ARGUMENTO_INVALIDO,
                    buscar_vizinhos_lista(grafo, 0, &vizinhos), 1);
    conferir_numero("aresta no grafo vazio", LISTA_ARGUMENTO_INVALIDO,
                    inserir_aresta_lista(grafo, 0, 1), 0);
    conferir_numero("vertice em grafo nulo", LISTA_ARGUMENTO_INVALIDO,
                    inserir_vertice_lista(NULL, &indice), 0);
    conferir_numero("saida preservada", SIZE_MAX, indice, 0);
    conferir_numero("saida de vertice nula", LISTA_ARGUMENTO_INVALIDO,
                    inserir_vertice_lista(grafo, NULL), 0);
    conferir_numero("sem insercao em erro", 0, quantidade_vertices_lista(grafo), 0);
    for (size_t vertice = 0; vertice < 5; vertice++) {
        conferir_numero("inserir isolado", LISTA_SUCESSO,
                        inserir_vertice_lista(grafo, &indice), 0);
    }
    conferir_numero("indice fora do intervalo", LISTA_ARGUMENTO_INVALIDO,
                    inserir_aresta_lista(grafo, 0, 5), 1);
    conferir_numero("indice SIZE_MAX", LISTA_ARGUMENTO_INVALIDO,
                    inserir_aresta_lista(grafo, SIZE_MAX, 0), 1);
    conferir_numero("grafo nulo", LISTA_ARGUMENTO_INVALIDO,
                    inserir_aresta_lista(NULL, 0, 1), 0);
    conferir_numero("laco rejeitado", LISTA_LACO_NAO_PERMITIDO,
                    inserir_aresta_lista(grafo, 0, 0), 1);
    conferir_numero("insercao valida", LISTA_SUCESSO,
                    inserir_aresta_lista(grafo, 0, 1), 0);
    conferir_numero("duplicata rejeitada", LISTA_ARESTA_DUPLICADA,
                    inserir_aresta_lista(grafo, 0, 1), 1);
    conferir_numero("duplicata invertida rejeitada", LISTA_ARESTA_DUPLICADA,
                    inserir_aresta_lista(grafo, 1, 0), 1);
    conferir_numero("duplicatas nao alteram contagem", 1, quantidade_arestas_lista(grafo), 1);
    conferir_vizinhos(grafo, 0, (const size_t[]){1}, 1, 1);
    conferir_vizinhos(grafo, 1, (const size_t[]){0}, 1, 1);
    conferir_numero("consulta valida", LISTA_SUCESSO,
                    buscar_vizinhos_lista(grafo, 0, &vizinhos), 0);
    const VizinhoLista *primeiro = vizinhos;
    conferir_numero("consulta de vertice invalido", LISTA_ARGUMENTO_INVALIDO,
                    buscar_vizinhos_lista(grafo, SIZE_MAX, &vizinhos), 0);
    conferir_numero("consulta preserva saida em erro", 1, vizinhos == primeiro, 0);
    conferir_numero("saida de vizinhos nula", LISTA_ARGUMENTO_INVALIDO,
                    buscar_vizinhos_lista(grafo, 0, NULL), 0);
    conferir_numero("consulta de grafo nulo", LISTA_ARGUMENTO_INVALIDO,
                    buscar_vizinhos_lista(NULL, 0, &vizinhos), 0);
    conferir_numero("consulta de aresta invalida", LISTA_ARGUMENTO_INVALIDO,
                    existe_aresta_lista(grafo, 0, 5, &existe), 0);
    conferir_numero("consulta preserva sentinela", 1, existe == -1, 0);
    conferir_numero("saida de existencia nula", LISTA_ARGUMENTO_INVALIDO,
                    existe_aresta_lista(grafo, 0, 1, NULL), 0);
    conferir_numero("existencia em grafo nulo", LISTA_ARGUMENTO_INVALIDO,
                    existe_aresta_lista(NULL, 0, 1, &existe), 0);
    conferir_numero("consulta de laco", LISTA_SUCESSO,
                    existe_aresta_lista(grafo, 0, 0, &existe), 0);
    conferir_numero("laco nao existe", 0, (size_t)existe, 0);
    conferir_numero("remocao de indice invalido", LISTA_ARGUMENTO_INVALIDO,
                    remover_aresta_lista(grafo, 5, 0), 0);
    conferir_numero("remocao em grafo nulo", LISTA_ARGUMENTO_INVALIDO,
                    remover_aresta_lista(NULL, 0, 1), 0);
    conferir_numero("remocao de laco inexistente", LISTA_ARESTA_INEXISTENTE,
                    remover_aresta_lista(grafo, 0, 0), 0);
    conferir_numero("remocao de conexao inexistente", LISTA_ARESTA_INEXISTENTE,
                    remover_aresta_lista(grafo, 1, 2), 0);
    conferir_numero("indice de vizinho nulo", SIZE_MAX, indice_vizinho_lista(NULL), 0);
    conferir_numero("proximo de vizinho nulo", 1, proximo_vizinho_lista(NULL) == NULL, 0);
    conferir_numero("contagem de vertices nula", 0, quantidade_vertices_lista(NULL), 0);
    conferir_numero("contagem de arestas nula", 0, quantidade_arestas_lista(NULL), 0);

    /* Cabeca, meio e cauda da lista de 0, sempre verificando os dois lados. */
    for (size_t destino = 2; destino <= 4; destino++) {
        conferir_numero("insercao estrela", LISTA_SUCESSO,
                        inserir_aresta_lista(grafo, 0, destino), 0);
    }
    const size_t ordem_remocao[] = {3, 1, 4, 2};
    int presentes[] = {0, 1, 1, 1, 1};
    for (size_t posicao = 0; posicao < 4; posicao++) {
        size_t destino = ordem_remocao[posicao];
        size_t esperados[4];
        size_t quantidade = 0;
        conferir_numero("remover meio/cauda/cabeca", LISTA_SUCESSO,
                        remover_aresta_lista(grafo, destino, 0), 0);
        presentes[destino] = 0;
        for (size_t vertice = 1; vertice <= 4; vertice++) {
            if (presentes[vertice]) {
                esperados[quantidade++] = vertice;
            }
            conferir_vizinhos(grafo, vertice, (const size_t[]){0},
                               presentes[vertice] ? 1 : 0, 0);
        }
        conferir_vizinhos(grafo, 0, esperados, quantidade, 0);
        conferir_numero("contagem apos remocao", 3 - posicao,
                        quantidade_arestas_lista(grafo), 0);
    }
    liberar_grafo_lista(grafo);
    liberar_grafo_lista(NULL);
}

#ifdef GRAFO_LISTA_TESTAR_ALOCACAO
/* Constroi uma cadeia sob injecao de falha. Confere o estado parcial e,
 * conforme recuperar, tenta novamente ou libera imediatamente esse estado.
 * A reserva do vetor tambem ocorre quando ja existem arestas no grafo. */
static int construir_com_falha(int recuperar)
{
    GrafoLista *grafo = criar_grafo_lista();
    int encontrou_falha = grafo == NULL;
    if (grafo == NULL) {
        return encontrou_falha;
    }
    for (size_t vertice = 0; vertice < 12; vertice++) {
        size_t indice = SIZE_MAX;
        ResultadoLista resultado = inserir_vertice_lista(grafo, &indice);
        if (resultado != LISTA_SUCESSO) {
            encontrou_falha = 1;
            conferir_numero("falha de reserva", LISTA_SEM_MEMORIA, resultado, 0);
            conferir_numero("indice preservado na falha", SIZE_MAX, indice, 0);
            conferir_cadeia(grafo, vertice, vertice > 0 ? vertice - 1 : 0);
            if (!recuperar) {
                liberar_grafo_lista(grafo);
                return encontrou_falha;
            }
            conferir_numero("recuperar reserva", LISTA_SUCESSO,
                            inserir_vertice_lista(grafo, &indice), 0);
        }
        conferir_numero("indice apos reserva", vertice, indice, 0);
        if (vertice == 0) {
            continue;
        }
        size_t blocos_antes = blocos_vivos;
        resultado = inserir_aresta_lista(grafo, vertice - 1, vertice);
        if (resultado != LISTA_SUCESSO) {
            encontrou_falha = 1;
            conferir_numero("falha de aresta", LISTA_SEM_MEMORIA, resultado, 0);
            conferir_numero("sem vazamento de meia aresta", blocos_antes, blocos_vivos, 0);
            conferir_cadeia(grafo, vertice + 1, vertice - 1);
            if (!recuperar) {
                liberar_grafo_lista(grafo);
                return encontrou_falha;
            }
            conferir_numero("recuperar insercao", LISTA_SUCESSO,
                            inserir_aresta_lista(grafo, vertice - 1, vertice), 0);
        }
    }
    conferir_cadeia(grafo, 12, 11);
    liberar_grafo_lista(grafo);
    return encontrou_falha;
}
/* Falha uma vez em cada alocacao da construcao: grafo, reservas e os dois
 * sentidos das arestas. O contador verifica liberacao em todos os caminhos. */
static void testar_falhas_alocacao(void)
{
    size_t falhas_antes = falhas;
    tentativas_alocacao = 0;
    falhar_na_tentativa = SIZE_MAX;
    conferir_numero("construcao de referencia sem falha", 0, construir_com_falha(1), 0);
    size_t total_alocacoes = tentativas_alocacao;
    conferir_numero("blocos apos referencia", 0, blocos_vivos, 0);
    for (size_t tentativa = 0; tentativa < total_alocacoes; tentativa++) {
        for (int recuperar = 0; recuperar <= 1; recuperar++) {
            tentativas_alocacao = 0;
            falhar_na_tentativa = tentativa;
            conferir_numero("falha injetada observada", 1,
                            construir_com_falha(recuperar), 0);
            conferir_numero("blocos vivos apos falha e liberacao", 0, blocos_vivos, 0);
        }
    }
    falhar_na_tentativa = SIZE_MAX;
    printf("ALOCACAO: %zu pontos de falha exercitados\n", total_alocacoes);
    conferir_numero("ALOCACAO: divergencias", 0, falhas - falhas_antes, 1);
    conferir_numero("ALOCACAO: blocos vivos finais", 0, blocos_vivos, 1);
}
#endif

/* Retorna EXIT_FAILURE se qualquer comparacao falhar. */
int main(void)
{
    testar_exemplo();
    testar_escala();
    testar_limites();
#ifdef GRAFO_LISTA_TESTAR_ALOCACAO
    conferir_numero("blocos vivos apos testes funcionais", 0, blocos_vivos, 1);
    testar_falhas_alocacao();
#endif
    printf("RESULTADO: %s (%zu verificacoes, %zu falhas)\n",
           falhas == 0 ? "PASSOU" : "FALHOU", verificacoes, falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
