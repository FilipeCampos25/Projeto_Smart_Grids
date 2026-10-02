#include "matriz_adjacencia.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* O Makefile redireciona malloc/calloc/free SOMENTE no objeto do modulo.
 * Este arquivo continua usando os alocadores reais da biblioteca C.
 * Assim podemos falhar cada etapa sem esgotar a memoria do computador.
 */
typedef struct {
    void *endereco;
    size_t bytes;
} RegistroAlocacao;

static RegistroAlocacao registros[4];
static size_t chamadas = 0;
static size_t falhar_na_chamada = 0;
static size_t blocos_ativos = 0;
static size_t bytes_ativos = 0;
static size_t falhas = 0;

/* Compara contagens; acumula falhas para retornar codigo nao zero. */
static void verificar(const char *descricao, size_t esperado, size_t obtido)
{
    printf("%s: esperado=%zu obtido=%zu %s\n", descricao, esperado, obtido,
           esperado == obtido ? "PASSOU" : "FALHOU");
    if (esperado != obtido) {
        ++falhas;
    }
}

/* Registra um bloco real e seus bytes solicitados. Retorna o proprio
 * endereco (ou NULL em falha); o modulo passa a ser dono do bloco.
 * O registro e fixo para nao interferir nas alocacoes sob teste.
 */
static void *registrar_alocacao(void *endereco, size_t bytes)
{
    if (endereco == NULL) {
        return NULL;
    }
    for (size_t indice = 0; indice < 4; ++indice) {
        if (registros[indice].endereco == NULL) {
            registros[indice].endereco = endereco;
            registros[indice].bytes = bytes;
            ++blocos_ativos;
            bytes_ativos += bytes;
            return endereco;
        }
    }
    verificar("Capacidade do registro excedida", 0, 1);
    free(endereco);
    return NULL;
}

/* Substitui malloc do modulo: falha na chamada selecionada ou registra
 * o bloco real de bytes. Quem receber o bloco deve usar teste_liberar.
 */
void *teste_alocar(size_t bytes)
{
    ++chamadas;
    if (chamadas == falhar_na_chamada) {
        return NULL;
    }
    return registrar_alocacao(malloc(bytes), bytes);
}

/* Como teste_alocar, mas preserva a inicializacao zerada de calloc.
 * O modulo verifica previamente o produto; o teste tambem o protege.
 */
void *teste_alocar_zerado(size_t quantidade, size_t tamanho)
{
    ++chamadas;
    if (chamadas == falhar_na_chamada ||
        (tamanho != 0 && quantidade > SIZE_MAX / tamanho)) {
        return NULL;
    }
    return registrar_alocacao(calloc(quantidade, tamanho), quantidade * tamanho);
}

/* Libera um bloco registrado e subtrai seus bytes. NULL e inofensivo;
 * endereco desconhecido ou liberado duas vezes faz o teste falhar.
 */
void teste_liberar(void *endereco)
{
    if (endereco == NULL) {
        return;
    }
    for (size_t indice = 0; indice < 4; ++indice) {
        if (registros[indice].endereco == endereco) {
            --blocos_ativos;
            bytes_ativos -= registros[indice].bytes;
            registros[indice].endereco = NULL;
            registros[indice].bytes = 0;
            free(endereco);
            return;
        }
    }
    verificar("Liberacao desconhecida ou duplicada", 0, 1);
}

/* Injeta falha no cabecalho e nas celulas, exigindo limpeza completa.
 * Uma criacao bem-sucedida em seguida prova que nao ficou estado global
 * de erro no modulo. Nao deixa objetos sob responsabilidade do chamador.
 */
