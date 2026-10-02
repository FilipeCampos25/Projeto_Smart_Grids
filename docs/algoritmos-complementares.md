# Algoritmos complementares da Fase I — F1-15

## Estado encontrado e integração

Em 01/10/2026, a branch `feat/issue-17-algoritmos-complementares` estava limpa no
commit `de3a99f`, com lista, matriz, BFS e DFS. A issue #21 (F1-15) foi lida
integralmente e não tinha comentários. A issue #9 de componentes conexos ainda
estava aberta, sem implementação, teste ou branch remota; por isso esse módulo
não pôde ser examinado/reutilizado e não foi recriado dentro desta entrega.

Os algoritmos não criam outra arquitetura de grafos. Articulações, pontes e
coloração usam `GrafoLista`. A matriz recebeu uma extensão pequena para dígrafos:
`criar_matriz` permanece não direcionada e compatível, enquanto
`criar_matriz_direcionada` faz inserção/remoção em um único sentido. A consulta,
armazenamento, limites e liberação continuam os mesmos.

Arquivos da implementação:

- `include/algoritmos_complementares.h`: contratos e resultados públicos;
- `src/algoritmos_complementares.c`: os cinco recursos;
- `tests/test_algoritmos_complementares.c`: exemplos controlados e comparações;
- `include/matriz_adjacencia.h` e `src/matriz_adjacencia.c`: modo direcionado;
- `Makefile`: compilação e execução junto das regressões existentes.

## Conectividade em dígrafos

`verificar_conectividade_digrafo` aceita somente uma matriz criada no modo
direcionado e devolve dois valores:

- **fraca:** todos os vértices ficam conectados quando a direção dos arcos é
  ignorada. Durante a busca, `u` e `v` são vizinhos se existe `u -> v` ou `v -> u`;
- **forte:** existe caminho dirigido entre todo par de vértices. A função parte
  do vértice 0 e exige alcançar todos no digrafo original e no transposto. Isso
  equivale a todos alcançarem 0 e serem alcançados por 0.

A fila vetorial marca ao enfileirar. O custo é `O(V²)` e a memória auxiliar é
`O(V)`. O digrafo vazio devolve falso/falso; um único vértice devolve verdadeiro/
verdadeiro. As saídas são preservadas em erro.

Teste manual:

| Dígrafo | Fraca esperada/obtida | Forte esperada/obtida |
| --- | --- | --- |
| Ciclo `0->1->2->0` | 1 / 1 | 1 / 1 |
| Caminho `0->1->2` | 1 / 1 | 0 / 0 |
| Arco `0->1`, vértice 2 isolado | 0 / 0 | 0 / 0 |

O teste também confirma que inserir `0->1` não cria `1->0` e que remover um arco
preserva o arco inverso quando ele existe.

## Articulações e pontes

`analisar_criticidade_lista` executa uma única DFS clássica de Tarjan em todas as
componentes. Para cada vértice, guarda:

- `tempo_descoberta`: ordem em que entrou na DFS;
- `menor_tempo` (`low`): menor tempo alcançável descendo pela árvore e usando
  uma aresta de retorno;
- pai na árvore da DFS.

Uma raiz é articulação quando tem mais de um filho. Um vértice não raiz é
articulação quando existe filho com `low[filho] >= descoberta[vertice]`: esse
filho não consegue chegar a um ancestral sem passar pelo vértice.

Uma aresta de árvore `(vertice, filho)` é ponte quando
`low[filho] > descoberta[vertice]`. Nesse caso, nem o próprio vértice nem seus
ancestrais podem ser alcançados pelo ramo do filho sem essa aresta. Cada ponte é
devolvida uma vez, com o menor índice primeiro.

A lista é um grafo simples não direcionado, exatamente o contrato necessário.
O grafo não é modificado. O custo é `O(V+E)` e a memória é `O(V+E)`, incluindo a
saída. A implementação recursiva deixa a apresentação do algoritmo próxima da
formulação estudada. Em grafos extremamente profundos, a pilha de chamadas passa
a ser uma limitação prática; a cadeia sintética de 1.000 da DFS anterior usa uma
implementação iterativa e não tem essa limitação.

No teste, um triângulo `0-1-2-0` recebe o ramo `1-3`, com folhas `3-4` e `3-5`:

| Resultado | Esperado | Obtido |
| --- | --- | --- |
| Articulações | `{1,3}` | `{1,3}` |
| Pontes | `{(1,3),(3,4),(3,5)}` | as mesmas três |
| Aresta `(0,1)` do ciclo | não é ponte | não classificada |

