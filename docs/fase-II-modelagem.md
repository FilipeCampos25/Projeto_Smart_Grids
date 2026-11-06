# Fase II - modelagem de pesos

## Escopo

Esta etapa adiciona pesos sem mudar os CSVs normalizados nem atribuir grandezas
que a fonte nao fornece. A topologia continua sendo um grafo simples, nao
direcionado, com 1.838 vertices e 1.837 arestas no recorte real.

O peso e metadado da aresta. A existencia da conexao permanece independente do
peso para que tres estados diferentes sejam representaveis:

| Estado | Adjacencia | `possui_peso` | Valor/origem |
|---|---:|---:|---|
| Sem aresta | 0 | nao consultavel | nenhum |
| Aresta sem peso | 1 | 0 | `0.0` / `ORIGEM_PESO_NAO_INFORMADA` |
| Aresta com peso zero | 1 | 1 | `0.0` / medida ou assumida |
| Aresta com peso positivo | 1 | 1 | valor finito / medida ou assumida |

Pesos validos sao `double` finitos e nao negativos. NaN, infinito, valores
negativos e origem nao informada em uma insercao ponderada sao rejeitados sem
alterar o grafo. Nao foi imposto um maximo arbitrario: `DBL_MAX` e valido como
valor armazenado, embora algoritmos futuros devam detectar overflow ao somar
pesos.

## Origem e unidade

`ORIGEM_PESO_COMPRIMENTO_MEDIDO` significa que o valor veio diretamente de
`arestas.csv:comprimento_m`, cuja origem e o campo `COMP` da BDGD. A unidade e
metro. No recorte atual, os 1.533 segmentos de media tensao possuem comprimento;
minimo 2,24 m, maximo 435,06 m e soma 104.500,97 m.

As 304 chaves nao possuem comprimento. Para elas, a configuracao oferece dois
modos:

- `PESO_AUSENTE_MANTER_SEM_PESO`: conserva a aresta, mas sem peso;
- `PESO_AUSENTE_USAR_CUSTO_ASSUMIDO`: aplica o `custo_assumido` informado pelo
  chamador e registra `ORIGEM_PESO_CUSTO_ASSUMIDO`.

O custo assumido e um custo abstrato de algoritmo, configuravel, sem unidade
fisica. Ele nao representa metro, resistencia, impedancia, tempo de manobra,
perda eletrica ou valor financeiro. Zero e permitido e continua marcado como
assumido, portanto nao se confunde com ausencia de dado.

Exemplo de configuracao explicita:

```c
ConfiguracaoPesosDataset configuracao = {
    PESO_AUSENTE_USAR_CUSTO_ASSUMIDO,
    0.25
};
```

O valor acima e apenas um exemplo de entrada do chamador, nao um valor recomendado
nem uma propriedade da rede real.

## APIs e compatibilidade

As funcoes antigas `construir_lista_dataset` e `construir_matriz_dataset`
continuam criando a topologia da Fase I sem pesos. As novas funcoes
`construir_lista_ponderada_dataset` e `construir_matriz_ponderada_dataset`
exigem uma `ConfiguracaoPesosDataset`.

Na lista, cada copia simetrica do vizinho guarda o mesmo `PesoAresta`. Na matriz,
adjacencia continua em `unsigned char`; pesos e origens ficam em vetores paralelos
alocados somente na primeira insercao ponderada. Por isso, matrizes usadas apenas
pela Fase I mantem o armazenamento topologico. O teto fornecido a `criar_matriz`
tambem limita a posterior alocacao dos vetores ponderados; use
`estimar_memoria_matriz_ponderada` quando pesos forem necessarios.

Os nos da lista agora reservam metadados de peso mesmo para uma aresta sem peso.
Assim, os algoritmos e contratos da Fase I permanecem iguais, mas uma nova
execucao da metrica de memoria da lista nao deve ser comparada diretamente aos
valores historicos anteriores a esta modelagem.

Remover uma aresta elimina seu peso. A simulacao de falha e uma excecao
controlada: captura valor e origem antes da remocao temporaria e os usa na
restauracao. Assim, a contingencia nao converte peso medido em assumido nem
transforma uma aresta ponderada em aresta sem peso.

## Integridade dos dados

O carregador continua rejeitando referencias desconhecidas, lacos, IDs e pares
duplicados, comprimentos negativos, NaN e infinito. Campos de comprimento vazios
sao preservados com `possui_comprimento=0`. Os CSVs reais nao foram alterados.

Nao existem nos artefatos atuais campos de resistencia, capacidade, impedancia,
tempo ou custo financeiro. Obter qualquer dessas grandezas exigira nova fonte,
rastreabilidade do campo e unidade documentada; elas nao podem ser derivadas
somente do comprimento.

## Algoritmos de arvore geradora minima

Prim e Kruskal estao implementados em `include/agm.h` e `src/agm.c`; contratos,
complexidades, politica para pesos ausentes, exemplos e validacoes estao em
[docs/agm.md](./agm.md).

## Limitacoes para algoritmos futuros

- Dijkstra ainda deve definir sua politica para arestas sem peso; nao deve
  converte-las implicitamente em zero. AGM retorna status especifico.
- Somar valores validos individualmente ainda pode exceder `DBL_MAX`; o algoritmo
  deve detectar resultado nao finito.
- A rede real normalizada e uma arvore. Sua AGM coincide com a propria rede e o
  caminho entre dois vertices e topologicamente unico, por isso grafos sinteticos
  ciclicos continuam necessarios para validar algoritmos de otimizacao.
- Custos assumidos devem aparecer nos resultados separados de medidas reais.
