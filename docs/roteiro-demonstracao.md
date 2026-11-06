# Roteiro de demonstracao

## Preparacao

Requisitos: compilador C11, GNU Make e Python 3 para os testes e o analisador
dos experimentos. Na raiz do projeto:

```sh
make
make test
make test-real
```

No Windows, use `mingw32-make` se `make` nao estiver instalado. O alvo `make
test` tambem executa os testes da CLI e do protocolo experimental da Fase II.
Em PowerShell, o executavel compilado pode ser iniciado como
`.\build\smart_grid.exe --demo`.

## Demonstracao automatizada

Execute:

```sh
build/smart_grid --demo
```

O fluxo e nao interativo e utiliza o dataset versionado localmente. Apresenta
BFS, DFS, componentes conexos e simulacao/restauracao de falha em Lista e
Matriz. Depois executa Prim e Kruskal nas duas representacoes sobre um ciclo
sintetico de quatro vertices e cinco arestas. O resultado esperado e uma AGM
com tres arestas e custo minimo numerico 4. Os pesos sao custos assumidos
abstratos, nao medidas eletricas.

A demonstracao tambem informa que Dijkstra e Cobertura de Vertices nao estao
implementados neste workspace. Nenhum resultado para essas dependencias e
fabricado.

## Percurso interativo

Inicie `make run` ou `build/smart_grid` (PowerShell:
`.\build\smart_grid.exe`) e apresente:

1. Opcoes 1-4 para alternar Lista/Matriz e executar BFS, DFS e componentes.
2. Opcoes 5-8 para simular falhas, consultar metricas e ver algoritmos
   complementares.
3. Opcao 9 para verificar o estado real das funcionalidades da Fase II.
4. Opcao 10 para selecionar Prim/Kruskal, Lista/Matriz e a politica das arestas
   sem comprimento.

Para a AGM sobre a rede real, a opcao de manter arestas sem peso demonstra a
rejeicao explicita da entrada; nenhum peso e presumido. Para obter uma arvore
no dataset, escolha custos assumidos e informe um valor finito nao negativo.
O programa avisa que a solucao combina comprimentos medidos em metros e
valores abstratos, informa suas origens e separa os subtotais. Esse total nao
deve ser apresentado como grandeza fisica.

Entradas nao numericas, fora do intervalo e EOF podem ser demonstrados sem
interromper o processo com falha.

## Experimentos reproduziveis de AGM

Para gerar os arquivos brutos, consolidados e SVGs:

```sh
make experimentos-fase2
```

Para alterar a configuracao sem editar codigo:

```sh
make experimentos-fase2 REPETICOES_FASE2=11 SEMENTE_FASE2=12345
```

Os artefatos ficam sob `resultados/`, separados dos resultados historicos da
Fase I. A metodologia, as definicoes de memoria e as limitacoes estao em
[experimentos-fase-II.md](./experimentos-fase-II.md). O tempo varia com a
maquina; nao ha duracao fixa prometida. A geracao do grafo e a escrita dos
relatorios nao fazem parte do tempo cronometrado dos algoritmos.

## Limites para a apresentacao

- O dataset real e uma arvore (1.838 vertices, 1.837 arestas), portanto nao
  ilustra escolha entre alternativas ciclicas para a AGM; use o ciclo sintetico
  da demonstracao.
- Existem 304 arestas sem comprimento. Uma AGM ponderada sobre toda a rede
  requer que o apresentador declare explicitamente um custo assumido.
- Dijkstra, Cobertura de Vertices e seus experimentos estao pendentes das
  implementacoes das Issues #33 e #35.
- A simulacao de ilhas descreve conectividade topologica, nao autossuficiencia
  ou fluxo de potencia.
