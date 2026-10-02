#include "simulacao_falha.h"

#include <stdint.h>
#include <stdlib.h>

typedef size_t (*ObterQuantidadeVertices)(const void *grafo);
typedef int (*ConsultarConexao)(const void *grafo, size_t origem, size_t destino,
                               int *existe);
typedef int (*AlterarConexao)(void *grafo, size_t origem, size_t destino);
typedef StatusComponentes (*IdentificarComponentes)(
    const void *grafo, ComponentesConexos **resultado);

typedef struct {
    ObterQuantidadeVertices quantidade_vertices;
    ConsultarConexao consultar;
    AlterarConexao remover;
    AlterarConexao inserir;
    IdentificarComponentes identificar;
} OperacoesGrafo;

void liberar_resultado_simulacao(ResultadoSimulacaoFalha **resultado)
{
    if (resultado != NULL && *resultado != NULL) {
        liberar_componentes_conexos(&(*resultado)->componentes_antes);
        liberar_componentes_conexos(&(*resultado)->componentes_depois);
        liberar_componentes_conexos(&(*resultado)->componentes_apos_restauracao);
        free(*resultado);
        *resultado = NULL;
    }
}

static StatusSimulacaoFalha converter_status_componentes(StatusComponentes status)
{
    switch (status) {
    case COMPONENTES_SUCESSO: return SIMULACAO_SUCESSO;
    case COMPONENTES_SEM_MEMORIA: return SIMULACAO_SEM_MEMORIA;
    case COMPONENTES_LIMITE_EXCEDIDO: return SIMULACAO_LIMITE_EXCEDIDO;
    default: return SIMULACAO_ARGUMENTO_INVALIDO;
    }
}

/* Os rotulos sao deterministas porque componentes examina as origens em ordem:
 * cada componente recebe o rotulo na visita de seu menor indice. */
static int mesma_particao(const ComponentesConexos *primeira,
                          const ComponentesConexos *segunda)
{
    if (primeira == NULL || segunda == NULL ||
        primeira->quantidade_vertices != segunda->quantidade_vertices ||
        primeira->quantidade_componentes != segunda->quantidade_componentes) {
        return 0;
    }
    for (size_t vertice = 0; vertice < primeira->quantidade_vertices; vertice++) {
        if (primeira->componente_por_vertice[vertice] !=
            segunda->componente_por_vertice[vertice]) {
            return 0;
        }
    }
    return 1;
}

