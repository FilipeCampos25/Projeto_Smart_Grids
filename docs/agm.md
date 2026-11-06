# Arvore e floresta geradora minima

## API e propriedade do resultado

`include/agm.h` expoe quatro funcoes: `agm_prim_lista`,
`agm_kruskal_lista`, `agm_prim_matriz` e `agm_kruskal_matriz`. Cada funcao
recebe um grafo existente e uma saida `ResultadoAGM` inicializada vazia
(`arestas == NULL`). A entrada nunca e modificada. Em sucesso, a saida possui
as arestas selecionadas, custo total, quantidade de componentes e
`arvore_completa`; o chamador deve liberar a memoria com
`liberar_resultado_agm`. Erros deixam a saida intacta, inclusive em falha de
alocacao ou overflow.

`StatusAGM` diferencia argumento invalido, memoria insuficiente, limite de
representacao, peso ausente, peso invalido, matriz direcionada e overflow do
custo. Matrizes direcionadas sao rejeitadas, pois estes algoritmos implementam
AGM tradicional para grafos nao direcionados. Lacos que a matriz permite sao
ignorados, pois nao conectam componentes distintas nem podem pertencer a uma
arvore.

## Pesos e interpretacao

Todos os segmentos existentes devem ter peso valido. Se qualquer aresta da
entrada nao tiver peso, a operacao retorna `AGM_PESO_AUSENTE`, mesmo que essa
aresta nao acabasse escolhida. Isso evita tratar implicitamente ausencia como
zero ou omitir uma conexao da topologia. Peso zero e aceito normalmente.
Valores devem ser finitos e nao negativos; custo acumulado que exceda `DBL_MAX`
ou resulte nao finito retorna `AGM_OVERFLOW_CUSTO`, sem publicar resultado
parcial.

Cada `ArestaAGM` copia `PesoAresta`, incluindo a origem
`ORIGEM_PESO_COMPRIMENTO_MEDIDO` ou `ORIGEM_PESO_CUSTO_ASSUMIDO`. `custo_total`
e a soma numerica dos pesos escolhidos: nao converte origem, nao cria unidade e
nao apresenta custos assumidos como medidas fisicas. Unidades e limitacoes da
fonte estao em [fase-II-modelagem.md](./fase-II-modelagem.md).

Um grafo desconectado resulta em floresta geradora minima, uma arvore minima
por componente. Vertices isolados contam como componentes e nao produzem
arestas. O grafo vazio retorna zero componentes, zero arestas e
`arvore_completa == 0`; um unico vertice isolado tem uma componente e
`arvore_completa == 1`.

## Algoritmos e complexidade

As duas implementacoes consomem um retrato temporario das arestas para validar
pesos e preservar as estruturas de entrada.

- **Prim** armazena as incidencias em listas auxiliares e escolhe repetidamente
  o menor candidato por varredura linear dos vertices. Tempo `O(V^2 + E)` e
  memoria auxiliar/saida `O(V + E)`.
- **Kruskal** ordena as arestas por peso e indices deterministas, e usa
  Union-Find com compressao de caminho e uniao por rank. Tempo
  `O(V + E log E)` e memoria auxiliar/saida `O(V + E)`.
- Com matriz, a captura e validacao da topologia percorre `O(V^2)` celulas;
  portanto, o tempo total efetivo de Kruskal nessa representacao e
  `O(V^2 + E log E)`. Com lista, a captura custa `O(V + E)`.

Empates podem produzir arvores diferentes, mas conservam custo minimo. A ordem
de soma dos `double` pode variar; comparacoes numericas em testes usam
tolerancia relativa/absoluta de `1e-12` para valores finitos. A soma interna
sempre verifica overflow sem tolerancia.

## Validacao

`make test-agm` executa os testes funcionais e injeta falha em cada alocacao
temporaria de Prim e Kruskal, verificando que nenhum resultado parcial e
publicado e que toda memoria e liberada. Os casos incluem ciclo com AGM
conhecida, floresta desconexa e vertice isolado, arvore, aresta de peso zero,
peso ausente, multiplas solucoes otimas, direcionalidade, custo extremo,
equivalencia Lista/Matriz e invariantes de cobertura, ausencia de ciclos e
`E = V - componentes`.
