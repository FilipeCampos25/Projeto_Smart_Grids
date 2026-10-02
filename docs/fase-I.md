# Consolidação da Fase I

O fluxo final é:

```text
BDGD/ANEEL -> normalização reproduzível -> carregador C
-> Lista e Matriz -> BFS/DFS/componentes
-> falha real/restauração -> métricas/experimentos -> CLI
```

O carregador mantém dados de domínio separados das estruturas de grafos. Ele
resolve IDs externos por tabela hash e armazena as arestas uma única vez; as
funções `construir_lista_dataset` e `construir_matriz_dataset` consomem os mesmos
pares. Essa separação permite alternar a representação sem reinterpretar o CSV.

A falha demonstrativa é a ponte real entre os PACs `72352` e `12692`. O recorte
é uma árvore ativa: a remoção divide a rede em grupos de 1.149 (contendo a
subestação) e 689 vértices. O simulador existente calcula os três retratos —
antes, depois e após restauração — e confirma o retorno à topologia inicial.

Os algoritmos complementares permanecem no Makefile e na suíte. Na CLI,
articulações, pontes e coloração se aplicam à lista real. Conectividade forte é
explicada como própria de dígrafos; Euler exige faces de uma representação
planar e não é aplicado silenciosamente ao recorte geográfico.

## Limitações

- A análise é exclusivamente topológica; não há geração, demanda ou fluxo.
- O recorte inclui a rede MT ativa do circuito, não toda a rede BT da FORCEL.
- A memória registrada é estimativa das estruturas alocadas, sem overhead do
  alocador, atributos do dataset ou RSS do processo.
- Tempos muito pequenos sofrem variação do sistema; por isso são informadas
  sete repetições e mediana, sem ocultar os valores brutos.
- Graphviz é opcional e usado apenas para visualização.