static StatusSimulacaoFalha simular_falha(
    void *grafo, const OperacoesGrafo *operacoes, size_t vertice_origem,
    size_t vertice_destino, size_t subestacao,
    ResultadoSimulacaoFalha **resultado)
{
    size_t quantidade_vertices;
    ResultadoSimulacaoFalha *simulacao;
    StatusComponentes status_componentes;
    StatusSimulacaoFalha status;
    int conexao_existe = 0;

    if (grafo == NULL || operacoes == NULL || resultado == NULL ||
        *resultado != NULL) {
        return SIMULACAO_ARGUMENTO_INVALIDO;
    }
    if (subestacao == SIZE_MAX) {
        return SIMULACAO_SUBESTACAO_NAO_INFORMADA;
    }
    quantidade_vertices = operacoes->quantidade_vertices(grafo);
    if (vertice_origem >= quantidade_vertices ||
        vertice_destino >= quantidade_vertices || subestacao >= quantidade_vertices) {
        return SIMULACAO_VERTICE_INEXISTENTE;
    }
    if (!operacoes->consultar(grafo, vertice_origem, vertice_destino,
                             &conexao_existe)) {
        return SIMULACAO_ARGUMENTO_INVALIDO;
    }
    if (!conexao_existe) {
        return SIMULACAO_ARESTA_INEXISTENTE;
    }

    simulacao = malloc(sizeof(*simulacao));
    if (simulacao == NULL) {
        return SIMULACAO_SEM_MEMORIA;
    }
    *simulacao = (ResultadoSimulacaoFalha){0};
    simulacao->vertice_origem = vertice_origem;
    simulacao->vertice_destino = vertice_destino;
    simulacao->subestacao = subestacao;

    status_componentes = operacoes->identificar(
        grafo, &simulacao->componentes_antes);
    status = converter_status_componentes(status_componentes);
    if (status != SIMULACAO_SUCESSO) {
        liberar_resultado_simulacao(&simulacao);
        return status;
    }
    simulacao->componente_principal_antes =
        simulacao->componentes_antes->componente_por_vertice[subestacao];

    /* A remocao temporaria representa a falha. As buscas posteriores enxergam
     * somente as conexoes ainda presentes, sem qualquer algoritmo paralelo. */
    if (!operacoes->remover(grafo, vertice_origem, vertice_destino)) {
        liberar_resultado_simulacao(&simulacao);
        return SIMULACAO_ARGUMENTO_INVALIDO;
    }
    status_componentes = operacoes->identificar(
        grafo, &simulacao->componentes_depois);
    status = converter_status_componentes(status_componentes);
    if (status != SIMULACAO_SUCESSO) {
        int restaurou = operacoes->inserir(grafo, vertice_origem, vertice_destino);
        liberar_resultado_simulacao(&simulacao);
        return restaurou ? status : SIMULACAO_FALHA_RESTAURACAO;
    }

    simulacao->componente_principal_depois =
        simulacao->componentes_depois->componente_por_vertice[subestacao];
    simulacao->vertices_componente_principal =
        simulacao->componentes_depois->tamanhos_componentes[
            simulacao->componente_principal_depois];
    simulacao->quantidade_isolados = quantidade_vertices -
        simulacao->vertices_componente_principal;
    for (size_t vertice = 0; vertice < quantidade_vertices; vertice++) {
        int pertencia_principal =
            simulacao->componentes_antes->componente_por_vertice[vertice] ==
            simulacao->componente_principal_antes;
        int ficou_fora =
            simulacao->componentes_depois->componente_por_vertice[vertice] !=
            simulacao->componente_principal_depois;
        simulacao->quantidade_novos_isolados += pertencia_principal && ficou_fora;
    }
    simulacao->houve_separacao =
        simulacao->componentes_depois->quantidade_componentes >
        simulacao->componentes_antes->quantidade_componentes;

    /* A conexao e recolocada antes do ultimo retrato. Na lista, essa etapa pode
     * alocar dois nos; uma falha e explicitada para nao fingir recuperacao. */
    if (!operacoes->inserir(grafo, vertice_origem, vertice_destino)) {
        liberar_resultado_simulacao(&simulacao);
        return SIMULACAO_FALHA_RESTAURACAO;
    }
    status_componentes = operacoes->identificar(
        grafo, &simulacao->componentes_apos_restauracao);
    status = converter_status_componentes(status_componentes);
    if (status != SIMULACAO_SUCESSO) {
        liberar_resultado_simulacao(&simulacao);
        return status;
    }
    conexao_existe = 0;
    simulacao->topologia_restaurada =
        operacoes->consultar(grafo, vertice_origem, vertice_destino,
                             &conexao_existe) && conexao_existe &&
        mesma_particao(simulacao->componentes_antes,
                       simulacao->componentes_apos_restauracao);
    *resultado = simulacao;
    return SIMULACAO_SUCESSO;
}

static size_t quantidade_lista(const void *grafo)
{
    return quantidade_vertices_lista(grafo);
}

static int consultar_lista(const void *grafo, size_t origem, size_t destino,
                           int *existe)
{
    return existe_aresta_lista(grafo, origem, destino, existe) == LISTA_SUCESSO;
}

static int remover_lista(void *grafo, size_t origem, size_t destino)
{
    return remover_aresta_lista(grafo, origem, destino) == LISTA_SUCESSO;
}

static int inserir_lista(void *grafo, size_t origem, size_t destino)
{
    return inserir_aresta_lista(grafo, origem, destino) == LISTA_SUCESSO;
}

static StatusComponentes componentes_lista(const void *grafo,
                                            ComponentesConexos **resultado)
{
    return identificar_componentes_conexos_lista(grafo, resultado);
}

static size_t quantidade_matriz(const void *grafo)
{
    return quantidade_vertices_matriz(grafo);
}