static void testar_falhas_e_recuperacao(void)
{
    size_t estimativa = 0;
    verificar("Estimar pequena", MATRIZ_SUCESSO, estimar_memoria_matriz(6, &estimativa));
    for (size_t etapa = 1; etapa <= 2; ++etapa) {
        MatrizAdjacencia *matriz = NULL;
        chamadas = 0;
        falhar_na_chamada = etapa;
        printf("\nFalha controlada na alocacao %zu\n", etapa);
        verificar("Criacao sem memoria", MATRIZ_SEM_MEMORIA,
                  criar_matriz(6, estimativa, &matriz));
        verificar("Ponteiro de saida NULL", 1, matriz == NULL);
        verificar("Chamadas ate a falha", etapa, chamadas);
        verificar("Blocos restantes apos falha", 0, blocos_ativos);
        verificar("Bytes restantes apos falha", 0, bytes_ativos);
        destruir_matriz(&matriz);
        falhar_na_chamada = 0;
        chamadas = 0;
        verificar("Recuperacao", MATRIZ_SUCESSO, criar_matriz(6, estimativa, &matriz));
        verificar("Alocacoes da matriz", 2, chamadas);
        verificar("Bytes reais solicitados", estimativa, bytes_ativos);
        destruir_matriz(&matriz);
        destruir_matriz(&matriz);
        verificar("Blocos restantes apos recuperacao", 0, blocos_ativos);
        verificar("Bytes restantes apos recuperacao", 0, bytes_ativos);
    }
}

/* Mede as requisicoes reais do modulo para N=0, 1, 6 e 1000, incluindo
 * cabecalho, e verifica que consultas/alteracoes nao alocam auxiliares.
 */
static void testar_contabilidade(void)
{
    const size_t tamanhos[] = {0, 1, 6, 1000};

    for (size_t indice = 0; indice < 4; ++indice) {
        MatrizAdjacencia *matriz = NULL;
        size_t quantidade_vertices = tamanhos[indice];
        size_t estimativa = 0;
        int adjacente = -1;
        chamadas = 0;
        printf("\nContabilidade para %zu vertices\n", quantidade_vertices);
        verificar("Estimativa", MATRIZ_SUCESSO,
                  estimar_memoria_matriz(quantidade_vertices, &estimativa));
        verificar("Criacao", MATRIZ_SUCESSO,
                  criar_matriz(quantidade_vertices, estimativa, &matriz));
        verificar("Estimativa = bytes solicitados", estimativa, bytes_ativos);
        verificar("Memoria informada = bytes solicitados", bytes_ativos, memoria_matriz(matriz));
        if (quantidade_vertices != 0 && matriz != NULL) {
            verificar("Inserir ultimo indice", MATRIZ_SUCESSO,
                      inserir_aresta(matriz, 0, quantidade_vertices - 1));
            verificar("Consultar ultimo indice", MATRIZ_SUCESSO,
                      consultar_adjacencia(matriz, 0, quantidade_vertices - 1, &adjacente));
            verificar("Conexao presente", 1, (size_t)adjacente);
            verificar("Remover ultimo indice", MATRIZ_SUCESSO,
                      remover_aresta(matriz, 0, quantidade_vertices - 1));
        }
        verificar("Total de alocacoes", quantidade_vertices == 0 ? 1 : 2, chamadas);
        destruir_matriz(&matriz);
        verificar("Ponteiro liberado", 1, matriz == NULL);
        verificar("Blocos nao liberados", 0, blocos_ativos);
        verificar("Bytes nao liberados", 0, bytes_ativos);
    }
}

/* Prova que entradas inviaveis sao rejeitadas ANTES de chamar malloc. */
static void testar_rejeicao_sem_alocar(void)
{
    MatrizAdjacencia *matriz = NULL;
    size_t estimativa = 0;
    chamadas = 0;

    puts("\nREJEICOES ANTES DA ALOCACAO");
    verificar("Overflow", MATRIZ_TAMANHO_INVALIDO, criar_matriz(SIZE_MAX, SIZE_MAX, &matriz));
    verificar("Estimar escala", MATRIZ_SUCESSO, estimar_memoria_matriz(1000, &estimativa));
    verificar("Limite excedido", MATRIZ_LIMITE_MEMORIA,
              criar_matriz(1000, estimativa - 1, &matriz));
    verificar("Saida inexistente", MATRIZ_ARGUMENTO_INVALIDO, criar_matriz(6, 1024, NULL));
    verificar("Chamadas ao alocador", 0, chamadas);
    verificar("Ponteiro permanece NULL", 1, matriz == NULL);
    destruir_matriz(&matriz);
}

/* Executa testes de falhas/contabilidade; qualquer divergencia retorna 1. */
int main(void)
{
    testar_falhas_e_recuperacao();
    testar_contabilidade();
    testar_rejeicao_sem_alocar();
    printf("\nTOTAL ALOCACAO: esperado=0 falhas obtido=%zu %s\n", falhas,
           falhas == 0 ? "PASSOU" : "FALHOU");
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
