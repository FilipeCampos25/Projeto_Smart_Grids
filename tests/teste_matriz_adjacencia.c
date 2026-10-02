#include "matriz_adjacencia.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static size_t falhas = 0;

/* Compara valores e registra falhas para o codigo de saida do programa. */
static void verificar(const char *descricao, size_t esperado, size_t obtido)
{
    printf("%s: esperado=%zu obtido=%zu %s\n", descricao, esperado, obtido,
           esperado == obtido ? "PASSOU" : "FALHOU");
    if (esperado != obtido) {
        ++falhas;
    }
}

/* Compara as 36 posicoes e a simetria. Mostra a primeira divergencia
 * com coordenadas; todos os erros participam do resultado agregado.
 */
static void comparar_pequena(const char *etapa, const MatrizAdjacencia *matriz,
                             int esperada[6][6])
{
    size_t divergencias = 0;
    size_t assimetrias = 0;

    for (size_t vertice_origem = 0; vertice_origem < 6; ++vertice_origem) {
        for (size_t vertice_destino = 0; vertice_destino < 6; ++vertice_destino) {
            int obtido = -1;
            int inverso = -1;
            ResultadoMatriz resultado = consultar_adjacencia(
                matriz, vertice_origem, vertice_destino, &obtido);
            ResultadoMatriz resultado_inverso = consultar_adjacencia(
                matriz, vertice_destino, vertice_origem, &inverso);
            if (resultado != MATRIZ_SUCESSO ||
                obtido != esperada[vertice_origem][vertice_destino]) {
                if (divergencias == 0) {
                    printf("FALHOU [%zu][%zu]: esperado=%d obtido=%d status=%d\n",
                           vertice_origem, vertice_destino,
                           esperada[vertice_origem][vertice_destino], obtido, resultado);
                }
                ++divergencias;
            }
            if (resultado != MATRIZ_SUCESSO || resultado_inverso != MATRIZ_SUCESSO ||
                obtido != inverso) {
                ++assimetrias;
            }
        }
    }
    printf("\n%s (36 posicoes)\n", etapa);
    verificar("Divergencias", 0, divergencias);
    verificar("Assimetrias", 0, assimetrias);
}

/* Imprime rotulos, celulas e memoria da matriz pequena ja validada. */
static void imprimir_pequena(const MatrizAdjacencia *matriz)
{
    puts("\nMatriz pequena: linhas=origem, colunas=destino (1=conexao)");
    puts("     0 1 2 3 4 5");
    for (size_t vertice_origem = 0; vertice_origem < 6; ++vertice_origem) {
        printf("%zu | ", vertice_origem);
        for (size_t vertice_destino = 0; vertice_destino < 6; ++vertice_destino) {
            int adjacente = -1;
            consultar_adjacencia(matriz, vertice_origem, vertice_destino, &adjacente);
            printf("%d ", adjacente);
        }
        putchar('\n');
    }
    printf("Memoria da estrutura pequena: %zu bytes\n", memoria_matriz(matriz));
}

/* Executa o exemplo exigido, isolado, duplicatas, remocao e restauracao.
 * A matriz local e liberada antes do retorno, inclusive em erro de criacao.
 */
