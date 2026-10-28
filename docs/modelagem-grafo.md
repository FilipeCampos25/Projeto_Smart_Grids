# Modelo conceitual do grafo — Fase I

## Definição formal e significado físico

O modelo principal é um grafo simples, não direcionado e não ponderado

```text
G = (V, E), com E contido em {{u, v} | u, v em V e u != v}.
```

- Cada vértice representa um **ponto de conexão elétrico** (PAC) do circuito de
  média tensão. Um PAC é um nó topológico, não necessariamente um equipamento.
- Cada aresta `{u, v}` representa uma **conexão física ativa** entre dois PACs.
  No recorte atual ela vem de um segmento de média tensão (`SSDMT`) ou de uma
  chave de média tensão fechada (`UNSEMT`).

A ordem `origem,destino` do CSV não define orientação: `{u,v} = {v,u}`. A Lista
de Adjacência armazena dois nós de vizinhança por aresta, e a matriz principal
grava células simétricas. Assim, BFS, DFS e componentes analisam
**conectividade topológica** nos dois sentidos.

Conectividade não é fluxo elétrico. O modelo não calcula sentido ou intensidade
de corrente, queda de tensão, capacidade, proteção nem balanço de potência. A
remoção de uma aresta permite afirmar que PACs ficaram topologicamente separados
da subestação; isso, isoladamente, não é um estudo de fluxo de potência.

## Identidade externa e índice interno

Os IDs externos dos PACs e ativos são strings da BDGD e permanecem nos campos
`id_externo` de `VerticeDataset` e `ArestaDataset`. Eles não são usados como
posições de vetor. Durante a carga, `src/dataset.c` constrói o mapeamento

```text
ID externo do PAC -> size_t consecutivo no intervalo [0, |V|)
```

Os campos textuais `origem` e `destino` de cada registro são resolvidos para os
índices `ArestaDataset.origem` e `.destino`. A ordem das linhas de
`vertices.csv` determina esses índices; no recorte versionado ela foi produzida
por uma BFS determinística iniciada em `PAC_INI`, mas os algoritmos dependem
somente de índices válidos, não dessa ordenação. A tabela hash existe apenas
durante a carga. O `Dataset` conserva o vetor que permite obter o ID externo a
partir do índice, e `buscar_indice_dataset` oferece a busca inversa linear para
interação e relatórios.

IDs de vértice repetidos e arestas que referenciem um ID ausente rejeitam a
carga inteira, com liberação da construção parcial.

## Atributos e subestação

Os atributos de domínio ficam no `Dataset`; `GrafoLista` e
`MatrizAdjacencia` guardam somente índices e adjacência.

| Elemento | Atributos preservados | Uso na Fase I |
|---|---|---|
| Vértice | `id_externo`, `tipo`, `subestacao`, `circuito` | identidade, apresentação e escolha da raiz |
| Aresta | `id_externo`, `origem`, `destino`, `tipo`, `comprimento_m`, `possui_comprimento`, `status`, `circuito` | identidade e construção da topologia ativa |

O normalizador marca o `CTMT.PAC_INI` como `tipo=subestacao` e registra o código
`CTMT.SUB` no atributo `subestacao`. O carregador escolhe como
`Dataset.indice_subestacao` o **primeiro** vértice cujo `tipo` seja
`subestacao` e rejeita um arquivo sem nenhum; ele não exige unicidade desse tipo
nem deduz a subestação pelo índice zero. Os demais PACs do recorte usam
`tipo=ponto_conexao`, embora o código aceite outras strings não vazias.

Transformadores (`UNTRMT`) não são lidos pelo normalizador atual e, portanto,
não viram vértices nem arestas do recorte da Fase I. Não se deve interpretar um
PAC genérico como transformador. Uma extensão futura pode ligar os terminais do
equipamento e preservar seus atributos, mas essa transformação ainda não faz
parte do modelo implementado.

Comprimento é opcional e está presente nos segmentos do recorte; não é copiado
para as structs genéricas do grafo nem usado como peso. Resistência, potência,
geometria e atributos de transformadores não estão disponíveis no `Dataset`
atual. Logo, uma Fase II ponderada exigirá ampliar a extração e/ou o modelo, e
não apenas alterar os algoritmos.

