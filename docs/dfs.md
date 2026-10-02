# DFS — F1-07 / issue #8

## Estado inspecionado e dependências

Em 01/10/2026 (America/Sao_Paulo), foram lidas integralmente as issues
[#8 — DFS](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/8),
[#5 — lista](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/5),
[#6 — matriz](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/6) e
[#4 — carregamento](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/4),
incluindo seus endpoints de comentários (todos vazios). #5 e #6 estavam fechadas;
#4 e #8, abertas. Não foram encontrados `AGENTS.md` aplicáveis.

O checkout correto era `Projeto_Smart_Grids`, limpo na branch `feat/issue-8-dfs`,
commit `acde247`. Após consultar o remoto, a branch recebeu por fast-forward a
`main` em `800738306c5ae6a9c128739a3959629175119e68`, que já integra lista e matriz.
Essa atualização trouxe o trabalho publicado da #6; a DFS não implementou outra
representação. Não havia BFS ou contrato compartilhado de resultados de travessia.
Foram preservados os módulos, headers e testes existentes de ambas as estruturas.

A #4 descreve apenas um formato preliminar: `vertices.csv` com
`id,type,substation,circuit` e `edges.csv` com
`source,target,length,resistance,status`. Não há contrato C do carregador, arquivos
do dataset nem semântica definitiva de `status`. A DFS não interpreta CSV ou IDs
externos e não supõe quais textos significam uma conexão ativa.

## Arquivos e API

| Arquivo | Responsabilidade |
| --- | --- |
| `include/dfs.h` | Contrato público, resultado e códigos de erro |
| `src/dfs.c` | Duas travessias autorais usando as APIs existentes |
| `tests/test_dfs.c` | Casos conhecidos, escala, comparação de conjuntos e falhas |
| `Makefile` | Alvos DFS e integração das regressões de lista/matriz |

| Função | Objetivo |
| --- | --- |
| `executar_dfs_lista(grafo, origem, &percurso)` | Busca pelos iteradores da lista, em sua ordem natural |
| `executar_dfs_matriz(matriz, origem, &percurso)` | Busca consultando as colunas em ordem crescente |
| `liberar_percurso_dfs(&percurso)` | Libera vetores e resultado; define o dono como NULL |

As duas buscas recebem um índice interno `size_t` em `[0, V)`. `percurso` deve
apontar para um ponteiro inicializado em `NULL`, como na criação da matriz. O
resultado `PercursoDFS` contém:

- `quantidade_vertices`: tamanho do grafo no momento da chamada;
- `visitados`: V bytes, cada um igual a 0 ou 1;
- `ordem`: vetor com capacidade V; apenas as primeiras `quantidade_alcancados`
  posições são válidas e contêm a ordem de descoberta;
- `quantidade_alcancados`: quantidade de descobertas únicas, incluindo a origem.

`visitados` representa o conjunto alcançado; `ordem` permite enumerá-lo sem buscar
por posições marcadas. Não há distâncias, tempos de descoberta ou classificação
de arestas. O resultado não guarda ponteiros para o grafo: pode ser lido após sua
liberação. O chamador deve usar `liberar_percurso_dfs`, sem liberar seus campos
separadamente ou alterar os ponteiros. A liberação aceita `NULL` e pode ser
repetida sobre o mesmo dono já zerado.

| Retorno | Condição |
| --- | --- |
| `DFS_SUCESSO` | Resultado completo entregue ao chamador |
| `DFS_ARGUMENTO_INVALIDO` | Grafo NULL, origem fora da faixa, saída NULL ou dono já ocupado |
| `DFS_SEM_MEMORIA` | Falha ao alocar resultado, vetores ou pilha |
| `DFS_LIMITE_EXCEDIDO` | Multiplicação de tamanho excederia `SIZE_MAX` |

Um grafo vazio não tem origem válida e retorna `DFS_ARGUMENTO_INVALIDO`. Um índice
negativo convertido para `SIZE_MAX` também é inválido nos grafos testados; o futuro
carregador deve rejeitar negativos antes da conversão. Em qualquer erro, a saída
fica preservada e as alocações parciais são liberadas. Entradas inválidas são
rejeitadas antes de alocar. A DFS nunca altera a topologia.

## Como a pilha produz uma busca em profundidade

Foi escolhida a versão **iterativa**, com pilha no heap. Uma cadeia longa pode
exigir V níveis de profundidade; usar recursão transferiria esse limite para a
pilha de chamadas, cujo tamanho depende do ambiente. A pilha explícita mantém o
procedimento simples, tem capacidade V e permite retornar erro de alocação.
Isso não pressupõe memória ilimitada nem garante que qualquer dataset caiba na RAM.

1. Alocar resultado, visitados zerados, ordem e pilha.
2. Marcar a origem e registrá-la na ordem; guardar sua vizinhança na pilha.
3. Examinar o próximo vizinho do topo e salvar onde continuar no pai.
4. Se o vizinho ainda não foi visitado, marcá-lo, registrá-lo e descer para ele.
5. Ao esgotar os vizinhos, retirar esse nível e retomar o pai.
6. Quando a pilha esvaziar, liberar somente a pilha e entregar o resultado.

Na lista, cada nível guarda o próximo `VizinhoLista` emprestado. Na matriz, guarda
o vértice e a próxima coluna a consultar. Salvar essa posição evita reiniciar a
busca de vizinhos ao voltar de um filho. Os iteradores são válidos porque o grafo
deve permanecer sem alterações durante a chamada, conforme o contrato da lista.

A marcação acontece **ao descer para um vértice**, antes de examinar seus vizinhos.
Assim ele nunca é empilhado/registrado duas vezes, mesmo com ciclo ou laço. Cada
nível representa um vértice distinto: a profundidade nunca excede V. Marcar todos
os vizinhos de um pai antecipadamente pode impedir a exploração correta de uma
aresta entre ramos; há um teste específico para detectar esse erro.

## Ordem, desconexão e conexões desativadas

A lista não promete uma ordem na API; a DFS consome a ordem entregue pelos seus
iteradores, sem ordenar ou copiar a vizinhança. Na implementação publicada, uma
inserção entra na cabeça. Inserindo `(0,1), (0,2), (1,3), (1,4)` nessa sequência:
0 tem vizinhos `[2,1]` e 1 tem `[4,3,0]`. A ordem derivada é `[0,2,1,4,3]`.
Uma futura mudança da ordem da estrutura exige ajustar essa expectativa específica
do teste, sem mudar a regra da DFS.

A matriz consulta índices de 0 a V-1. Para as mesmas arestas, a ordem é
`[0,1,3,4,2]`: esgota o ramo de 1 antes de voltar para 2. As duas buscas produzem
o mesmo conjunto `{0,1,2,3,4}`. O vértice 5 não é visitado partindo de 0; partindo
de 5, o resultado é `[5]`. A DFS não começa automaticamente outra componente.

As representações atuais desativam conexões por **remoção bilateral**. A DFS usa
somente conexões presentes, sem restaurar ou modificar nada. O simulador pode
remover uma conexão, executar a busca e reinseri-la depois. A lista rejeita laços
e duplicatas; a matriz aceita laços e inserções repetidas idempotentes. Essas
políticas continuam nas estruturas; a DFS respeita ambas. A compatibilização de
segmentos paralelos e estados de domínio continua sob #3/#4.

O cálculo futuro de componentes pode manter um vetor global separado, escolher
uma origem ainda não coberta e incorporar `ordem`/`visitados` ao resultado global.
Esse laço pertence à issue de componentes. Cada chamada atual começa uma busca
nova e inicializa V posições; ela não recebe um vetor global já marcado. Portanto,
repeti-la por C componentes tem também o custo O(CV) de inicialização. Uma eventual
API que compartilhe marcações para calcular todas as componentes em tempo linear
deve ser decidida nessa integração, sem atribuir tal garantia a esta interface.

## Complexidade e memória

Com R vértices e A arestas na componente alcançada:

| Representação | Tempo da chamada | Memória adicional |
| --- | --- | --- |
| Lista | O(V + R + A), portanto O(V + E) | O(V) |
| Matriz | O(V + R·V), portanto O(V²) para V > 0 | O(V) |

A parcela O(V) inicializa visitados. Na lista, cada elo alcançado é examinado uma
vez; na matriz, cada linha alcançada é examinada uma vez. A capacidade da pilha e
da ordem é V, mesmo em grafo desconectado. São quatro alocações por busca bem
sucedida: cabeçalho, visitados, ordem e pilha. A pilha é liberada antes do retorno;
os outros três blocos pertencem ao resultado. Os produtos são verificados antes
de alocar. O teste não tenta materializar um grafo próximo de `SIZE_MAX`.

## Exemplo de utilização

Esta função recebe uma lista já construída e deixa o grafo sob propriedade do
chamador. O mesmo padrão vale para `executar_dfs_matriz`.

```c
#include "dfs.h"
#include <stdio.h>

int mostrar_alcancados(const GrafoLista *grafo, size_t origem)
{
    PercursoDFS *percurso = NULL;
    ResultadoDFS resultado = executar_dfs_lista(grafo, origem, &percurso);
    if (resultado != DFS_SUCESSO) {
        fprintf(stderr, "Falha na DFS: %d\n", (int)resultado);
        return 1;
    }
    for (size_t posicao = 0; posicao < percurso->quantidade_alcancados; posicao++) {
        printf("%zu\n", percurso->ordem[posicao]);
    }
    liberar_percurso_dfs(&percurso);
    return 0;
}
```

## Compilação e execução realizadas

Os comandos de preparação do PowerShell estão em [development.md](development.md).
Na raiz do checkout, com Clang 23.1.2 e GNU Make 4.4.1 no PATH:

```powershell
mingw32-make CC=clang BUILD_DIR=build/dfs all
mingw32-make CC=clang BUILD_DIR=build/dfs test
mingw32-make CC=clang BUILD_DIR=build/dfs sanitize
```

Linha exata da compilação normal da DFS, emitida pelo Makefile:

```text
clang -Iinclude -std=c11 -Wall -Wextra -Wpedantic -O2 src/dfs.c src/grafo_lista.c src/matriz_adjacencia.c tests/test_dfs.c -o build/dfs/teste_dfs.exe
```

Para executar isoladamente os binários, já compilados:

```powershell
.\build\dfs\teste_dfs.exe
.\build\dfs\teste_dfs_falhas.exe
```

O alvo `test-dfs` também compila/executa essas duas suites. O alvo `test` executa
adicionalmente as quatro suites anteriores de lista e matriz, incluindo alocação.
`sanitize` recompila todas em `build/dfs/sanitizers` com ASan/UBSan. Os logs estão
em `build/dfs-compilacao.log`, `build/dfs-testes.log` e
`build/dfs-sanitizadores.log`, ignorados pelo Git.

### Pequenos — esperado e obtido

Todas as ordens abaixo foram definidas antes de executar e comparadas
programaticamente. Quantidade, conjunto completo de visitados e ausência de
repetições também foram comparados; a tabela mostra **esperado = obtido**.

| Caso | Lista: esperado = obtido | Matriz: esperado = obtido | Quantidade | Resultado |
| --- | --- | --- | ---: | --- |
| Obrigatório, origem 0 | `[0,2,1,4,3]` | `[0,1,3,4,2]` | 5 | PASSOU |
| Origem isolada 5 | `[5]` | `[5]` | 1 | PASSOU |
| Adicionar ciclo `(3,0)` | `[0,3,1,4,2]` | `[0,1,3,4,2]` | 5 | PASSOU |
| Remover `(1,4)` do caso com ciclo | `[0,3,1,2]` | `[0,1,3,2]` | 4 | PASSOU |
| Partir de 4 após remoção | `[4]` | `[4]` | 1 | PASSOU |
| Restaurar `(1,4)`, origem 0 | `[0,3,1,4,2]` | `[0,1,3,4,2]` | 5 | PASSOU |
| Ramos conectados: `(0,1),(0,2),(1,3),(2,4),(1,2)` | `[0,2,1,3,4]` | `[0,1,2,4,3]` | 5 | PASSOU |

As 36 adjacências do exemplo obrigatório foram conferidas contra matrizes
esperadas antes/depois das buscas, incluindo ausências e o vértice isolado:
**zero alterações indevidas**. O ciclo manteve o conjunto `{0,1,2,3,4}` nas duas
representações, com **zero repetições**.

### Escala — exclusivamente sintética

Cadeia de 1.000 vértices (0 a 999), com 999 arestas `(i,i+1)`. Cada um dos índices
foi comparado, embora a saída mostre apenas extremos para ordens longas.

| Caso, nas duas representações | Esperado | Obtido | Resultado |
| --- | --- | --- | --- |
| Origem 0 | `[0,1,...,999]`, 1.000 alcançados | Igual | PASSOU |
| Origem 999 | `[999,998,...,0]`, 1.000 alcançados | Igual | PASSOU |
| Cortar `(499,500)`, origem 0 | `[0,1,...,499]`, 500 alcançados | Igual | PASSOU |
| Após corte, origem 999 | `[999,998,...,500]`, 500 alcançados | Igual | PASSOU |
| Restaurar conexão, origem 0 | `[0,1,...,999]`, 1.000 alcançados | Igual | PASSOU |

### Erros, alocação e regressões

Foram testados grafos NULL/vazios, origens V e `SIZE_MAX`, saída NULL, dono ocupado,
grafo unitário, laço e duplicata de laço na matriz, resultado após liberar o grafo
e liberação repetida. Erros retornaram `DFS_ARGUMENTO_INVALIDO` e preservaram as
saídas. A instrumentação reaproveita `tests/alocacao_teste.h`, substituindo apenas
`malloc/free` no objeto da DFS; os grafos e o próprio teste continuam usando a libc.

Cada uma das quatro alocações da DFS falhou separadamente em cada representação:
**8 pontos exercitados**. Esperado/obtido em todos: `DFS_SEM_MEMORIA`, saída NULL,
zero blocos vivos e grafo preservado. A nova tentativa teve sucesso. Ao terminar
os testes, esperado **0 blocos vivos**, obtido **0**.

| Suite | Normal | ASan + UBSan | Saída |
| --- | --- | --- | --- |
| DFS | PASSOU, 4.755 verificações, 0 falhas | Mesmo resultado | 0 |
| DFS com falhas | PASSOU, 5.743 verificações, 0 falhas | Mesmo resultado | 0 |
| Lista preexistente | PASSOU, 4.130 verificações, 0 falhas | Mesmo resultado | 0 |
| Lista com falhas | PASSOU, 5.587 verificações, 0 falhas | Mesmo resultado | 0 |
| Matriz preexistente | PASSOU, total sintético 0 falhas | Mesmo resultado | 0 |
| Alocação da matriz | PASSOU, total alocação 0 falhas | Mesmo resultado | 0 |

Não houve avisos do compilador nem diagnósticos dos sanitizadores. Não foi
executado LeakSanitizer/Valgrind: a evidência de liberação é o contador de blocos
do módulo nos caminhos testados, complementado por ASan/UBSan. Os totais das
suites instrumentadas incluem novamente os casos funcionais. Qualquer comparação
divergente imprime `FALHOU`, incrementa o contador e faz `main` retornar
`EXIT_FAILURE`; não se usa `assert` como única verificação.

### Dataset real

O teste de integração carregou o circuito FORCEL/BDGD com 1.838 vértices e 1.837
arestas. A DFS partindo da subestação alcançou todos os vértices na Lista e na
Matriz; os vetores de visitados foram comparados semanticamente.

Para concluir esse critério: entregar recorte rastreável e carregador, aplicar o
mesmo mapeamento de IDs e estado ativo às duas representações, registrar V/E e
origem, executar as duas buscas, conferir conjuntos/contagens/ausência de repetição
e preservar os logs. As ordens podem diferir. Caso o recorte não caiba na matriz,
registrar a estimativa e escolher um recorte comum explícito, conforme a #6.

## Critérios da issue #8

| Critério | Estado e evidência |
| --- | --- |
| DFS autoral em C | Atendido: C11, apenas biblioteca padrão e APIs do projeto |
| Lista de adjacência | Atendido: `executar_dfs_lista`, exemplo, ciclos e escala |
| Matriz de adjacência | Atendido: `executar_dfs_matriz`, mesmos casos |
| Não processa o mesmo vértice indevidamente | Atendido: marcação na descoberta; repetições esperadas/obtidas = 0 |
| Grafos desconectados | Atendido: origem 0 não alcança 5; isolados e corte da cadeia |
| Pelo menos 1.000 vértices | Atendido: cadeia completa nas duas direções e representações |
| Grafo pequeno conhecido | Atendido: seis vértices e quatro arestas exatas do pedido |
| Dataset real | Atendido: FORCEL/BDGD, mesmo conjunto nas duas representações |
| Documentação de implementação/comportamento | Atendido: header, este guia, README e desenvolvimento |

A implementação, a validação sintética e a execução com dados reais estão
entregues.
