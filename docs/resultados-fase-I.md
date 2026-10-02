# Resultados experimentais da Fase I

## Protocolo

Os subconjuntos N=100, 500 e 1000 são prefixos da BFS determinística iniciada
no `PAC_INI` do mesmo circuito real. Como o recorte é uma árvore, cada prefixo
conectado tem N−1 arestas. Para cada tamanho são executados BFS, DFS e
Componentes em Lista e Matriz, com sete repetições. O CSV bruto conserva todas
as medições e a tabela abaixo usa a mediana.

| N | Arestas | Algoritmo | Lista (ms) | Matriz (ms) |
|---:|---:|---|---:|---:|
| 100 | 99 | BFS | 0,0011 | 0,0241 |
| 100 | 99 | DFS | 0,0014 | 0,0235 |
| 100 | 99 | Componentes | 0,0016 | 0,0245 |
| 500 | 499 | BFS | 0,0053 | 0,5538 |
| 500 | 499 | DFS | 0,0065 | 0,5259 |
| 500 | 499 | Componentes | 0,0060 | 0,5519 |
| 1000 | 999 | BFS | 0,0123 | 2,1894 |
| 1000 | 999 | DFS | 0,0154 | 2,1112 |
| 1000 | 999 | Componentes | 0,0140 | 2,1927 |

## Memória estimada

| N | Lista (bytes) | Matriz (bytes) |
|---:|---:|---:|
| 100 | 4.224 | 10.024 |
| 500 | 20.096 | 250.024 |
| 1000 | 40.192 | 1.000.024 |
| 1838 (aplicação) | 75.200 | 3.378.268 |

A Lista cresce com capacidade de vértices e dois nós por aresta. A Matriz usa
um byte por célula, portanto cresce com N². Esses números são solicitações de
memória das estruturas, não RSS nem memória total do processo.

Nos três tamanhos, a Lista foi mais rápida e menor para esta topologia esparsa.
Os valores completos, inclusive a primeira repetição mais lenta, permanecem no
CSV e não foram descartados.

## Artefatos

- `resultados/fase1_metricas.csv`: 126 medições brutas;
- `resultados/fase1_consolidado.csv`: média, mediana e memória;
- `resultados/tempo_lista_matriz.svg`: crescimento do tempo;
- `resultados/memoria_lista_matriz.svg`: memória (escala logarítmica);
- `resultados/simulacao_falha.dot`: rede após a falha demonstrativa.

Reprodução:

```sh
make experimentos
python scripts/gerar_graficos.py
```
