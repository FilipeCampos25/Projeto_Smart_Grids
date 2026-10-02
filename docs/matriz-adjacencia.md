# Matriz de Adjacência — F1-05 / issue #6

## Estado e dependências

Inspeção em **01/10/2026**, no fuso America/Sao_Paulo:

- Remoto `FilipeCampos25/Projeto_Smart_Grids`: apenas `main`, commit
  `2e0eba72b3b19a4911c6cb9de2838f47093118bd`; nenhum PR aberto.
- O checkout inicial estava limpo, na branch local
  `feat/issue-6-matriz-adjacencia`, no mesmo commit.
- Havia README, `.gitignore` e seis documentos vazios. Não havia C,
  headers, build, testes, dependências de código, parser, dataset ou `AGENTS.md`.
- Foram lidas integralmente as issues [#3 (modelo)](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/3),
  [#4 (carregador)](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/4),
  [#5 (lista)](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/5) e
  [#6 (matriz)](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/6).
  Todas estavam abertas, sem comentários, confirmado nos endpoints de comentários.

**O módulo e os testes sintéticos estão implementados e executados. A integração
com o dataset continua pendente.** A #3 contém propostas e decisões ainda abertas;
a #4 sugere CSV/TXT, sem contrato C ou arquivos disponíveis; a #5 não tem API
implementada. Este trabalho não implementa essas dependências nem define o modelo
elétrico definitivo. `docs/modelagem-grafo.md` permanece sob responsabilidade da #3.

## Representação e decisões provisórias

`MatrizAdjacencia` é uma estrutura opaca: o header declara seu nome e as funções,
mas seus campos ficam no `.c`. Isso evita que um chamador altere somente uma metade
da matriz e quebre a simetria. Ela contém a quantidade de vértices e um ponteiro
para um bloco de `unsigned char` com `N*N` células.

Cada célula vale **0** (sem conexão) ou **1** (com conexão). As linhas são armazenadas
em sequência: `[origem][destino]` está na posição `origem*N + destino`. Não há vetor
de ponteiros para linhas, atributos, contadores por aresta ou dependência da lista.

| Aspecto | Contrato deste módulo, sujeito à compatibilização com #3/#4/#5 |
| --- | --- |
| IDs | Índices densos `size_t` de `0` até `N-1`; não são IDs brutos da BDGD |
| Número de vértices | Fixo após a criação; `N=0` é válido e não tem índices consultáveis |
| Direção/pesos | Não direcionado e não ponderado, para conectividade na Fase I |
| Duplicatas | Inserções repetidas, inclusive invertidas, mantêm uma conexão binária |
| Laços | Aceitos explicitamente na diagonal; ausentes até serem inseridos |
| Isolados | Preservados como linhas/colunas zeradas, como o vértice 5 do exemplo |
| Falha de conexão | Remoção do par em ambos os sentidos; restauração por reinserção |
| Conexão já ausente | Remoção retorna sucesso e não altera as demais células |
| Erro de entrada | Não altera a matriz nem a saída de uma consulta/estimativa |

A política de laços foi escolhida para não descartar silenciosamente uma conexão
recebida. A definição definitiva continua pendente na #3. A matriz guarda apenas
conectividade entre pares: **não distingue ausência original de desativação e não
identifica segmentos paralelos individualmente**. O chamador deve guardar o que
pretende restaurar. Se dois segmentos ligarem o mesmo par, remover esse par elimina
sua adjacência inteira; a futura integração deve resolver o estado agregado dos
segmentos conforme a modelagem, antes de atualizar a matriz.

## Funções e uso

O contrato completo de parâmetros, retornos, casos especiais e propriedade está
em [`include/matriz_adjacencia.h`](../include/matriz_adjacencia.h).

| Função | Objetivo |
| --- | --- |
| `estimar_memoria_matriz(N, &bytes)` | Calcula o tamanho sem alocar, verificando overflow |
| `criar_matriz(N, limite_bytes, &matriz)` | Cria a estrutura zerada respeitando um teto explícito |
| `inserir_aresta(matriz, origem, destino)` | Grava 1 nos dois sentidos |
| `consultar_adjacencia(matriz, origem, destino, &adjacente)` | Retorna status e escreve 0 ou 1 separadamente |
| `remover_aresta(matriz, origem, destino)` | Grava 0 nos dois sentidos |
| `quantidade_vertices_matriz(matriz)` | Informa o limite para percorrer os vértices |
| `memoria_matriz(matriz)` | Informa os bytes solicitados pela estrutura existente |
| `destruir_matriz(&matriz)` | Libera células e cabeçalho e zera o ponteiro do dono |

`ResultadoMatriz` diferencia sucesso (0), argumento inválido (1), tamanho não
representável (2), limite de memória excedido (3) e falta de memória no alocador (4).
Não confunda uma consulta válida que escreve `0` com um erro de consulta.

Exemplo de uso, com todas as saídas de erro liberando a estrutura:

```c
#include "matriz_adjacencia.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    MatrizAdjacencia *matriz = NULL;
    const size_t limite_bytes = 2 * 1024 * 1024;
    int adjacente;

    if (criar_matriz(6, limite_bytes, &matriz) != MATRIZ_SUCESSO) {
        return EXIT_FAILURE;
    }
    if (inserir_aresta(matriz, 0, 1) != MATRIZ_SUCESSO) {
        destruir_matriz(&matriz);
        return EXIT_FAILURE;
    }
    /* Enumeracao de vizinhos utilizavel por BFS/DFS, sem implementa-los. */
    for (size_t vertice_destino = 0;
         vertice_destino < quantidade_vertices_matriz(matriz);
         ++vertice_destino) {
        if (consultar_adjacencia(matriz, 0, vertice_destino, &adjacente)
            != MATRIZ_SUCESSO) {
            destruir_matriz(&matriz);
            return EXIT_FAILURE;
        }
        if (adjacente) {
            printf("Vizinho de 0: %zu\n", vertice_destino);
        }
    }
    destruir_matriz(&matriz);
    return EXIT_SUCCESS;
}
```

O ponteiro passado à criação deve começar em `NULL`. Passar um dono de uma matriz
existente é erro: a função não sobrescreve esse ponteiro nem vaza o objeto anterior.
Após destruição, cópias do ponteiro antigo não podem mais ser usadas. Chamadas
repetidas de destruição sobre o mesmo dono, já zerado, são seguras.

Não há alocação em consultas, inserções ou remoções. Criar e zerar custa `O(N²)`;
cada acesso/alteração custa `O(1)`; enumerar os vizinhos de um vértice custa `O(N)`.
O módulo apenas libera seus dois blocos ao destruir, sem percorrer as células.

## Memória e segurança de tamanhos

A fórmula é:

```text
bytes = sizeof(MatrizAdjacencia) + N*N*sizeof(unsigned char)
```

O cabeçalho inclui o contador, o ponteiro e eventual padding da estrutura.
`sizeof(unsigned char)` vale 1 byte C. No ambiente Windows x64 testado, o cabeçalho
ocupa 16 bytes e cada byte tem 8 bits:

| Vértices | Células | Cabeçalho | Total solicitado |
| ---: | ---: | ---: | ---: |
| 0 | 0 | 16 | 16 bytes |
| 1 | 1 | 16 | 17 bytes |
| 6 | 36 | 16 | 52 bytes |
| 1.000 | 1.000.000 | 16 | 1.000.016 bytes (aprox. 0,954 MiB) |

São os bytes **solicitados** ao alocador pela estrutura. Não incluem arredondamento,
metadados internos de `malloc`, mapeamento de IDs do futuro carregador, buffers dos
testes, pilha, runtime ou instrumentação dos sanitizadores. Não são RSS, working set
nem memória total do processo. O teste de alocação registra os tamanhos realmente
passados a `malloc`/`calloc` e os compara com a estimativa e com `memoria_matriz`.

A multiplicação e a soma são verificadas contra `SIZE_MAX` antes da execução.
O teto `limite_bytes` é verificado antes de qualquer alocação, inclui o cabeçalho
e aceita igualdade. Zero é um teto insuficiente até para a matriz vazia. O teto
não é uma sondagem da RAM disponível: o chamador decide seu orçamento e o alocador
ainda pode falhar. Se a alocação das células falhar, o cabeçalho é liberado antes
do retorno; o chamador não recebe uma construção parcial.

Não se deve testar a viabilidade do dataset tentando primeiro uma alocação enorme.
Use a estimativa, escolha um orçamento menor que a memória disponível com margem
para o carregador e o processo, e só depois crie. Para 1.000 vértices, o teste usa
um teto de 2 MiB. A inspeção registrou 16.643.268 KiB de RAM visível e 6.002.028 KiB
livres antes da preparação; uma leitura posterior registrou 5.595.456 KiB livres.
Esse teste foi viável. Não há quantidade de vértices de dataset real disponível
para calcular ou alegar uma restrição de memória do dataset integral.

## Integração futura com os dados carregados

O único formato disponível na #4 é preliminar: `vertices.csv` com
`id,type,substation,circuit` e `edges.csv` com
`source,target,length,resistance,status`. Não há parser, esquema final, semântica
final de `status` ou função C para chamar. Por isso não há adaptador fictício nem
segundo parser nesta implementação.

O ponto de integração preparado é a API de criação/inserção acima. Quando #3/#4
entregarem o contrato real, a integração deverá:

1. Selecionar e registrar a origem/versionamento dos arquivos e o recorte.
2. Obter **todos** os vértices desse recorte, inclusive isolados. Se os IDs forem
   esparsos ou textuais, reutilizar o mapeamento ID → índice denso do carregador,
   preservando o mesmo mapa na lista e na matriz. Não usar `maior_id+1` como N.
3. Aplicar o mesmo conjunto de vértices, pares de arestas, política de duplicatas,
   laços e estado ativo na lista e na matriz. IDs negativos devem ser rejeitados
   antes de converter para `size_t`; IDs não mapeados não devem chegar ao módulo.
4. Estimar os bytes e verificar o orçamento. Se necessário, definir um recorte
   explícito e reproduzível em ambas as estruturas, registrando N, arestas, memória
   necessária, disponível e o motivo da limitação. Não truncar silenciosamente.
5. Chamar `criar_matriz` com N e inserir cada par ativo por `inserir_aresta`, após
   traduzir seus extremos. Interpretar `status` conforme #3/#4, sem supor que
   qualquer texto específico signifique ativo. Tratar falhas e liberar a matriz
   caso a construção não possa ser concluída.
6. Contar registros de segmentos e pares únicos separadamente, validar simetria,
   comparar as adjacências com os mesmos dados da lista e testar desativação e
   restauração conforme a semântica acordada.

**Pendências:** decisões de modelagem (#3), carregador e arquivos reais (#4 e suas
dependências), compatibilização com a futura lista (#5) e teste de integração real.
Nenhuma comparação de desempenho/memória com uma lista foi alegada.

## Validação executada em 01/10/2026

Windows 11 Home x64, Clang 23.1.2, GNU Make 4.4.1, C11, flags
`-Wall -Wextra -Wpedantic`. Não havia testes preexistentes a executar.
Compilação normal sem avisos; todos os executáveis e os alvos `test`/`sanitize`
retornaram código **0**. Os comandos reproduzíveis estão em
[`development.md`](development.md).

### Pequeno — sintético obrigatório

N=6, vértices 0 a 5; pares `(0,1), (0,2), (1,3), (2,3), (3,4)`.
Todas as 36 posições e suas inversas foram comparadas programaticamente:
inicialização zerada, inserção, duplicatas, remoção repetida/invertida de `(3,4)`
e restauração por `(4,3)`. O vértice 5 permaneceu isolado e a diagonal zerada.

A matriz impressa foi exatamente:

```text
     0 1 2 3 4 5
0 | 0 1 1 0 0 0
1 | 1 0 0 1 0 0
2 | 1 0 0 1 0 0
3 | 0 1 1 0 1 0
4 | 0 0 0 1 0 0
5 | 0 0 0 0 0 0
```

| Verificação | Esperado | Obtido | Resultado |
| --- | --- | --- | --- |
| Divergências em cada etapa | 0 | 0 | PASSOU |
| Assimetrias em cada etapa | 0 | 0 | PASSOU |
| Após remoção: [3][4] e [4][3] | 0 e 0 | 0 e 0 | PASSOU |
| Após restauração | Matriz acima | Matriz acima | PASSOU |
| Bytes da estrutura | 52 | 52 | PASSOU |

### Escala — exclusivamente sintética

N=1.000, cadeia `(0,1), (1,2), ..., (998,999)`, total de 999 arestas não
direcionadas. Foram comparadas **1.000.000 de posições em cada uma das quatro
etapas**: inicialização, cadeia completa, remoção de `(499,500)` e restauração.
A fórmula esperada contempla ambas as direções, todos os não vizinhos e a diagonal.

| Verificação | Esperado | Obtido | Resultado |
| --- | ---: | ---: | --- |
| Divergências por etapa | 0 | 0 | PASSOU |
| Inserções bem-sucedidas | 999 | 999 | PASSOU |
| Células conectadas na cadeia completa | 1.998 | 1.998 | PASSOU |
| Células conectadas após remoção | 1.996 | 1.996 | PASSOU |
| Células conectadas após restauração | 1.998 | 1.998 | PASSOU |
| Bytes estimados, informados e solicitados | 1.000.016 | 1.000.016 | PASSOU |

### Entradas inválidas e liberação

Testados: argumentos NULL, N=0, N=1, laço e duplicata de laço, índices iguais a N
e `SIZE_MAX` em ambos os extremos, overflow, saída de consulta preservada em erro,
matriz preservada em operações inválidas, tentativa de sobrescrever um dono,
teto insuficiente, teto exato e destruição repetida.

O segundo executável substitui apenas as chamadas do objeto do módulo a
`malloc`/`calloc`/`free` por funções de teste (flags `-D` no Makefile). O código de
produção mantém os alocadores padrão. O teste força falha na primeira e segunda
alocações, verifica retorno `MATRIZ_SEM_MEMORIA`, saída NULL, ausência de blocos
pendentes e sucesso da criação posterior. Também mede alocações para N=0, 1, 6 e
1.000; verifica que operações não alocam auxiliares e que entradas inviáveis são
rejeitadas antes de chamar o alocador.

```text
TOTAL SINTETICO: esperado=0 falhas obtido=0 PASSOU
TOTAL ALOCACAO: esperado=0 falhas obtido=0 PASSOU
```

As duas suites também passaram com **AddressSanitizer + UndefinedBehaviorSanitizer**,
sem diagnósticos de acesso indevido ou comportamento indefinido. Não foi executado
LeakSanitizer/Valgrind. A verificação de blocos e bytes não liberados é feita pela
instrumentação de alocações do módulo: esperado 0, obtido 0 em todas as etapas.
Isso não é uma medição de vazamentos de todos os componentes do processo.

### Dataset real

**NÃO EXECUTADO / PENDENTE:** não há arquivos do dataset nem carregador.
A BDGD/ANEEL é a fonte prevista pelo README, ainda sem distribuidora, versão,
arquivos ou recorte entregues. N e quantidade de arestas reais são desconhecidos;
nenhuma verificação de dados reais foi executada. Os dois grafos acima são
entradas controladas geradas nos testes, e não recortes da BDGD.

## Critérios de aceitação

| Critério da #6 | Estado | Evidência |
| --- | --- | --- |
| Matriz em C | Atendido | `src/matriz_adjacencia.c`, compilação C11 |
| Suporta recorte mínimo | Atendido em entradas controladas | 6 e 1.000 vértices testados |
| Inserção de arestas | Atendido | Cinco pares e 999 pares da cadeia |
| Consulta de adjacência | Atendido | Comparação de todas as células, inclusive ausências |
| Grafo não direcionado | Atendido | Verificação dos inversos e da simetria |
| Remoção/desativação | Atendido para pares | Remoção dos dois sentidos e reinserção sem alterar demais células |
| Integra com dataset | **Pendente** | Bloqueada por modelo, carregador e arquivos ausentes |
| Liberação correta | Atendido nos cenários testados | Contabilidade zerada, falhas injetadas, ASan/UBSan |
| Sem biblioteca externa de grafos | Atendido | Apenas biblioteca padrão C no módulo |
| Memória mensurável | Atendido | Estimativa comparada com bytes solicitados ao alocador |

A issue não deve ser considerada integralmente concluída enquanto o teste com o
dataset e carregador reais não for implementado e executado.
