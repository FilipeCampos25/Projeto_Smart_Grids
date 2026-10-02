# Simulação de falha e ilhas topológicas — F1-09 / issue #10

## Objetivo e limite da análise

O módulo simula a indisponibilidade temporária de uma conexão da rede e compara
os componentes conexos antes e depois. O componente que contém o índice informado
como subestação é a região principal. Todo componente fora dele é chamado de
**ilha topológica**.

Essa classificação mede somente conectividade. Ela não afirma que a ilha é
eletricamente autossuficiente: seriam necessários dados de geração, consumo,
capacidade e demanda, que não fazem parte da Fase I.

## Reutilização das implementações existentes

Não há uma segunda implementação de grafo, BFS ou componentes conexos. A
simulação usa:

- `remover_aresta_lista` / `remover_aresta` para representar a falha;
- `identificar_componentes_conexos_lista` ou
  `identificar_componentes_conexos_matriz` para os três retratos;
- `inserir_aresta_lista` / `inserir_aresta` para restaurar a conexão;
- a BFS já reutilizada internamente pelo módulo de componentes conexos.

Uma pequena tabela interna de operações permite compartilhar todo o fluxo entre
lista e matriz. Somente os adaptadores das APIs públicas variam.

## Fluxo

`simular_falha_aresta_lista` e `simular_falha_aresta_matriz` executam:

1. validam grafo, vértices, subestação, saída e existência da aresta;
2. calculam `componentes_antes`;
3. removem temporariamente a aresta;
4. calculam `componentes_depois`;
5. localizam a componente que contém `subestacao`;
6. contam todos os vértices fora da região principal e aqueles que foram
   separados dela especificamente por essa falha;
7. reinserem a aresta;
8. calculam `componentes_apos_restauracao`;
9. confirmam que a aresta voltou e que a partição original foi recuperada.

O `ResultadoSimulacaoFalha` conserva os três retratos, a aresta, a subestação,
as contagens e os indicadores de separação/restauração. Ele independe do tempo de
vida do grafo e deve ser liberado com `liberar_resultado_simulacao`.

`imprimir_resultado_simulacao` produz um registro textual com a aresta desativada,
quantidade e membros das componentes, tamanho da região principal, isolados e
estado da restauração.

## Erros tratados

- argumento ou dono de saída inválido;
- subestação ausente (`SIZE_MAX`);
- vértice ou subestação fora da faixa;
- aresta inexistente ou já removida/desativada;
- falta de memória e limite de tamanho propagados de componentes/BFS;
- falha ao reinserir a aresta na lista.

Na lista, restaurar uma conexão exige alocar novamente os dois nós de vizinhança.
Se essa alocação falhar, a API retorna `SIMULACAO_FALHA_RESTAURACAO`; nesse caso
o chamador deve considerar a conexão ainda ausente. Erros posteriores à remoção
tentam restaurar a conexão antes de retornar.

## Graphviz

As funções `exportar_simulacao_dot_lista` e
`exportar_simulacao_dot_matriz` escrevem DOT para as etapas:

- `REDE_ANTES_FALHA`;
- `REDE_DEPOIS_FALHA`;
- `REDE_APOS_RESTAURACAO`.

Na etapa posterior à falha, a subestação usa círculo duplo, a componente principal
fica verde, ilhas ficam vermelhas e a conexão desativada aparece tracejada. A API
recebe um `FILE *`, portanto o programa chamador escolhe os nomes dos três arquivos.
Exemplo:

```c
FILE *dot = fopen("rede_depois.dot", "w");
if (dot != NULL) {
    exportar_simulacao_dot_lista(grafo, resultado, REDE_DEPOIS_FALHA, dot);
    fclose(dot);
}
```

O algoritmo não chama nem depende do executável `dot`. Se Graphviz estiver
instalado, uma imagem pode ser produzida separadamente:

```sh
dot -Tpng rede_depois.dot -o rede_depois.png
```

No ambiente desta entrega, `dot` não estava instalado. A geração textual DOT foi
testada; nenhum binário foi adicionado ao repositório.

## Teste controlado

Grafo usado nas duas representações:

```text
0 -- 1 -- 2 -- 3
          |
          4 -- 5
```

Subestação: `0`. Aresta desativada: `1 -- 2`.

### Resultado esperado

- antes: 1 componente com os 6 vértices;
- depois: componente principal `{0,1}` e ilha `{2,3,4,5}`;
- depois: 2 componentes e 4 vértices isolados;
- após restaurar: 1 componente e aresta `1 -- 2` presente.

### Resultado obtido

Lista e matriz produziram a mesma partição lógica:

```text
Componentes antes: esperado=1 obtido=1 PASSOU
Componentes depois: esperado=2 obtido=2 PASSOU
Vertices na regiao principal: esperado=2 obtido=2 PASSOU
Vertices isolados: esperado=4 obtido=4 PASSOU
Componentes apos restauracao: esperado=1 obtido=1 PASSOU
Topologia restaurada: esperado=1 obtido=1 PASSOU
```

A ordem dos membros pode variar entre lista e matriz, mas a partição é igual.

## Compilação e validação

Comandos executados no Windows com Clang 23.1.2 e GNU Make 4.4.1:

```powershell
mingw32-make CC=clang BUILD_DIR=build/issue10 test-simulacao
mingw32-make CC=clang BUILD_DIR=build/issue10-regressao test
mingw32-make CC=clang BUILD_DIR=build/issue10-sanitize sanitize
```

A suíte normal da simulação realizou 54 verificações e a instrumentada realizou
153, ambas com zero falhas. Foram exercitados 19 pontos de falha de alocação,
sem blocos vivos e com a aresta restaurada após cada erro. O alvo geral confirmou
lista, matriz, BFS, DFS, componentes conexos e simulação.

## Dataset real

**Não executado / pendente.** O repositório ainda não contém arquivos do dataset
real nem o carregador previsto na issue #4. Não foi inventado um dataset e o grafo
controlado não foi apresentado como validação real.
