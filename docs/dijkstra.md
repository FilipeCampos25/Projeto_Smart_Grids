# Dijkstra: caminhos mínimos ponderados

## Funcionamento

O módulo `include/dijkstra.h` / `src/dijkstra.c` calcula um caminho de custo
mínimo entre dois índices internos. Ele usa somente pesos finitos e não
negativos, conforme a modelagem da Fase II, e não altera o grafo recebido.

Na lista de adjacência, a fronteira é uma fila de prioridade implementada como
heap binário indexado. Cada vértice aparece no máximo uma vez no heap e a
operação de diminuição de chave atualiza sua posição. Na matriz, o próximo
vértice é escolhido por varredura, coerente com o custo de percorrer uma linha
inteira da matriz. Empates são resolvidos pelo menor índice para tornar a
execução reproduzível entre as representações.

O resultado informa:

- caminho da origem ao destino, em ordem;
- custo acumulado;
- quantidade de vértices no caminho;
- quantidade de vértices definitivamente processados;
- alcançabilidade;
- uso ou não de custo assumido no caminho.

Origem igual ao destino é um caso válido: caminho unitário, custo zero e um
vértice processado. Destino inalcançável também é uma execução bem-sucedida,
com `alcancavel == 0`, caminho vazio e custo inicial zero. Erros de execução
são retornados separadamente no `StatusDijkstra`.

## Complexidade

Para `V` vértices e `E` arestas:

| Representação | Tempo | Espaço auxiliar |
|---|---:|---:|
| Lista + heap binário indexado | `O((V + E) log V)` | `O(V)` |
| Matriz + varredura | `O(V²)` | `O(V)` |

No modo de rejeição, a validação prévia de pesos custa `O(V + E)` na lista e
`O(V²)` na matriz, sem alterar as ordens assintóticas acima. A reconstrução do
caminho custa `O(V)` no pior caso.

## Contratos das APIs

```c
StatusDijkstra executar_dijkstra_lista(
    const GrafoLista *grafo, size_t origem, size_t destino,
    PoliticaPesoDijkstra politica, ResultadoDijkstra **resultado);

StatusDijkstra executar_dijkstra_matriz(
    const MatrizAdjacencia *matriz, size_t origem, size_t destino,
    PoliticaPesoDijkstra politica, ResultadoDijkstra **resultado);
```

`grafo`/`matriz` deve existir, os índices devem pertencer a `[0, V)`, a
política deve ser válida e `resultado` deve apontar para `NULL`. Em
`DIJKSTRA_SUCESSO`, o chamador recebe o resultado, inclusive no caso
inalcançável, e deve chamar `liberar_resultado_dijkstra`. Em qualquer erro, a
saída permanece `NULL` e toda memória parcial é liberada.

Status possíveis:

- `DIJKSTRA_SUCESSO`;
- `DIJKSTRA_ARGUMENTO_INVALIDO`;
- `DIJKSTRA_SEM_MEMORIA`;
- `DIJKSTRA_ARESTA_SEM_PESO`;
- `DIJKSTRA_OVERFLOW`;
- `DIJKSTRA_ERRO_GRAFO`.

A soma é verificada antes e depois da operação. Se uma relaxação excederia o
intervalo finito de `double`, a execução é abortada com
`DIJKSTRA_OVERFLOW`; um resultado parcial não é publicado.

## Política de pesos ausentes

Ausência de peso nunca significa peso zero. O chamador escolhe explicitamente:

- `DIJKSTRA_REJEITAR_ARESTA_SEM_PESO`: valida todo o grafo e rejeita a
  execução se qualquer aresta não tiver peso;
- `DIJKSTRA_IGNORAR_ARESTA_SEM_PESO`: mantém a aresta na topologia, mas ela
  não participa das relaxações e pode tornar o destino inalcançável.

Peso `0.0` informado é válido e participa normalmente do caminho. Pesos
negativos, NaN e infinito já são rejeitados pelas APIs de construção das
representações.

A CLI acrescenta uma terceira escolha de preparação do dataset: preencher
lacunas com um custo assumido, por meio das APIs ponderadas de `dataset.h`.
Quando o caminho usa uma dessas arestas, `utiliza_custo_assumido == 1` e a
saída identifica o resultado como **cenário abstrato**. Esse custo não é
distância física em metros. Sem custo assumido, os comprimentos medidos têm
unidade metro; no modo de ignorar, conexões sem comprimento deixam de
participar do caminho.

## Limitações

- Dijkstra requer pesos não negativos.
- A matriz ponderada usa armazenamento `O(V²)` e pode ser inadequada para
  grafos muito grandes.
- A rede real normalizada é uma árvore; grafos sintéticos cíclicos são usados
  para testar caminhos alternativos.
- A quantidade de processados depende do término ao retirar o destino da
  fronteira, não corresponde à quantidade de vértices do caminho.
- O custo somente representa metros quando todas as arestas do caminho têm
  comprimento medido.

## Exemplo reproduzível

```c
GrafoLista *g = criar_grafo_lista();
ResultadoDijkstra *r = NULL;
size_t indice;

for (size_t i = 0; i < 4; i++) inserir_vertice_lista(g, &indice);
inserir_aresta_ponderada_lista(g, 0, 1, 4.0,
                               ORIGEM_PESO_COMPRIMENTO_MEDIDO);
inserir_aresta_ponderada_lista(g, 0, 2, 1.0,
                               ORIGEM_PESO_COMPRIMENTO_MEDIDO);
inserir_aresta_ponderada_lista(g, 2, 1, 2.0,
                               ORIGEM_PESO_COMPRIMENTO_MEDIDO);
inserir_aresta_ponderada_lista(g, 1, 3, 1.0,
                               ORIGEM_PESO_COMPRIMENTO_MEDIDO);

executar_dijkstra_lista(g, 0, 3,
    DIJKSTRA_REJEITAR_ARESTA_SEM_PESO, &r);
/* caminho: 0 -> 2 -> 1 -> 3; custo: 4.0 */

liberar_resultado_dijkstra(&r);
liberar_grafo_lista(g);
```

O mesmo cenário faz parte de `tests/test_dijkstra.c` e é executado por
`make test`.
