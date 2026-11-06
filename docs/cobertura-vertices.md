# Cobertura mínima de vértices

## Definição e interpretação

Para um grafo não direcionado `G = (V, E)`, uma cobertura de vértices é um
conjunto `C` contido em `V` tal que, para toda aresta `{u, v}` de `E`, `u`
pertence a `C` ou `v` pertence a `C`. O problema de Cobertura Mínima de
Vértices procura uma cobertura de menor cardinalidade. Sua versão de decisão é
NP-completa e a versão de otimização é NP-difícil.

Neste estudo, um vértice selecionado representa um possível ponto de
monitoramento topológico: cada conexão possui pelo menos uma extremidade
selecionada. Essa interpretação é acadêmica. **Cobrir todas as arestas da
topologia não equivale automaticamente a garantir observabilidade elétrica
real.** Observabilidade depende também do modelo elétrico, das grandezas
medidas, da posição e tipo dos sensores, da redundância, dos parâmetros da
rede e do estimador de estado.

Pesos de distância ou custo das arestas não são usados, pois esta implementação
resolve a versão clássica não ponderada.

## API e representações

O módulo público está em `include/cobertura_vertices.h` e aceita tanto
`GrafoLista` quanto `MatrizAdjacencia`. A implementação percorre somente as APIs
públicas desses tipos e cria uma visão temporária com cada aresta não
direcionada uma única vez. Não há uma terceira estrutura persistente de grafo.
Matrizes direcionadas são rejeitadas explicitamente.

Principais operações:

- `cobertura_vertices_exata_lista` e `cobertura_vertices_exata_matriz`;
- `cobertura_vertices_aproximada_lista` e
  `cobertura_vertices_aproximada_matriz`;
- `validar_cobertura_lista` e `validar_cobertura_matriz`;
- `vertices_selecionados_cobertura` e `cardinalidade_cobertura`;
- `liberar_resultado_cobertura`.

O vetor `selecionados` possui um byte por índice interno do grafo. Ele é
emprestado pelo resultado e não deve ser liberado separadamente.

## Solução exata por Backtracking

O algoritmo mantém uma cobertura parcial e escolhe uma aresta ainda não
coberta `{u, v}`. Toda cobertura válida deve conter `u` ou `v`; portanto, os
dois ramos adicionam respectivamente cada extremo. Quando nenhuma aresta resta,
a solução é comparada com a melhor conhecida.

As podas são seguras:

1. uma solução aproximada válida fornece o limite superior inicial;
2. ramos cuja cardinalidade já alcançou a melhor solução são descartados;
3. um matching maximal guloso entre as arestas ainda descobertas fornece um
   limite inferior: arestas disjuntas exigem vértices distintos na cobertura;
4. se a cobertura parcial somada ao limite inferior não pode melhorar a melhor
   solução, o ramo é descartado.

Sem interrupção, a árvore cobre todas as escolhas obrigatórias e o resultado
tem `otima_comprovada = 1`. O pior caso continua exponencial: a apresentação
direta é `O(2^V * (V + E))`, pois cada nó pode percorrer as arestas e zerar um
vetor de vértices. A memória auxiliar alocada é `O(V + E)`, além da pilha
recursiva `O(V)`.

## Solução aproximativa e garantia 2

Enquanto existe uma aresta não coberta, o algoritmo seleciona as duas
extremidades. As arestas incidentes ficam logicamente cobertas; repetir o
processo constrói um matching maximal `M` e devolve os extremos de suas
arestas.

As arestas de `M` não compartilham extremos. Qualquer cobertura ótima precisa
conter pelo menos um vértice para cada aresta de `M`, então `|OPT| >= |M|`. O
algoritmo escolhe exatamente dois extremos por aresta de `M`, logo
`|C| = 2|M| <= 2|OPT|`. A garantia vale para grafos não direcionados e não
ponderados.

Depois de obter a visão de arestas, o tempo é `O(V + E)` e a memória é
`O(V + E)`. Para a matriz, a extração custa `O(V²)`; para a lista, `O(V + E)`.
Essa abordagem é apropriada para o dataset completo.

## Interrupção segura

`ConfiguracaoCoberturaExata` possui limites independentes:

- `maximo_vertices`: impede iniciar a árvore em instâncias grandes;
- `maximo_nos_explorados`: orçamento determinístico para nós do Backtracking.

`SIZE_MAX` desativa o limite correspondente; zero impede qualquer instância ou
nó. Quando a configuração é nula, são usados limites conservadores de 60
vértices e 1.000.000 de nós. A CLI exige que o usuário informe ambos antes do
método exato e mostra um aviso sobre crescimento exponencial.

Ao atingir um limite, a função retorna `COBERTURA_LIMITE_ATINGIDO`. Pode haver
uma cobertura viável em `ResultadoCoberturaVertices`, mas
`otima_comprovada = 0`. A CLI a identifica como **INCOMPLETA** e nunca a chama
de ótima. Para a rede real de 1.838 vértices, a configuração padrão interrompe
antes do Backtracking; a aproximação continua disponível sobre o grafo todo.

## Memória e medições

A CLI relata separadamente:

- memória estimada da representação, por `memoria_lista` ou `memoria_matriz`;
- memória persistente do resultado;
- pico estimado das estruturas auxiliares do algoritmo.

O pico auxiliar considera a visão de arestas e os vetores alocados. Não inclui
metadados do alocador, RSS do processo nem o tamanho dependente de plataforma
dos frames da pilha recursiva. O tempo é medido pelo relógio já usado pelo
projeto; não são apresentadas medições fabricadas.

## Exemplos

```c
ResultadoCoberturaVertices *aproximada = NULL;
StatusCoberturaVertices status = cobertura_vertices_aproximada_lista(
    grafo, &aproximada);
if (status == COBERTURA_SUCESSO && aproximada->cobertura_valida) {
    printf("cardinalidade: %zu\n", cardinalidade_cobertura(aproximada));
}
liberar_resultado_cobertura(&aproximada);
```

```c
ConfiguracaoCoberturaExata cfg = {30, 500000};
ResultadoCoberturaVertices *exata = NULL;
StatusCoberturaVertices status = cobertura_vertices_exata_matriz(
    matriz, &cfg, &exata);
if (status == COBERTURA_SUCESSO && exata->otima_comprovada) {
    printf("mínimo comprovado: %zu\n", exata->cardinalidade);
} else if (status == COBERTURA_LIMITE_ATINGIDO) {
    puts("execução incompleta; não há certificado de otimalidade");
}
liberar_resultado_cobertura(&exata);
```

No menu, a opção **Cobertura mínima de vértices** permite executar o método
aproximado, o exato ou ambos sobre a representação atual. Na execução conjunta,
a diferença de cardinalidades só é interpretada como gap em relação ao ótimo
quando o método exato terminou com prova de otimalidade.

## Limitações

- somente grafos não direcionados e a versão não ponderada são suportados;
- a solução exata não é adequada a instâncias gerais grandes;
- a escolha da cobertura pode depender da ordem das arestas quando há empates;
- a estimativa de memória não é uma medição de RSS;
- a modelagem indica cobertura topológica de conexões, não observabilidade,
  confiabilidade, fluxo de potência ou suficiência de instrumentação;
- conforme registrado na issue #35, a adequação de Cobertura de Vértices ao
  requisito acadêmico NP-difícil ainda deve ser validada com o professor.
