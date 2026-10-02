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


## Busca em largura (F1-06)

A BFS em `src/bfs.c` usa as APIs existentes de lista e matriz. Retorna os
vértices alcançados, sua quantidade, ordem de visita e distâncias em número de
arestas. Respeita conexões removidas e percorre somente a componente da origem.

```sh
make CC=gcc all
make CC=gcc test
```

A suíte conjunta executa os testes da lista, matriz e BFS, incluindo falhas de
alocação. Para selecionar somente a BFS, use `make CC=gcc test-bfs`.
Foram testados o exemplo de seis vértices, as camadas de visita, grafos
desconectados e uma cadeia e uma estrela de 1.000 vértices nas duas representações.

Consulte [API, explicação da fila, comandos Windows/WSL e resultados](docs/bfs.md).
O teste com dataset real continua **pendente** do carregador e dos arquivos.