static int consultar_matriz(const void *grafo, size_t origem, size_t destino,
                            int *existe)
{
    return consultar_adjacencia(grafo, origem, destino, existe) == MATRIZ_SUCESSO;
}

static int remover_matriz(void *grafo, size_t origem, size_t destino)
{
    return remover_aresta(grafo, origem, destino) == MATRIZ_SUCESSO;
}

static int inserir_matriz(void *grafo, size_t origem, size_t destino)
{
    return inserir_aresta(grafo, origem, destino) == MATRIZ_SUCESSO;
}

static StatusComponentes componentes_matriz(const void *grafo,
                                             ComponentesConexos **resultado)
{
    return identificar_componentes_conexos_matriz(grafo, resultado);
}

static const OperacoesGrafo operacoes_lista = {
    quantidade_lista, consultar_lista, remover_lista, inserir_lista,
    componentes_lista
};

static const OperacoesGrafo operacoes_matriz = {
    quantidade_matriz, consultar_matriz, remover_matriz, inserir_matriz,
    componentes_matriz
};

StatusSimulacaoFalha simular_falha_aresta_lista(
    GrafoLista *grafo, size_t vertice_origem, size_t vertice_destino,
    size_t subestacao, ResultadoSimulacaoFalha **resultado)
{
    return simular_falha(grafo, &operacoes_lista, vertice_origem,
                         vertice_destino, subestacao, resultado);
}

StatusSimulacaoFalha simular_falha_aresta_matriz(
    MatrizAdjacencia *matriz, size_t vertice_origem, size_t vertice_destino,
    size_t subestacao, ResultadoSimulacaoFalha **resultado)
{
    return simular_falha(matriz, &operacoes_matriz, vertice_origem,
                         vertice_destino, subestacao, resultado);
}

const char *descricao_status_simulacao(StatusSimulacaoFalha status)
{
    switch (status) {
    case SIMULACAO_SUCESSO: return "simulacao concluida e conexao restaurada";
    case SIMULACAO_ARGUMENTO_INVALIDO: return "argumento invalido";
    case SIMULACAO_SUBESTACAO_NAO_INFORMADA: return "subestacao nao informada";
    case SIMULACAO_VERTICE_INEXISTENTE: return "vertice inexistente";
    case SIMULACAO_ARESTA_INEXISTENTE: return "aresta inexistente ou ja desativada";
    case SIMULACAO_SEM_MEMORIA: return "memoria insuficiente";
    case SIMULACAO_LIMITE_EXCEDIDO: return "limite de tamanho excedido";
    case SIMULACAO_FALHA_RESTAURACAO: return "nao foi possivel restaurar a conexao";
    default: return "status de simulacao desconhecido";
    }
}

static int imprimir_particao(const char *titulo,
                             const ComponentesConexos *componentes, FILE *saida)
{
    size_t inicio = 0;
    if (fprintf(saida, "%s: %zu componente(s)\n", titulo,
                componentes->quantidade_componentes) < 0) {
        return 0;
    }
    for (size_t componente = 0;
         componente < componentes->quantidade_componentes; componente++) {
        size_t tamanho = componentes->tamanhos_componentes[componente];
        if (fprintf(saida, "  Componente %zu: {", componente) < 0) {
            return 0;
        }
        for (size_t posicao = 0; posicao < tamanho; posicao++) {
            if (fprintf(saida, "%s%zu", posicao == 0 ? "" : ", ",
                        componentes->membros[inicio + posicao]) < 0) {
                return 0;
            }
        }
        if (fputs("}\n", saida) == EOF) {
            return 0;
        }
        inicio += tamanho;
    }
    return 1;
}