static void testar_pequena(void)
{
    MatrizAdjacencia *matriz = NULL;
    const size_t arestas[5][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {3, 4}};
    int vazia[6][6] = {{0}};
    int esperada[6][6] = {
        {0, 1, 1, 0, 0, 0},
        {1, 0, 0, 1, 0, 0},
        {1, 0, 0, 1, 0, 0},
        {0, 1, 1, 0, 1, 0},
        {0, 0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 0}
    };
    size_t cabecalho = 0;
    size_t estimativa = 0;

    puts("\nTESTE PEQUENO SINTETICO: 6 vertices, 5 arestas");
    verificar("Estimar cabecalho", MATRIZ_SUCESSO, estimar_memoria_matriz(0, &cabecalho));
    verificar("Estimar pequena", MATRIZ_SUCESSO, estimar_memoria_matriz(6, &estimativa));
    verificar("Criar pequena", MATRIZ_SUCESSO, criar_matriz(6, estimativa, &matriz));
    if (matriz == NULL) {
        return;
    }
    verificar("Vertices", 6, quantidade_vertices_matriz(matriz));
    verificar("Bytes pequena", cabecalho + 36 * sizeof(unsigned char), memoria_matriz(matriz));
    comparar_pequena("Inicializacao sem conexoes", matriz, vazia);
    for (size_t indice_aresta = 0; indice_aresta < 5; ++indice_aresta) {
        verificar("Insercao pequena", MATRIZ_SUCESSO,
                  inserir_aresta(matriz, arestas[indice_aresta][0], arestas[indice_aresta][1]));
    }
    comparar_pequena("Cinco arestas; diagonal e vertice 5 sem conexoes", matriz, esperada);
    imprimir_pequena(matriz);
    verificar("Duplicata", MATRIZ_SUCESSO, inserir_aresta(matriz, 0, 1));
    verificar("Duplicata invertida", MATRIZ_SUCESSO, inserir_aresta(matriz, 1, 0));
    comparar_pequena("Duplicatas preservam o grafo", matriz, esperada);
    verificar("Remover (3,4)", MATRIZ_SUCESSO, remover_aresta(matriz, 3, 4));
    verificar("Repetir remocao invertida", MATRIZ_SUCESSO, remover_aresta(matriz, 4, 3));
    esperada[3][4] = esperada[4][3] = 0;
    comparar_pequena("Remocao nos dois sentidos preserva demais posicoes", matriz, esperada);
    verificar("Restaurar (4,3)", MATRIZ_SUCESSO, inserir_aresta(matriz, 4, 3));
    esperada[3][4] = esperada[4][3] = 1;
    comparar_pequena("Restauracao da matriz inicial", matriz, esperada);
    destruir_matriz(&matriz);
    verificar("Destruir zera ponteiro", 1, matriz == NULL);
    destruir_matriz(&matriz);
}

/* Compara cada celula da cadeia com o padrao matematico esperado,
 * inclusive nao vizinhos, diagonal e direcao inversa. Nao aloca memoria.
 */
static void comparar_cadeia(const char *etapa, const MatrizAdjacencia *matriz,
                           int populada, int interrompida)
{
    size_t divergencias = 0;
    size_t conexoes = 0;

    for (size_t vertice_origem = 0; vertice_origem < 1000; ++vertice_origem) {
        for (size_t vertice_destino = 0; vertice_destino < 1000; ++vertice_destino) {
            int esperado = populada && (vertice_origem + 1 == vertice_destino ||
                                       vertice_destino + 1 == vertice_origem);
            int obtido = -1;
            if (interrompida && ((vertice_origem == 499 && vertice_destino == 500) ||
                                 (vertice_origem == 500 && vertice_destino == 499))) {
                esperado = 0;
            }
            ResultadoMatriz resultado = consultar_adjacencia(
                matriz, vertice_origem, vertice_destino, &obtido);
            if (resultado != MATRIZ_SUCESSO || obtido != esperado) {
                if (divergencias == 0) {
                    printf("FALHOU [%zu][%zu]: esperado=%d obtido=%d status=%d\n",
                           vertice_origem, vertice_destino, esperado, obtido, resultado);
                }
                ++divergencias;
            }
            if (obtido == 1) {
                ++conexoes;
            }
        }
    }
    printf("\n%s (1000000 posicoes)\n", etapa);
    verificar("Divergencias", 0, divergencias);
    verificar("Celulas conectadas", populada ? (interrompida ? 1996 : 1998) : 0, conexoes);
}

/* Valida escala sintetica de 1000 vertices com teto explicito de 2 MiB.
 * Agrega as 999 insercoes para nao esconder divergencias em excesso de log.
 */
