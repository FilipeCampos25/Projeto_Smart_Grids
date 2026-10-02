#include "componentes_conexos.h"
#include "bfs_interno.h"

#include <stdlib.h>

void liberar_componentes_conexos(ComponentesConexos **resultado)
{
    if (resultado != NULL && *resultado != NULL) {
        free((*resultado)->componente_por_vertice);
        free((*resultado)->tamanhos_componentes);
        free((*resultado)->membros);
        free(*resultado);
        *resultado = NULL;
    }
}

/* Reserva cabecalho e vetores de rotulos/tamanhos; membros sera recebido da
 * BFS. Em erro libera a construcao parcial e preserva a saida. Em sucesso,
 * o chamador interno deve liberar ou transferir a posse do resultado. */
static StatusComponentes criar_componentes(size_t quantidade_vertices,
                                           ComponentesConexos **resultado)
{
    ComponentesConexos *novo_resultado;
    if (quantidade_vertices > SIZE_MAX / sizeof(size_t)) {
        return COMPONENTES_LIMITE_EXCEDIDO;
    }
    novo_resultado = malloc(sizeof(*novo_resultado));
    if (novo_resultado == NULL) {
        return COMPONENTES_SEM_MEMORIA;
    }
    *novo_resultado = (ComponentesConexos){0};
    novo_resultado->quantidade_vertices = quantidade_vertices;
    if (quantidade_vertices != 0) {
        novo_resultado->componente_por_vertice = malloc(quantidade_vertices * sizeof(size_t));
        if (novo_resultado->componente_por_vertice == NULL) {
            liberar_componentes_conexos(&novo_resultado);
            return COMPONENTES_SEM_MEMORIA;
        }
        novo_resultado->tamanhos_componentes = malloc(quantidade_vertices * sizeof(size_t));
        if (novo_resultado->tamanhos_componentes == NULL) {
            liberar_componentes_conexos(&novo_resultado);
            return COMPONENTES_SEM_MEMORIA;
        }
    }
    *resultado = novo_resultado;
    return COMPONENTES_SUCESSO;
}

/* Traduz erros da dependencia sem acoplar os valores numericos dos enums. */
static StatusComponentes converter_status_bfs(StatusBfs status)
{
    switch (status) {
    case BFS_SUCESSO: return COMPONENTES_SUCESSO;
    case BFS_SEM_MEMORIA: return COMPONENTES_SEM_MEMORIA;
    case BFS_LIMITE_EXCEDIDO: return COMPONENTES_LIMITE_EXCEDIDO;
    default: return COMPONENTES_ARGUMENTO_INVALIDO;
    }
}

/* Orquestra a mesma BFS nas duas representacoes, sem duplicar a travessia.
 * Os wrappers garantem exatamente um grafo nao NULL e saida livre.
 * Vem da BFS tanto a marcacao global quanto a fila, reservadas uma unica vez.
 * Em erro, libera todos os temporarios; em sucesso transfere sua propriedade. */
static StatusComponentes identificar_componentes(const GrafoLista *grafo,
                                                 const MatrizAdjacencia *matriz,
                                                 ComponentesConexos **resultado)
{
    size_t quantidade_vertices = grafo != NULL ? quantidade_vertices_lista(grafo)
                                               : quantidade_vertices_matriz(matriz);
    ComponentesConexos *componentes = NULL;
    ResultadoBfs *busca = NULL;
    StatusComponentes status = criar_componentes(quantidade_vertices, &componentes);
    if (status != COMPONENTES_SUCESSO) {
        return status;
    }
    if (quantidade_vertices != 0) {
        status = converter_status_bfs(criar_busca_bfs(quantidade_vertices, &busca));
        if (status != COMPONENTES_SUCESSO) {
            liberar_componentes_conexos(&componentes);
            return status;
        }
    }
    for (size_t origem = 0; origem < quantidade_vertices; origem++) {
        size_t inicio_componente;
        size_t identificador = componentes->quantidade_componentes;
        if (busca->distancias[origem] != SIZE_MAX) {
            continue;
        }
        inicio_componente = busca->quantidade_alcancados;
        status = converter_status_bfs(grafo != NULL
            ? continuar_bfs_lista(grafo, origem, busca)
            : continuar_bfs_matriz(matriz, origem, busca));
        if (status != COMPONENTES_SUCESSO) {
            liberar_resultado_bfs(&busca);
            liberar_componentes_conexos(&componentes);
            return status;
        }
        /* Somente as novas descobertas recebem este rotulo. A BFS marca antes
         * de enfileirar, logo cada vertice recebe exatamente uma atribuicao. */
        for (size_t posicao = inicio_componente; posicao < busca->quantidade_alcancados; posicao++) {
            size_t vertice = busca->ordem_visita[posicao];
            componentes->componente_por_vertice[vertice] = identificador;
        }
        componentes->tamanhos_componentes[identificador] =
            busca->quantidade_alcancados - inicio_componente;
        componentes->quantidade_componentes++;
    }
    if (busca != NULL) {
        /* Transfere a ordem, ja agrupada por componente, sem copiar V indices. */
        componentes->membros = busca->ordem_visita;
        busca->ordem_visita = NULL;
        liberar_resultado_bfs(&busca);
    }
    *resultado = componentes;
    return COMPONENTES_SUCESSO;
}

StatusComponentes identificar_componentes_conexos_lista(
    const GrafoLista *grafo, ComponentesConexos **resultado)
{
    if (grafo == NULL || resultado == NULL || *resultado != NULL) {
        return COMPONENTES_ARGUMENTO_INVALIDO;
    }
    return identificar_componentes(grafo, NULL, resultado);
}

StatusComponentes identificar_componentes_conexos_matriz(
    const MatrizAdjacencia *matriz, ComponentesConexos **resultado)
{
    if (matriz == NULL || resultado == NULL || *resultado != NULL) {
        return COMPONENTES_ARGUMENTO_INVALIDO;
    }
    return identificar_componentes(NULL, matriz, resultado);
}

int imprimir_componentes_conexos(const ComponentesConexos *resultado, FILE *saida)
{
    size_t inicio = 0;
    if (resultado == NULL || saida == NULL) {
        return 0;
    }
    if (fprintf(saida, "Total de componentes: %zu; vertices: %zu\n"
                "Subestacao: indisponivel (representacoes sem atributos).\n",
                resultado->quantidade_componentes, resultado->quantidade_vertices) < 0) {
        return 0;
    }
    for (size_t componente = 0; componente < resultado->quantidade_componentes; componente++) {
        size_t tamanho = resultado->tamanhos_componentes[componente];
        if (fprintf(saida, "Componente %zu: tamanho=%zu; membros={", componente, tamanho) < 0) {
            return 0;
        }
        for (size_t posicao = 0; posicao < tamanho; posicao++) {
            if (fprintf(saida, "%s%zu", posicao == 0 ? "" : ",",
                        resultado->membros[inicio + posicao]) < 0) {
                return 0;
            }
        }
        if (fprintf(saida, "}\n") < 0) {
            return 0;
        }
        inicio += tamanho;
    }
    return 1;
}
