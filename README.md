# Smart Grid Graph Analysis

Projeto integrador desenvolvido para a disciplina de Teoria dos Grafos.

## Tema

Smart Grids / Redes de Distribuição de Energia Elétrica.

## Objetivo

Modelar uma rede de distribuição elétrica real utilizando grafos e aplicar
algoritmos de conectividade e otimização para analisar contingências,
reconfiguração da rede e rotas de menor custo.

## Tecnologias

- Linguagem C
- Git/GitHub
- Dataset real de rede elétrica
- Implementações autorais dos algoritmos de grafos

## Fase I — Topologia e Conectividade

A primeira fase contempla:

- carregamento do dataset;
- lista de adjacência;
- matriz de adjacência;
- BFS;
- DFS;
- componentes conexos;
- simulação de falhas;
- comparação de tempo e memória.

## Fase II — Otimização e Complexidade

Planejado:

- Prim;
- Dijkstra;
- problema NP-Difícil a definir/validar;
- comparação entre soluções exatas e heurísticas.

## Dataset

A fonte prioritária em avaliação é a Base de Dados Geográfica da
Distribuidora (BDGD), disponibilizada pela ANEEL.

## Organização

As tarefas do projeto são gerenciadas utilizando GitHub Issues e GitHub Projects.

## Matriz de Adjacência (F1-05)

O módulo em C11 implementa criação, inserção, consulta, remoção/restauração de
conexões não direcionadas, liberação e estimativa de memória. Os testes cobrem
o exemplo de 6 vértices e uma cadeia sintética de 1.000 vértices.

Com compilador C11 e GNU Make instalados, execute na raiz:

```sh
make all
make test
```

No Windows com LLVM-MinGW: `mingw32-make CC=clang test`.
Para AddressSanitizer/UndefinedBehaviorSanitizer, use `make CC=clang sanitize`
(ou `mingw32-make CC=clang sanitize` no Windows).

Consulte [compilação e execução](docs/development.md) e
[contrato, explicação e resultados da matriz](docs/matriz-adjacencia.md).
A integração com dados reais permanece **pendente** das issues #3 e #4:
o repositório ainda não contém modelagem definitiva, carregador ou dataset.

## DFS — F1-07 / issue #8

A busca em profundidade está em `src/dfs.c`, com contrato em `include/dfs.h`.
`executar_dfs_lista` e `executar_dfs_matriz` usam as representações existentes e
retornam visitados, ordem de descoberta e quantidade de alcançados. A pilha fica
no heap, sem recursão. Cada busca visita somente a componente da origem e respeita
as conexões removidas, sem modificar o grafo. Libere o resultado com
`liberar_percurso_dfs`.

```sh
make all
make test-dfs  # DFS normal e com falhas de alocação
make test      # DFS e regressões da lista e da matriz
```

No Windows: `mingw32-make CC=clang test-dfs`. O alvo `sanitize` também inclui a DFS.
Foram executados o exemplo pequeno da issue, ciclos, desconexões e uma cadeia
sintética de 1.000 vértices nas duas representações. O [guia da DFS](docs/dfs.md)
explica a pilha, as ordens distintas de vizinhança, a memória e os resultados.
Consulte os [comandos e logs](docs/development.md) para reproduzir a validação.
**O teste com dataset real continua pendente:** faltam os arquivos e o carregador
da #4; os testes sintéticos não concluem esse critério da #8.

## Algoritmos complementares da Fase I — F1-15

O módulo `src/algoritmos_complementares.c` acrescenta conectividade fraca/forte
em dígrafos, articulações, pontes, verificação da Fórmula de Euler e coloração
gulosa. Ele reutiliza a lista existente e uma extensão direcionada compatível da
matriz. A criação tradicional da matriz permanece não direcionada.

```sh
make CC=gcc all
make CC=gcc test-complementares
make CC=gcc test
```

O último alvo executa também as regressões de matriz, lista, BFS e DFS. Consulte
[algoritmos, exemplos e resultados](docs/algoritmos-complementares.md). A fórmula
e a desigualdade de Euler são verificações acadêmicas, não um teste geral de
planaridade. A issue de componentes conexos continua aberta e fora deste escopo.
