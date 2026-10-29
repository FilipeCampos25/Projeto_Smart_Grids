# Metodologia das métricas de tempo e memória

## Escopo da medição

Os experimentos medem separadamente BFS, DFS e componentes conexos nas
representações por lista e por matriz. O intervalo começa imediatamente antes
da chamada do algoritmo e termina assim que ela retorna. Construção do grafo,
gravação do CSV e liberação do resultado ficam fora desse intervalo. A
instrumentação recebe os grafos como somente leitura e descarta o resultado
apenas depois de encerrar a medição; portanto, não muda a lógica nem o resultado
dos algoritmos.

O relógio é monotônico: `QueryPerformanceCounter` no Windows e
`clock_gettime(CLOCK_MONOTONIC)` em sistemas POSIX. Se a plataforma não fornecer
esse relógio, `clock()` é usado como alternativa. Os valores são convertidos e
registrados em milissegundos. Falhas do relógio e valores negativos, infinitos
ou NaN não são exportados como medições.

## Memória estimada das representações

A métrica de memória registrada no CSV se refere à estimativa estrutural da
representação usada em cada execução, e não ao RSS, heap total do processo,
metadados do alocador, dataset externo ou memória temporária dos algoritmos.

Para a Lista de Adjacência, a contagem é:

```text
sizeof(GrafoLista)
+ capacidade_vertices * sizeof(VizinhoLista *)
+ 2 * arestas * sizeof(VizinhoLista)
```

Esses termos cobrem o cabeçalho do grafo, o vetor de listas de vizinhos na
capacidade atual e os dois nós alocados por aresta não direcionada. A
capacidade do vetor é considerada explicitamente porque o grafo pode ter mais
slots do que vértices efetivamente utilizados. A estimativa usa a quantidade de
arestas atual, não o número de elementos duplicados da lista, e preserva a
invariância da estrutura: inserções e remoções alteram apenas a soma em bytes.

Para a Matriz de Adjacência, a contagem é:

```text
sizeof(MatrizAdjacencia) + N * N * sizeof(unsigned char)
```

A matriz armazena uma célula por par ordenado de vértices em um bloco contíguo,
portanto o custo cresce com $N^2$ e o cabeçalho da estrutura é incluído na
estimativa. A API pública expõe a função `memoria_matriz()` e a estimativa
`estimar_memoria_matriz()`, enquanto `memoria_lista()` mantém a mesma convenção
para o grafo em lista.

A implementação protege os cálculos contra overflow de `size_t`, retornando
zero quando a estimativa excede o intervalo representável sem alterar a API
pública. A unidade da coluna `memoria_bytes` no CSV é bytes; valores nulos,
negativos ou fora do intervalo representável são rejeitados durante a escrita.

## Repetições e exportação

`make experimentos` executa sete repetições de cada combinação nos prefixos de
100, 500 e 1000 vértices do dataset normalizado. Cada repetição é preservada no
CSV bruto, sem aquecimento descartado nem correção manual. O script
`scripts/gerar_graficos.py` calcula média e mediana a partir desses valores
brutos; a mediana é usada nos gráficos para reduzir a influência de variações do
sistema operacional.

O arquivo `resultados/fase1_metricas.csv` usa o esquema:

```text
algoritmo,representacao,vertices,arestas,tempo_ms,memoria_bytes,repeticao
```

As colunas identificam algoritmo, representação, V, E, tempo em milissegundos,
estimativa de memória da representação e número da repetição. Os números são
medidas da execução que gerou o arquivo; não devem ser reutilizados como se
fossem resultados de outra máquina ou configuração.

O teste `tests/test_metricas.c` gera um CSV temporário com execuções reais e
verifica o cabeçalho exato, os metadados, a presença de todas as repetições e
tempos finitos e não negativos.
