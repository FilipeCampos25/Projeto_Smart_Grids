#include "benchmark_alocacao.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void *ponteiro;
    size_t tamanho;
} RegistroAlocacao;

enum { CAPACIDADE_REGISTROS = 32 };

static RegistroAlocacao registros[CAPACIDADE_REGISTROS];
static size_t bytes_ativos;
static size_t bytes_pico;

void *benchmark_malloc(size_t tamanho)
{
    void *ponteiro = malloc(tamanho);
    if (ponteiro == NULL) {
        return NULL;
    }
    for (size_t indice = 0; indice < CAPACIDADE_REGISTROS; indice++) {
        if (registros[indice].ponteiro == NULL) {
            if (tamanho > SIZE_MAX - bytes_ativos) {
                free(ponteiro);
                return NULL;
            }
            registros[indice].ponteiro = ponteiro;
            registros[indice].tamanho = tamanho;
            bytes_ativos += tamanho;
            if (bytes_ativos > bytes_pico) {
                bytes_pico = bytes_ativos;
            }
            return ponteiro;
        }
    }
    free(ponteiro);
    return NULL;
}

void *benchmark_calloc(size_t quantidade, size_t tamanho)
{
    size_t total;
    void *ponteiro;
    if (tamanho != 0 && quantidade > SIZE_MAX / tamanho) {
        return NULL;
    }
    total = quantidade * tamanho;
    ponteiro = benchmark_malloc(total);
    if (ponteiro != NULL) {
        memset(ponteiro, 0, total);
    }
    return ponteiro;
}

void benchmark_free(void *ponteiro)
{
    if (ponteiro == NULL) {
        return;
    }
    for (size_t indice = 0; indice < CAPACIDADE_REGISTROS; indice++) {
        if (registros[indice].ponteiro == ponteiro) {
            bytes_ativos -= registros[indice].tamanho;
            registros[indice] = (RegistroAlocacao){NULL, 0};
            free(ponteiro);
            return;
        }
    }
}

int benchmark_alocacao_reiniciar(void)
{
    if (bytes_ativos != 0) {
        return 0;
    }
    bytes_pico = 0;
    return 1;
}

size_t benchmark_alocacao_pico(void)
{
    return bytes_pico;
}

size_t benchmark_alocacao_ativa(void)
{
    return bytes_ativos;
}
