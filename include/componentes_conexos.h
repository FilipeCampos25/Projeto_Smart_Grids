#ifndef COMPONENTES_CONEXOS_H
#define COMPONENTES_CONEXOS_H

#include "grafo_lista.h"
#include "matriz_adjacencia.h"

#include <stdio.h>

typedef enum {
    COMPONENTES_SUCESSO = 0,
    COMPONENTES_ARGUMENTO_INVALIDO,
    COMPONENTES_SEM_MEMORIA,
    COMPONENTES_LIMITE_EXCEDIDO
} StatusComponentes;

/* Particao independente do grafo, usando seus indices internos [0, V).
 * componente_por_vertice tem V entradas, cada uma em [0, quantidade_componentes).
 * tamanhos_componentes tem capacidade V; apenas as primeiras C entradas valem.
 * membros tem V entradas, cada vertice exatamente uma vez, agrupadas por rotulo.
 * Os membros de c comecam na soma dos tamanhos das componentes anteriores.
 * A ordem dos membros/rotulos pode variar: compare particoes, nao numeros de IDs.
 * Os campos sao de leitura; nao altere nem libere os vetores separadamente.
 * As APIs atuais nao possuem atributos para identificar subestacoes. */
typedef struct {
    size_t quantidade_vertices;
    size_t quantidade_componentes;
    size_t *componente_por_vertice;
    size_t *tamanhos_componentes;
    size_t *membros;
} ComponentesConexos;

/* Identifica TODAS as componentes, inclusive vertices isolados, usando a BFS.
 * As representacoes aceitas sao exclusivamente nao direcionadas por contrato;
 * esta API nao calcula componentes fortemente conexas de digrafos.
 * Considera apenas conexoes presentes (ativas); nao altera o grafo, que deve
 * permanecer valido e inalterado durante a chamada. Recalcule apos cada mudanca
 * de topologia; resultados antigos continuam validos como retratos anteriores.
 * resultado deve apontar para NULL. Grafo/saida NULL ou dono ocupado retorna
 * ARGUMENTO_INVALIDO. Retorna SEM_MEMORIA ou LIMITE_EXCEDIDO quando necessario.
 * Todo erro preserva *resultado e libera a construcao parcial.
 * Grafo vazio e sucesso: V=C=0 e todos os vetores NULL.
 * Em sucesso o chamador possui o resultado; use liberar_componentes_conexos,
 * mesmo se o grafo ja tiver sido liberado. Tempo O(V+E), memoria adicional O(V). */
StatusComponentes identificar_componentes_conexos_lista(
    const GrafoLista *grafo, ComponentesConexos **resultado);

/* Mesmo contrato da lista, consultando diretamente a matriz, sem conversao.
 * Tempo O(V^2) para V > 0, memoria adicional O(V). Lacos nao repetem vertices. */
StatusComponentes identificar_componentes_conexos_matriz(
    const MatrizAdjacencia *matriz, ComponentesConexos **resultado);

/* Escreve total, identificador, tamanho e todos os membros de cada componente
 * em saida, alem da indisponibilidade dos atributos de subestacao. Recebe um
 * resultado valido produzido acima e FILE aberto para escrita; nao aloca nem
 * fecha o arquivo. Retorna 1 em sucesso, 0 para NULL ou erro de escrita.
 * Custo O(V+C). O chamador verifica tambem fflush/fclose para erros tardios. */
int imprimir_componentes_conexos(const ComponentesConexos *resultado, FILE *saida);

/* Libera resultado e vetores; define *resultado como NULL. Aceita NULL e
 * repeticao sobre o mesmo dono. Copias antigas dos ponteiros ficam invalidas. */
void liberar_componentes_conexos(ComponentesConexos **resultado);

#endif
