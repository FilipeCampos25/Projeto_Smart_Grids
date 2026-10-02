# Modelagem final do grafo — Fase I

## Elementos do domínio

Um vértice representa um **ponto de conexão elétrico** (`PAC`) do circuito de
média tensão. O `PAC_INI` da tabela `CTMT` é classificado como vértice de
subestação; os demais são pontos de conexão. Transformadores não são fundidos
ao vértice: seus terminais continuam sendo PACs e seus atributos originais
podem ser incorporados na Fase II.

Uma aresta representa uma conexão física ativa entre dois PACs. No recorte há
segmentos de média tensão (`SSDMT`) e chaves fechadas (`UNSEMT`). Comprimento e
tipo são preservados no CSV, mas a Fase I usa um grafo não ponderado.

O grafo é **simples, não direcionado e não ponderado**. A orientação geométrica
de uma linha não representa sentido obrigatório do fluxo elétrico, portanto
BFS, DFS, componentes e falhas usam os dois sentidos.

## IDs e índices

IDs externos são strings e nunca viram posições de vetor diretamente. Durante
a carga, `dataset.c` cria uma tabela hash:

```text
ID real do PAC -> índice interno consecutivo [0, V)
```

Cada aresta resolve `origem` e `destino` nessa tabela. Referências ausentes,
IDs duplicados, linhas malformadas e self-loops rejeitam a carga inteira de
forma controlada. Depois da validação, a mesma lista de pares internos alimenta
tanto a Lista quanto a Matriz de Adjacência.

## Políticas de normalização

- Duplicatas não direcionadas: conserva-se uma conexão; o normalizador registra
  a ocorrência. O carregador também recebe erro da Lista se uma duplicata
  escapar do pipeline.
- Self-loops: removidos pelo normalizador e rejeitados pelo carregador.
- Conexões desativadas: não entram nos CSVs; chaves com `P_N_OPE=A` são abertas.
- Vértices isolados: o formato aceita, mas o recorte demonstrativo conserva o
  componente ativo da subestação.
- Falha: a aresta é removida temporariamente pelas APIs existentes, componentes
  são recalculados e a mesma aresta é restaurada antes do retorno.

## Atributos preservados

`arestas.csv` mantém o ID BDGD, a entidade/tipo, o comprimento em metros quando
existente, status e circuito. `vertices.csv` mantém ID, tipo, subestação e
circuito. Comprimento, resistência, potência e demais grandezas não são usados
como pesos nesta fase e ficam reservados a uma modelagem futura da Fase II.