static void testar_escala(void)
{
    MatrizAdjacencia *matriz = NULL;
    size_t cabecalho = 0;
    size_t estimativa = 0;
    size_t insercoes_corretas = 0;

    puts("\nTESTE DE ESCALA SINTETICO: 1000 vertices, cadeia com 999 arestas");
    verificar("Estimar cabecalho", MATRIZ_SUCESSO, estimar_memoria_matriz(0, &cabecalho));
    verificar("Estimar escala", MATRIZ_SUCESSO, estimar_memoria_matriz(1000, &estimativa));
    verificar("Estimativa escala", cabecalho + 1000000 * sizeof(unsigned char), estimativa);
    verificar("Criar escala", MATRIZ_SUCESSO, criar_matriz(1000, 2 * 1024 * 1024, &matriz));
    if (matriz == NULL) {
        return;
    }
    verificar("Vertices escala", 1000, quantidade_vertices_matriz(matriz));
    verificar("Bytes escala", estimativa, memoria_matriz(matriz));
    comparar_cadeia("Cadeia antes das insercoes", matriz, 0, 0);
    for (size_t vertice_origem = 0; vertice_origem < 999; ++vertice_origem) {
        if (inserir_aresta(matriz, vertice_origem, vertice_origem + 1) == MATRIZ_SUCESSO) {
            ++insercoes_corretas;
        }
    }
    verificar("Insercoes da cadeia", 999, insercoes_corretas);
    comparar_cadeia("Cadeia completa e simetrica", matriz, 1, 0);
    verificar("Remover centro", MATRIZ_SUCESSO, remover_aresta(matriz, 499, 500));
    comparar_cadeia("Cadeia interrompida", matriz, 1, 1);
    verificar("Restaurar centro", MATRIZ_SUCESSO, inserir_aresta(matriz, 500, 499));
    comparar_cadeia("Cadeia restaurada", matriz, 1, 0);
    verificar("Bytes apos alteracoes", estimativa, memoria_matriz(matriz));
    destruir_matriz(&matriz);
    verificar("Destruir escala", 1, matriz == NULL);
}

/* Exercita limites, NULL, overflow e teto de memoria sem tentar alocacoes
 * enormes. Garante que erros preservem a saida e a matriz ja existente.
 */
static void testar_entradas_invalidas(void)
{
    MatrizAdjacencia *matriz = NULL;
    size_t estimativa = 0;
    size_t bytes_preservados = 123;
    int adjacente = 7;

    puts("\nENTRADAS INVALIDAS E LIMITES");
    verificar("Estimativa com saida NULL", MATRIZ_ARGUMENTO_INVALIDO,
              estimar_memoria_matriz(6, NULL));
    verificar("Overflow na estimativa", MATRIZ_TAMANHO_INVALIDO,
              estimar_memoria_matriz(SIZE_MAX, &bytes_preservados));
    verificar("Erro preserva bytes", 123, bytes_preservados);
    verificar("Overflow na criacao", MATRIZ_TAMANHO_INVALIDO,
              criar_matriz(SIZE_MAX, SIZE_MAX, &matriz));
    verificar("Overflow nao entrega matriz", 1, matriz == NULL);
    verificar("Criacao sem saida", MATRIZ_ARGUMENTO_INVALIDO, criar_matriz(6, 1024, NULL));
    verificar("Estimar limite", MATRIZ_SUCESSO, estimar_memoria_matriz(6, &estimativa));
    verificar("Teto insuficiente", MATRIZ_LIMITE_MEMORIA,
              criar_matriz(6, estimativa - 1, &matriz));
    verificar("Teto nao entrega matriz", 1, matriz == NULL);
    verificar("Teto zero", MATRIZ_LIMITE_MEMORIA, criar_matriz(0, 0, &matriz));
    verificar("Insercao NULL", MATRIZ_ARGUMENTO_INVALIDO, inserir_aresta(NULL, 0, 1));
    verificar("Remocao NULL", MATRIZ_ARGUMENTO_INVALIDO, remover_aresta(NULL, 0, 1));
    verificar("Consulta NULL", MATRIZ_ARGUMENTO_INVALIDO, consultar_adjacencia(NULL, 0, 1, &adjacente));
    verificar("Consulta invalida preserva saida", 7, (size_t)adjacente);
    verificar("Memoria NULL", 0, memoria_matriz(NULL));
    verificar("Quantidade NULL", 0, quantidade_vertices_matriz(NULL));
    destruir_matriz(NULL);
    verificar("Criar para limites", MATRIZ_SUCESSO, criar_matriz(6, estimativa, &matriz));
    if (matriz == NULL) {
        return;
    }
    verificar("Nao sobrescrever dono", MATRIZ_ARGUMENTO_INVALIDO, criar_matriz(6, estimativa, &matriz));
    verificar("Insercao valida preservada", MATRIZ_SUCESSO, inserir_aresta(matriz, 0, 1));
    const size_t indices_invalidos[] = {6, SIZE_MAX};
    for (size_t indice = 0; indice < 2; ++indice) {
        size_t invalido = indices_invalidos[indice];
        verificar("Origem invalida na insercao", MATRIZ_ARGUMENTO_INVALIDO, inserir_aresta(matriz, invalido, 1));
        verificar("Destino invalido na insercao", MATRIZ_ARGUMENTO_INVALIDO, inserir_aresta(matriz, 0, invalido));
        verificar("Origem invalida na remocao", MATRIZ_ARGUMENTO_INVALIDO, remover_aresta(matriz, invalido, 1));
        verificar("Destino invalido na remocao", MATRIZ_ARGUMENTO_INVALIDO, remover_aresta(matriz, 0, invalido));
        verificar("Origem invalida na consulta", MATRIZ_ARGUMENTO_INVALIDO, consultar_adjacencia(matriz, invalido, 1, &adjacente));
        verificar("Destino invalido na consulta", MATRIZ_ARGUMENTO_INVALIDO, consultar_adjacencia(matriz, 0, invalido, &adjacente));
        verificar("Erro preserva saida", 7, (size_t)adjacente);
    }
    verificar("Consulta sem saida", MATRIZ_ARGUMENTO_INVALIDO, consultar_adjacencia(matriz, 0, 1, NULL));
    int esperada[6][6] = {{0, 1, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0}};
    comparar_pequena("Erros preservam todas as celulas", matriz, esperada);
    destruir_matriz(&matriz);
}