## Segmentos e demais conexões

Cada linha `SSDMT` ativa preservada pelo normalizador gera uma aresta
`tipo=segmento_mt` entre seus `PAC_1` e `PAC_2`; segmentos consecutivos não são
fundidos. Cada `UNSEMT` ativa e normalmente fechada gera uma aresta
`tipo=chave_mt`. Reguladores (`UNREMT`) e compensadores (`UNCRMT`) são candidatos
no script, mas nenhum permaneceu no recorte versionado. Equipamentos abertos ou
inativos não são substituídos por uma ligação direta entre seus extremos.

## Casos de integridade e estado

As regras abaixo separam o contrato do modelo carregado das capacidades das
estruturas genéricas:

- **Duplicatas:** o normalizador elimina pares repetidos sem considerar a ordem
  dos extremos. O carregador C não procura previamente IDs de aresta nem pares
  duplicados: ao construir a Lista, `inserir_aresta_lista` rejeita uma segunda
  conexão, inclusive invertida, e a carga do grafo falha; a matriz, isoladamente,
  trata repetição como escrita idempotente. O modelo conceitual conserva no
  máximo uma aresta por par de PACs e não representa arestas paralelas.
- **Laços:** o normalizador os elimina e `carregar_dataset` rejeita
  `origem == destino`. A Lista também os rejeita. A API genérica da matriz aceita
  a diagonal, mas esse recurso não pertence ao modelo principal nem é alcançado
  por `construir_matriz_dataset` com um `Dataset` válido.
- **Vértices isolados:** são válidos conceitualmente e nas duas representações;
  têm lista vazia ou linha/coluna zerada e formam componente unitária. O
  carregador aceita um vértice sem arestas incidentes, embora exija que o arquivo
  de arestas contenha ao menos um registro. O recorte demonstrativo não possui
  isolados porque conserva somente o componente alcançável de `PAC_INI`.
- **Arestas inativas e chaves abertas:** o normalizador descarta unidades
  inativas e `P_N_OPE=A`; o CSV final contém apenas `status=ativo`, e o carregador
  rejeita outro valor. Portanto, inatividade de origem é representada por
  ausência em `E`, sem histórico dentro do grafo.
- **Falha simulada:** as APIs removem temporariamente uma aresta existente,
  recalculam componentes e restauram a mesma conexão antes de retornar. A Lista
  libera e recria seus dois nós; a matriz troca `1` por `0` e depois por `1`.
  Nenhuma delas guarda status, ID ou histórico da aresta removida.

`criar_matriz_direcionada` existe como extensão genérica para algoritmos de
digrafos, mas não é chamada por `construir_matriz_dataset` e não altera a decisão
de não direcionamento do modelo elétrico principal.

## Correspondência com a implementação

| Decisão | Evidência no código |
|---|---|
| PACs são vértices e atributos ficam fora do grafo | `VerticeDataset` em `include/dataset.h`; construção consecutiva em `construir_lista_dataset` |
| Conexões ativas são arestas sem peso | `ArestaDataset`; `construir_lista_dataset` e `construir_matriz_dataset` passam somente os dois índices |
| Modelo principal não direcionado | dois vizinhos em `src/grafo_lista.c`; `criar_matriz`, não `criar_matriz_direcionada`, em `src/dataset.c` |
| IDs externos são distintos dos índices | tabela `MapaIds` e campos `origem`/`destino` resolvidos em `src/dataset.c` |
| Subestação é atributo e raiz explícita | `VerticeDataset.tipo`, `.subestacao` e `Dataset.indice_subestacao` |
| Modelo é simples | rejeição de laços no carregador e na Lista; rejeição de duplicata na Lista |
| Falha é alteração topológica temporária | remoção, cálculo e restauração em `src/simulacao_falha.c` |

Essas decisões descrevem o comportamento implementado na Fase I sem atribuir
às estruturas topológicas informações elétricas que elas não armazenam.