Um ciclo de quatro vértices foi testado separadamente: zero articulações e zero
pontes, como esperado.

## Fórmula de Euler e limite de arestas

`verificar_formula_euler(V, E, F, &resultado)` verifica a igualdade pela forma
`V + F = E + 2`, evitando subtração sem sinal. Detecta overflow antes das somas.

Para uma **representação planar conexa** com faces conhecidas, `V-E+F=2`. Conferir
a igualdade não determina se um grafo arbitrário é planar: o valor de `F` já
pressupõe uma representação e a fórmula isolada não constrói nem valida essa
representação.

Para `V >= 3`, a função também informa `E <= 3V-6`. Esta é uma condição necessária
para grafos simples planares, mas **não suficiente**. O teste deixa isso explícito:

| Entrada | Esperado | Obtido |
| --- | --- | --- |
| Cubo: `V=8,E=12,F=6` | Euler verdadeiro | verdadeiro |
| `V=4,E=4,F=3` | Euler falso (`3`, não `2`) | falso |
| K5: `V=5,E=10` | viola `E<=9` | viola |
| K3,3: `V=6,E=9` | passa `E<=12`, apesar de não planar | passa somente no limite |

O exemplo K3,3 impede interpretar a desigualdade como teste de planaridade.

## Coloração gulosa

`colorir_grafo_guloso` processa índices internos em ordem `0..V-1`. Para cada
vértice, marca as cores de vizinhos já coloridos e escolhe a menor cor disponível,
começando em zero. Um vetor de marcações evita limpar V posições a cada vértice;
o custo fica `O(V+E)` e a memória auxiliar em `O(V)`.

O algoritmo sempre produz coloração válida para o grafo simples da lista, mas a
quantidade de cores depende da ordem e não é garantidamente mínima. No ciclo
ímpar `0-1-2-3-4-0`, o resultado esperado e obtido foi:

| Vértice | 0 | 1 | 2 | 3 | 4 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Cor esperada | 0 | 1 | 0 | 1 | 2 |
| Cor obtida | 0 | 1 | 0 | 1 | 2 |

Total esperado/obtido: 3/3 cores. Todas as cinco arestas foram verificadas e
nenhuma ligou vértices de mesma cor.

## Compilação e testes executados

No PowerShell, com GCC 16.2.0 e GNU Make 4.4.1 do MSYS2:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
make OS= EXT=.exe CC=gcc BUILD_DIR=build/f1-15-windows all
make OS= EXT=.exe CC=gcc BUILD_DIR=build/f1-15-windows test
```

`OS=` seleciona as receitas POSIX para esse Make do MSYS2; `EXT=.exe` preserva a
extensão Windows. Com MinGW Make nativo, use `mingw32-make CC=gcc test`.
Para executar somente esta issue: `make CC=gcc test-complementares`.

A suíte completa compila e executa matriz, testes de alocação da matriz, lista,
falhas da lista, BFS, falhas da BFS, DFS, falhas da DFS e os complementares.
Todos retornaram código zero. O teste novo apresentou **95 verificações e zero
falhas**. O log final está em `build/f1-15-windows/testes-finais.log`.

Também foi executada a suíte completa no Ubuntu/WSL com GCC 13.3.0:

```powershell
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/Users/jotar/OneDrive/Desktop/ProjetoSeleniumPython/Projeto_Smart_Grids && mkdir -p build/f1-15-final-linux && ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 make CC=gcc BUILD_DIR=build/f1-15-final-linux sanitize > build/f1-15-final-linux/sanitizadores-finais.log 2>&1'
```

O alvo usa AddressSanitizer, UndefinedBehaviorSanitizer e detecção de vazamentos.
Não houve aviso de compilação, acesso inválido, comportamento indefinido ou
vazamento nos caminhos executados. O log também fica em `build/`, ignorado pelo Git.

## Limitações e decisões

- A conectividade dirigida usa matriz; não foi criada uma segunda lista.
- Articulações, pontes e coloração se aplicam à lista não direcionada existente.
- A recursão de Tarjan é didática e pode exigir versão iterativa para grafos com
  profundidade muito maior que a disponível na pilha do processo.
- A coloração gulosa é válida, mas pode usar mais cores que o número cromático.
- Euler recebe contagens conhecidas e não substitui algoritmo de planaridade.
- Componentes conexos (#9), carregador e dataset real continuam ausentes; esta
  entrega não implementa silenciosamente essas outras issues.