/* Verifica N=0, N=1 e a politica provisoria de lacos. Libera os objetos. */
static void testar_vazia_e_laco(void)
{
    MatrizAdjacencia *matriz = NULL;
    int adjacente = 7;
    size_t cabecalho = 0;

    puts("\nMATRIZ VAZIA E LACO");
    verificar("Estimar vazia", MATRIZ_SUCESSO, estimar_memoria_matriz(0, &cabecalho));
    verificar("Criar vazia", MATRIZ_SUCESSO, criar_matriz(0, cabecalho, &matriz));
    if (matriz != NULL) {
        verificar("Vertices vazia", 0, quantidade_vertices_matriz(matriz));
        verificar("Memoria vazia", cabecalho, memoria_matriz(matriz));
        verificar("Inserir na vazia", MATRIZ_ARGUMENTO_INVALIDO, inserir_aresta(matriz, 0, 0));
        verificar("Remover na vazia", MATRIZ_ARGUMENTO_INVALIDO, remover_aresta(matriz, 0, 0));
        verificar("Consultar vazia", MATRIZ_ARGUMENTO_INVALIDO, consultar_adjacencia(matriz, 0, 0, &adjacente));
        verificar("Vazia preserva saida", 7, (size_t)adjacente);
        destruir_matriz(&matriz);
    }
    verificar("Criar unitaria", MATRIZ_SUCESSO, criar_matriz(1, cabecalho + 1, &matriz));
    if (matriz != NULL) {
        verificar("Consultar diagonal vazia", MATRIZ_SUCESSO, consultar_adjacencia(matriz, 0, 0, &adjacente));
        verificar("Diagonal inicia vazia", 0, (size_t)adjacente);
        verificar("Inserir laco", MATRIZ_SUCESSO, inserir_aresta(matriz, 0, 0));
        verificar("Repetir laco", MATRIZ_SUCESSO, inserir_aresta(matriz, 0, 0));
        verificar("Consultar laco", MATRIZ_SUCESSO, consultar_adjacencia(matriz, 0, 0, &adjacente));
        verificar("Laco presente", 1, (size_t)adjacente);
        verificar("Remover laco", MATRIZ_SUCESSO, remover_aresta(matriz, 0, 0));
        verificar("Consultar laco removido", MATRIZ_SUCESSO, consultar_adjacencia(matriz, 0, 0, &adjacente));
        verificar("Laco ausente", 0, (size_t)adjacente);
        destruir_matriz(&matriz);
    }
}

/* Executa a suite sintetica. O resultado nao inclui integracao com BDGD. */
int main(void)
{
    testar_pequena();
    testar_escala();
    testar_entradas_invalidas();
    testar_vazia_e_laco();
    printf("\nTOTAL SINTETICO: esperado=0 falhas obtido=%zu %s\n", falhas,
           falhas == 0 ? "PASSOU" : "FALHOU");
    puts("DATASET REAL: PENDENTE (modelo, carregador e arquivos ausentes).");
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