int imprimir_resultado_simulacao(const ResultadoSimulacaoFalha *resultado,
                                 FILE *saida)
{
    if (resultado == NULL || saida == NULL) {
        return 0;
    }
    if (fprintf(saida,
                "Aresta desativada: %zu -- %zu\n"
                "Subestacao: %zu\n"
                "Componente principal apos falha: %zu (%zu vertices)\n"
                "Vertices fora da regiao principal: %zu\n"
                "Novos vertices isolados pela falha: %zu\n"
                "Houve separacao: %s\n"
                "Topologia restaurada: %s\n",
                resultado->vertice_origem, resultado->vertice_destino,
                resultado->subestacao, resultado->componente_principal_depois,
                resultado->vertices_componente_principal,
                resultado->quantidade_isolados,
                resultado->quantidade_novos_isolados,
                resultado->houve_separacao ? "sim" : "nao",
                resultado->topologia_restaurada ? "sim" : "nao") < 0) {
        return 0;
    }
    return imprimir_particao("Antes", resultado->componentes_antes, saida) &&
           imprimir_particao("Depois", resultado->componentes_depois, saida) &&
           imprimir_particao("Apos restauracao",
                             resultado->componentes_apos_restauracao, saida);
}

static int escrever_vertices_dot(const ResultadoSimulacaoFalha *resultado,
                                 EtapaSimulacao etapa, FILE *saida)
{
    const ComponentesConexos *componentes = etapa == REDE_DEPOIS_FALHA
        ? resultado->componentes_depois : resultado->componentes_antes;
    size_t principal = etapa == REDE_DEPOIS_FALHA
        ? resultado->componente_principal_depois
        : resultado->componente_principal_antes;
    for (size_t vertice = 0; vertice < componentes->quantidade_vertices; vertice++) {
        const char *cor = componentes->componente_por_vertice[vertice] == principal
            ? "palegreen" : "lightcoral";
        const char *forma = vertice == resultado->subestacao
            ? "doublecircle" : "circle";
        if (fprintf(saida, "  %zu [shape=%s, fillcolor=%s];\n",
                    vertice, forma, cor) < 0) {
            return 0;
        }
    }
    return 1;
}

static int exportar_dot(const void *grafo, const OperacoesGrafo *operacoes,
                        const ResultadoSimulacaoFalha *resultado,
                        EtapaSimulacao etapa, FILE *saida)
{
    size_t quantidade_vertices;
    if (grafo == NULL || operacoes == NULL || resultado == NULL || saida == NULL ||
        etapa > REDE_APOS_RESTAURACAO) {
        return 0;
    }
    quantidade_vertices = operacoes->quantidade_vertices(grafo);
    if (quantidade_vertices != resultado->componentes_antes->quantidade_vertices ||
        fputs("graph rede_eletrica {\n"
              "  graph [label=\"Simulacao de falha topologica\", labelloc=t];\n"
              "  node [style=filled];\n", saida) == EOF ||
        !escrever_vertices_dot(resultado, etapa, saida)) {
        return 0;
    }
    for (size_t origem = 0; origem < quantidade_vertices; origem++) {
        for (size_t destino = origem; destino < quantidade_vertices; destino++) {
            int existe = 0;
            int falha = etapa == REDE_DEPOIS_FALHA &&
                ((origem == resultado->vertice_origem &&
                  destino == resultado->vertice_destino) ||
                 (origem == resultado->vertice_destino &&
                  destino == resultado->vertice_origem));
            if (!operacoes->consultar(grafo, origem, destino, &existe)) {
                return 0;
            }
            if (existe && !falha &&
                fprintf(saida, "  %zu -- %zu;\n", origem, destino) < 0) {
                return 0;
            }
        }
    }
    if (etapa == REDE_DEPOIS_FALHA &&
        fprintf(saida, "  %zu -- %zu [color=red, style=dashed, "
                "label=\"falha\"];\n", resultado->vertice_origem,
                resultado->vertice_destino) < 0) {
        return 0;
    }
    return fputs("}\n", saida) != EOF;
}

int exportar_simulacao_dot_lista(const GrafoLista *grafo,
                                 const ResultadoSimulacaoFalha *resultado,
                                 EtapaSimulacao etapa, FILE *saida)
{
    return exportar_dot(grafo, &operacoes_lista, resultado, etapa, saida);
}

int exportar_simulacao_dot_matriz(const MatrizAdjacencia *matriz,
                                  const ResultadoSimulacaoFalha *resultado,
                                  EtapaSimulacao etapa, FILE *saida)
{
    return exportar_dot(matriz, &operacoes_matriz, resultado, etapa, saida);
}
