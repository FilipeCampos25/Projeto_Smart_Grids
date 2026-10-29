# Metodologia das métricas de tempo

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
