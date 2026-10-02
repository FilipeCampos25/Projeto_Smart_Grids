#ifndef SIMULACAO_FALHA_H
#define SIMULACAO_FALHA_H

#include "componentes_conexos.h"

#include <stdio.h>

typedef enum {
    SIMULACAO_SUCESSO = 0,
    SIMULACAO_ARGUMENTO_INVALIDO,
    SIMULACAO_SUBESTACAO_NAO_INFORMADA,
    SIMULACAO_VERTICE_INEXISTENTE,
    SIMULACAO_ARESTA_INEXISTENTE,
    SIMULACAO_SEM_MEMORIA,
    SIMULACAO_LIMITE_EXCEDIDO,
    SIMULACAO_FALHA_RESTAURACAO
} StatusSimulacaoFalha;

/* Retrato completo da contingencia. As tres particoes sao independentes do
 * grafo e pertencem ao resultado. "Ilha" significa somente componente
 * topologica fora daquela que contem a subestacao; nao indica capacidade de
 * geracao, demanda ou autossuficiencia eletrica. */
typedef struct {
    size_t vertice_origem;
    size_t vertice_destino;
    size_t subestacao;
    ComponentesConexos *componentes_antes;
    ComponentesConexos *componentes_depois;
    ComponentesConexos *componentes_apos_restauracao;
    size_t componente_principal_antes;
    size_t componente_principal_depois;
    size_t vertices_componente_principal;
    size_t quantidade_isolados;
    size_t quantidade_novos_isolados;
    int houve_separacao;
    int topologia_restaurada;
} ResultadoSimulacaoFalha;

/* Simula a falha de uma conexao existente, recalcula os componentes e restaura
 * a aresta antes de retornar. subestacao == SIZE_MAX representa informacao
 * ausente. resultado deve apontar para NULL e passa a possuir os tres retratos.
 * Uma aresta removida/desativada e informada como ARESTA_INEXISTENTE.
 * Se a reinsercao falhar na lista por falta de memoria, retorna
 * FALHA_RESTAURACAO e o chamador deve considerar a conexao ainda ausente. */
StatusSimulacaoFalha simular_falha_aresta_lista(
    GrafoLista *grafo, size_t vertice_origem, size_t vertice_destino,
    size_t subestacao, ResultadoSimulacaoFalha **resultado);

/* Mesmo contrato, usando diretamente a matriz de adjacencia. */
StatusSimulacaoFalha simular_falha_aresta_matriz(
    MatrizAdjacencia *matriz, size_t vertice_origem, size_t vertice_destino,
    size_t subestacao, ResultadoSimulacaoFalha **resultado);

/* Registra aresta, contagens e membros dos componentes antes/depois/restaurados.
 * Nao fecha o FILE. Retorna 1 em sucesso e 0 para argumento/erro de escrita. */
int imprimir_resultado_simulacao(const ResultadoSimulacaoFalha *resultado,
                                 FILE *saida);

typedef enum {
    REDE_ANTES_FALHA,
    REDE_DEPOIS_FALHA,
    REDE_APOS_RESTAURACAO
} EtapaSimulacao;

/* Gera somente texto Graphviz DOT. O programa dot nao e dependencia.
 * A etapa posterior mostra a conexao falha tracejada, a componente principal
 * em verde e as ilhas topologicas em vermelho. Nao modifica o grafo. */
int exportar_simulacao_dot_lista(const GrafoLista *grafo,
                                 const ResultadoSimulacaoFalha *resultado,
                                 EtapaSimulacao etapa, FILE *saida);
int exportar_simulacao_dot_matriz(const MatrizAdjacencia *matriz,
                                  const ResultadoSimulacaoFalha *resultado,
                                  EtapaSimulacao etapa, FILE *saida);

/* Mensagem curta e compreensivel para exibicao dos retornos da API. */
const char *descricao_status_simulacao(StatusSimulacaoFalha status);

/* Libera os tres retratos e o resultado; aceita NULL e repeticao. */
void liberar_resultado_simulacao(ResultadoSimulacaoFalha **resultado);

#endif
