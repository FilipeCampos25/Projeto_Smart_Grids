# Protocolo experimental da Fase II

## Escopo e dependencias

O workspace implementa AGM por Prim e Kruskal em Lista e Matriz. Na data da
execucao, os modulos de Dijkstra (Issue #33) e Cobertura de Vertices exata/
aproximativa (Issue #35) nao estao presentes. Esta entrega nao implementa esses
algoritmos, nao imputa seus resultados e nao compara grandezas de problemas
diferentes. Os experimentos de caminhos minimos e cobertura de vertices
permanecem pendentes dessas dependencias.

O dataset eletrico original nao foi alterado. O protocolo desta entrega nao usa
seus recortes para AGM: arestas reais sem comprimento nao possuem peso e a API
de AGM rejeita explicitamente essa condicao. Atribuir custos assumidos a toda a
topologia real requereria uma decisao metodologica separada.

## Instancias e comparacao

Cada escala `N=100`, `500` e `1000` usa uma instancia nao direcionada, conectada
e ciclica, gerada uma unica vez fora da regiao cronometrada. Primeiro se cria
uma arvore conectada: cada vertice `v > 0` liga-se a um vertice escolhido entre
`0..v-1`. Depois sao inseridas `2N` arestas simples adicionais, escolhidas
deterministicamente sem lacos nem duplicatas. Cada instancia possui
`E = 3N - 1` arestas. Os pesos sao inteiros de `1` a `1000`, todos marcados
como `ORIGEM_PESO_CUSTO_ASSUMIDO`; sao valores sinteticos sem unidade ou
interpretacao fisica.

O gerador usa LCG de 32 bits (`estado = estado * 1664525 + 1013904223`, modulo
`2^32`) e semente `semente_base + N` com aritmetica `uint32_t`. A semente base
padrao e `20261106`. O teste do gerador recria `N=100` com a mesma semente e
compara topologia, valores e origem. Lista e Matriz recebem exatamente as
mesmas arestas e pesos.

Prim e Kruskal sao executados sobre as duas representacoes em sete repeticoes
por padrao. A mesma instancia e reutilizada em todas as comparacoes; a ordem de
execucao alterna entre repeticoes pares e impares. Geracao, validacao posterior
e escrita de CSV ficam fora do intervalo cronometrado. O intervalo cobre
somente a chamada ao algoritmo e usa `QueryPerformanceCounter` no Windows ou
`clock_gettime(CLOCK_MONOTONIC)` em POSIX; falha do relogio e reportada como
erro.

Antes de registrar uma medicao como `OK`, verifica-se que a solucao e uma
arvore com `N-1` arestas, que cada aresta selecionada existe e preserva peso e
origem, que nao ha ciclos, que ha cobertura dos vertices e que a soma dos pesos
coincide com o custo retornado. O custo de Prim e Kruskal e comparado com
tolerancia relativa/absoluta de `1e-12`; divergencias sao registradas com
status de erro, e o analisador recusa consolidar um CSV que contenha status
distinto de `OK` ou comparacoes incompletas.

## Medicoes de memoria

`memoria_representacao_bytes` e a solicitacao estrutural informada pelas APIs
`memoria_lista`/`memoria_matriz` para as representacoes ponderadas. Nao e RSS,
heap total do processo nem consumo total de RAM.

`memoria_auxiliar_pico_bytes` contabiliza o pico de bytes solicitados via
`malloc` pelas alocacoes heap internas do algoritmo AGM durante a chamada,
incluindo retrato temporario, Union-Find/listas auxiliares e o vetor da solucao
que ainda pertence ao chamador ao retornar. O medidor nao inclui alocacoes das
estruturas do grafo, pilha do processo, metadados do alocador ou RSS. A coleta
usa um wrapper dedicado somente ao objeto AGM do executavel experimental; a
aplicacao e demais testes continuam usando os alocadores normais.

## Artefatos e reproducao

`make experimentos-fase2` preserva os arquivos da Fase I e gera:

- `resultados/fase2_bruto.csv`: cada algoritmo, representacao, tamanho,
  repeticao, semente, parametros, tempo, memoria, custo e status;
- `resultados/fase2_ambiente.txt`: compilador/versao detectados, padrao C,
  sistema, relogio, configuracao e parametros do protocolo;
- `resultados/fase2_consolidado.csv`: medias/medianas de tempo e pico auxiliar,
  memoria estrutural, custo mediano e minimo/maximo, numero de repeticoes e
  indicacao de equivalencia;
- `resultados/fase2_tempo.svg`, `resultados/fase2_memoria.svg` e
  `resultados/fase2_qualidade.svg`.

Reproducao com os valores padrao:

```sh
make
make test
make experimentos-fase2
```

Quantidade de repeticoes e semente sao substituiveis sem editar fontes:
o executavel aceita de 1 a 100 repeticoes para manter a execucao limitada.

```sh
make experimentos-fase2 REPETICOES_FASE2=11 SEMENTE_FASE2=12345
```

O script de consolidacao pode ser executado separadamente com arquivos
alternativos:

```sh
python scripts/analisar_experimentos_fase2.py \
  --entrada resultados/fase2_bruto.csv --diretorio resultados
```

O CSV bruto contem sementes e parametros por linha; os arquivos gerados devem
ser preservados junto da descricao da maquina, sistema operacional e compilador
usados na execucao. Medicoes sao especificas ao ambiente e nao devem ser
interpretadas como constantes de complexidade.

## Interpretacao e limitacoes

Kruskal ordena as arestas em `O(E log E)` e usa Union-Find; a captura da Matriz
inspeciona `O(N^2)` celulas. Prim implementado usa varredura linear dos
vertices, `O(N^2 + E)`, e listas auxiliares. As estimativas sao consistentes com
as implementacoes e representacoes efetivamente medidas.

O grafico de qualidade compara somente custos de AGM equivalentes nas quatro
combinacoes algoritmo/representacao. Dijkstra resolve caminhos entre pares
especificos e deve ser avaliado por distancia e validade do caminho; seu custo
nao deve ser comparado ao custo total de uma AGM. Cobertura de Vertices precisa
de testes de cobertura de arestas e razao de aproximacao contra uma solucao
exata nas mesmas instancias pequenas. Esses experimentos nao podem ser
executados ate a implementacao e integracao das Issues #33 e #35. Nao ha
backtracking de cobertura nesta entrega, nem execucao exponencial.

Os artefatos da Fase I (`fase1_*.csv` e graficos existentes) nao sao
modificados pelo alvo Fase II. Os novos CSVs/SVGs so sao produzidos quando o
executavel e executado; nao se publicam resultados ficticios caso compilador ou
dependencias estejam indisponiveis.
