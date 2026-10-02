# Smart Grid — Fase I

Aplicação acadêmica em C para analisar topologia e conectividade de uma rede
real de distribuição de energia. O projeto carrega um recorte da BDGD/ANEEL,
constrói a mesma topologia como Lista e Matriz de Adjacência e executa BFS,
DFS, componentes conexos e uma simulação de falha com ilhas topológicas.

## Estado atual

- Dataset real: FORCEL, BDGD 2025, circuito `1_SFOR_1`.
- Topologia normalizada: 1.838 vértices e 1.837 arestas ativas.
- Subestação `SFOR`, origem externa `1_SFOR_1`.
- Lista e Matriz validadas semanticamente sobre os mesmos dados.
- CLI, medições, experimentos N=100/500/1000, CSVs e gráficos disponíveis.
- Algoritmos anteriores preservados: conectividade em dígrafos, articulações,
  pontes, verificação acadêmica de Euler e coloração gulosa.

“Ilha” significa somente uma componente topológica fora da componente da
subestação. O projeto não afirma autossuficiência energética e não calcula
fluxo de potência.

## Compilar e executar

É necessário um compilador C11 e GNU Make. Em Linux/macOS:

```sh
make
make run
make test
make test-real
make experimentos
python scripts/gerar_graficos.py
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

## CLI

O menu alterna Lista/Matriz, executa BFS, DFS e componentes, simula a falha
real, mostra métricas, exporta DOT e apresenta os algoritmos complementares.
Entradas inválidas são rejeitadas sem encerrar a aplicação.

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

As funcionalidades de otimização (Dijkstra, Prim, Kruskal etc.) pertencem à
Fase II e não fazem parte desta consolidação.
