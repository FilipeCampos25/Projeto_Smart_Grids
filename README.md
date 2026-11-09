# Smart Grid — Fases I e II

Aplicação acadêmica em C para analisar topologia e conectividade de uma rede
real de distribuição de energia. O projeto carrega um recorte da BDGD/ANEEL,
constrói a mesma topologia como Lista e Matriz de Adjacência e executa BFS,
DFS, componentes conexos, simulação de falha e, para grafos ponderados, Prim e
Kruskal.

## Estado atual

- Dataset real: FORCEL, BDGD 2025, circuito `1_SFOR_1`.
- Topologia normalizada: 1.838 vértices e 1.837 arestas ativas.
- Subestação `SFOR`, origem externa `1_SFOR_1`.
- Lista e Matriz validadas semanticamente sobre os mesmos dados.
- CLI, medições, experimentos N=100/500/1000, CSVs e gráficos disponíveis.
- Dijkstra em Lista (heap binário) e Matriz (varredura), com política explícita
  para arestas sem peso e indicação de custos assumidos.
- Algoritmos anteriores preservados: conectividade em dígrafos, articulações,
  pontes, verificação acadêmica de Euler e coloração gulosa.
- Prim e Kruskal calculam árvore ou floresta geradora mínima em Lista e Matriz.
- Cobertura de Vértices possui Backtracking exato com interrupção segura e
  aproximação 2 executável no dataset completo.

“Ilha” significa somente uma componente topológica fora da componente da
subestação. O projeto não afirma autossuficiência energética e não calcula
fluxo de potência.

## Compilar e executar

É necessário um compilador C11 e GNU Make. Python 3 também é necessário para
`make test`, os testes da CLI e os relatórios experimentais. Em Linux/macOS:

```sh
make
make run
make test
make test-real
make experimentos
python scripts/gerar_graficos.py
make experimentos-fase2
```

No Windows com LLVM-MinGW/MinGW:

```powershell
mingw32-make CC=clang
mingw32-make CC=clang run
mingw32-make CC=clang test
```

O executável é `build/smart_grid` (ou `build/smart_grid.exe`). Para uma
demonstração não interativa completa:

```sh
build/smart_grid --demo
```

Comandos de consulta:

```sh
build/smart_grid --help
build/smart_grid --experimentos
```

`--experimentos` continua sendo o modo de experimentos históricos da Fase I.
`make experimentos-fase2` executa separadamente AGM, Dijkstra e Cobertura de
Vértices exata/aproximada e gera os CSVs/SVGs documentados em
`docs/experimentos-fase-II.md`.

## CLI

O menu alterna Lista/Matriz, executa BFS, DFS, componentes, Dijkstra e Cobertura
de Vértices, simula a falha real, mostra métricas, exporta DOT e apresenta
algoritmos complementares.
Entradas inválidas são rejeitadas sem encerrar a aplicação.

No Dijkstra, origem e destino são informados pelos IDs externos do dataset. O
usuário escolhe rejeitar ou ignorar conexões sem comprimento, ou fornecer um
custo assumido. Resultados que usam custo assumido são identificados como
cenários abstratos, não como distância física real em metros.

O arquivo DOT pode ser convertido opcionalmente, sem afetar os algoritmos:

```sh
dot -Tpng resultados/simulacao_falha.dot -o resultados/simulacao_falha.png
```

## Documentação

- [Dataset e reprodução](docs/dataset.md)
- [Modelagem do grafo](docs/modelagem-grafo.md)
- [Desenvolvimento e testes](docs/development.md)
- [Consolidação da Fase I](docs/fase-I.md)
- [Resultados experimentais](docs/resultados-fase-I.md)
- [Modelagem de pesos da Fase II](docs/fase-II-modelagem.md)
- [Protocolo experimental da Fase II](docs/experimentos-fase-II.md)
- [Dijkstra: contratos, política de pesos e complexidade](docs/dijkstra.md)
- [Cobertura de Vértices: algoritmos, garantias e limites](docs/cobertura-vertices.md)

A infraestrutura de pesos da Fase II preserva comprimentos medidos e distingue
custos assumidos. Dijkstra, Prim e Kruskal estão disponíveis.
