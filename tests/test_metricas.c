#include "dataset.h"
#include "metricas.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARQUIVO_TEMPORARIO "build/metricas_teste.csv"

static int falhas;

static void verificar(const char *descricao, int condicao)
{
    printf("%s: %s\n", descricao, condicao ? "PASSOU" : "FALHOU");
    falhas += !condicao;
}

static int indice_algoritmo(const char *nome)
{
    const char *nomes[] = {"BFS", "DFS", "Componentes"};
    for (int i = 0; i < 3; i++) {
        if (strcmp(nome, nomes[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static int indice_tamanho(size_t vertices)
{
    const size_t tamanhos[] = {100, 500, 1000};
    for (int i = 0; i < 3; i++) {
        if (vertices == tamanhos[i]) {
            return i;
        }
    }
    return -1;
}

static void testar_registro_invalido(void)
{
    FILE *arquivo = tmpfile();
    verificar("Arquivo temporario para registro", arquivo != NULL);
    if (arquivo == NULL) {
        return;
    }
    verificar("Rejeita tempo negativo",
              !registrar_metrica(arquivo, "BFS", "lista", 1, 0, -0.1, 0, 1));
    verificar("Rejeita tempo NaN",
              !registrar_metrica(arquivo, "BFS", "lista", 1, 0, NAN, 0, 1));
    verificar("Rejeita tempo infinito",
              !registrar_metrica(arquivo, "BFS", "lista", 1, 0, INFINITY, 0, 1));
    fclose(arquivo);
}

static void testar_csv_experimental(void)
{
    Dataset *dataset = NULL;
    ErroDataset erro;
    FILE *arquivo;
    char linha[256];
    int vistos[3][3][2][2] = {{{{0}}}};
    size_t linhas = 0;
    int valido = carregar_dataset("data/forcel_2025_circuito_1/vertices.csv",
                                  "data/forcel_2025_circuito_1/arestas.csv",
                                  &dataset, &erro) == DATASET_SUCESSO;
    verificar("Carrega dataset dos experimentos", valido);
    if (!valido) {
        return;
    }

    verificar("Executa duas repeticoes", executar_experimentos(
        dataset, ARQUIVO_TEMPORARIO, 2));
    liberar_dataset(&dataset);

    arquivo = fopen(ARQUIVO_TEMPORARIO, "r");
    verificar("Abre CSV experimental", arquivo != NULL);
    if (arquivo == NULL) {
        return;
    }
    valido = fgets(linha, sizeof(linha), arquivo) != NULL &&
             strcmp(linha, "algoritmo,representacao,vertices,arestas,tempo_ms,"
                           "memoria_bytes,repeticao\n") == 0;
    verificar("Cabecalho CSV exato", valido);

    valido = 1;
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char algoritmo[32];
        char representacao[16];
        size_t vertices, arestas, memoria, repeticao;
        double tempo_ms;
        int consumidos = 0;
        int a, t, r;
        int p;
        linha[strcspn(linha, "\r\n")] = '\0';
        if (sscanf(linha, "%31[^,],%15[^,],%zu,%zu,%lf,%zu,%zu%n",
                   algoritmo, representacao, &vertices, &arestas, &tempo_ms,
                   &memoria, &repeticao, &consumidos) != 7 ||
            linha[consumidos] != '\0') {
            valido = 0;
            continue;
        }
        a = indice_algoritmo(algoritmo);
        t = indice_tamanho(vertices);
        p = strcmp(representacao, "lista") == 0 ? 0 :
            strcmp(representacao, "matriz") == 0 ? 1 : -1;
        r = repeticao >= 1 && repeticao <= 2 ? (int)repeticao - 1 : -1;
        if (a < 0 || t < 0 || p < 0 || r < 0 || !isfinite(tempo_ms) ||
            tempo_ms < 0.0 || arestas > vertices * vertices || memoria == 0) {
            valido = 0;
            continue;
        }
        vistos[t][a][p][r]++;
        linhas++;
    }
    verificar("Linhas com metadados e tempos validos", valido);
    verificar("Quantidade de linhas", linhas == 36);
    valido = 1;
    for (int t = 0; t < 3; t++) {
        for (int a = 0; a < 3; a++) {
            for (int p = 0; p < 2; p++) {
                for (int r = 0; r < 2; r++) {
                    valido = valido && vistos[t][a][p][r] == 1;
                }
            }
        }
    }
    verificar("Cada repeticao aparece uma vez", valido);
    fclose(arquivo);
    remove(ARQUIVO_TEMPORARIO);
}

int main(void)
{
    testar_registro_invalido();
    testar_csv_experimental();
    printf("Total de falhas: %d\n", falhas);
    return falhas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
