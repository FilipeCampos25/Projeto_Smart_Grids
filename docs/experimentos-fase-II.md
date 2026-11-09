# Protocolo experimental da Fase II

## Escopo

O protocolo mede separadamente três problemas: árvore geradora mínima (Prim e
Kruskal), caminho mínimo entre os vértices `0` e `N-1` (Dijkstra) e Cobertura
de Vértices (Backtracking exato e aproximação 2). Custos de problemas
diferentes nunca são comparados entre si.

As instâncias são sintéticas, não direcionadas, conectadas e ponderadas. Os
pesos inteiros de 1 a 1000 são custos assumidos, sem unidade e sem interpretação
como perdas elétricas. O dataset real de 1.838 vértices permanece fora deste
protocolo sintético e os artefatos históricos da Fase I não são modificados.

## Instâncias e repetibilidade

Para cada tamanho, o gerador cria uma árvore aleatória e acrescenta `2N`
arestas simples, totalizando `E=3N-1`. Um LCG de 32 bits recebe a semente
registrada no CSV. Lista e Matriz recebem as mesmas arestas e os mesmos pesos.

Prim, Kruskal, Dijkstra e a aproximação 2 são executados em `N=100`, `500` e
`1000`. O Backtracking exato é deliberadamente restrito a `N=9`, `11` e `13`;
a aproximação 2 é executada nessas mesmas instâncias para permitir a comparação
com o ótimo. A escolha evita executar indiscriminadamente um algoritmo
exponencial em mil vértices.

Por padrão são feitas sete repetições. A ordem das representações é alternada
entre repetições. Geração, validação e escrita do CSV ficam fora da região
cronometrada. O relógio é `QueryPerformanceCounter` no Windows e
`clock_gettime(CLOCK_MONOTONIC)` em POSIX.

## Validação

- AGM: a saída deve ter `N-1` arestas, ser acíclica e conectada, usar somente
  arestas da entrada e ter custo igual a um certificado independente por
  Kruskal. Prim e Kruskal, em Lista e Matriz, devem concordar no custo.
- Dijkstra: cada passo do caminho deve existir e a soma dos pesos deve ser igual
  ao custo informado. Esse custo também deve coincidir com uma referência
  independente por relaxamento de arestas e entre Lista e Matriz.
- Cobertura: todas as arestas devem ter ao menos um extremo selecionado. As duas
  execuções exatas devem provar o mesmo ótimo. Nas instâncias pequenas, a
  aproximação deve satisfazer `ótimo <= solução <= 2 * ótimo`.

Qualquer status diferente de `OK`, grupo incompleto, repetição ausente, métrica
não finita, total de memória inconsistente ou violação dessas relações faz o
analisador rejeitar o arquivo.

## Memória

As três métricas têm definições explícitas:

- `memoria_representacao_bytes`: bytes estruturais estimados pelas APIs da
  Lista ou Matriz ponderada;
- `memoria_auxiliar_pico_bytes`: maior quantidade simultânea de bytes
  solicitados por `malloc`/`calloc` dentro do algoritmo, incluindo a saída que
  ainda pertence ao chamador no retorno;
- `memoria_total_estimada_bytes`: soma das duas anteriores.

Essas métricas não são RSS e não incluem pilha, metadados do alocador ou o
próprio executável.

## Execução e artefatos

```sh
make test-experimentos-fase2
make experimentos-fase2
```

Parâmetros reproduzíveis podem ser alterados sem editar fontes:

```sh
make experimentos-fase2 REPETICOES_FASE2=11 SEMENTE_FASE2=12345
```

O alvo completo gera medições reais em `resultados/fase2_bruto.csv`, descrição
do ambiente em `resultados/fase2_ambiente.txt`, estatísticas em
`resultados/fase2_consolidado.csv` e os gráficos:

- `fase2_tempo.svg`;
- `fase2_memoria.svg`;
- `fase2_agm_custo.svg`;
- `fase2_dijkstra_custo.svg`;
- `fase2_cobertura_qualidade.svg`.

O consolidado informa média e mediana. Tempos dependem da máquina e da carga do
sistema; sementes e parâmetros permitem recriar instâncias e soluções, mas não
garantem tempos idênticos. A instrumentação mede solicitações explícitas no
heap, não consumo físico observado pelo sistema operacional.
