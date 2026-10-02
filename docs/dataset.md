# Dataset real da Fase I

## Fonte e versão

O recorte vem da **Base de Dados Geográfica da Distribuidora (BDGD)**,
publicada pela Agência Nacional de Energia Elétrica (ANEEL) no Portal de Dados
Abertos. A BDGD representa de forma simplificada o sistema elétrico real
informado pelas distribuidoras.

- Órgão: ANEEL — Superintendência de Regulação dos Serviços de Transmissão e
  Distribuição de Energia Elétrica.
- Portal: <https://dadosabertos.aneel.gov.br/dataset/base-de-dados-geografica-da-distribuidora-bdgd>
- Item oficial ArcGIS: `cd6c8ecb86c2427493cf4a0d129c2eb4`.
- URL de obtenção usada pelo script:
  <https://www.arcgis.com/sharing/rest/content/items/cd6c8ecb86c2427493cf4a0d129c2eb4/data>
- Distribuidora: FORCEL (código 83).
- Posição dos dados: 31/12/2025.
- Arquivo original: `Forcel_83_2025-12-31_V11_20260823-1832.gdb.zip`.
- Circuito: `1_SFOR_1`, “ALIMENTADOR 1 - STA TEREZINHA ZONA NORTE”.
- Subestação: `SFOR`; ponto inicial do circuito: `1_SFOR_1`.
- Licença informada pelo portal: Open Data Commons Open Database License.

O Geodatabase oficial tem aproximadamente 3,05 MB. Ele não é versionado; o
repositório contém somente o recorte CSV normalizado (cerca de 200 KB) e o
script capaz de reproduzi-lo.

## Entidades e números

No arquivo original foram inspecionados 5.051 `SSDMT`, 980 `UNSEMT`, 1
`UNREMT`, 12 `UNCRMT`, 10 `CTMT`, 8.503 `PONNOT`, 755 `UNTRMT` e uma `SUB`.
O circuito escolhido possuía 1.533 segmentos `SSDMT` e 311 chaves/equipamentos
de ligação candidatos. Foram excluídas chaves normalmente abertas (`P_N_OPE=A`)
e dois self-loops. O componente ativo que contém `PAC_INI` resultou em:

| Item | Quantidade |
|---|---:|
| Vértices (pontos de conexão) | 1.838 |
| Arestas ativas | 1.837 |
| Segmentos MT | 1.533 |
| Chaves MT fechadas | 304 |
| Comprimento informado nos segmentos | 104.500,97 m |
| Componentes no recorte final | 1 |

## Transformações

1. As camadas `SSDMT`, `UNSEMT`, `UNREMT` e `UNCRMT` são filtradas por
   `CTMT=1_SFOR_1`.
2. Unidades não ativas e chaves normalmente abertas são descartadas.
3. `PAC_1` e `PAC_2` formam os extremos das conexões.
4. Self-loops e pares duplicados são rejeitados.
5. Mantém-se somente o componente que contém o `PAC_INI` real do circuito.
6. Os vértices são ordenados por BFS determinística (vizinhos em ordem textual),
   começando na subestação. Essa ordem também define os prefixos experimentais.
7. São preservados ID do ativo, tipo, circuito, status e comprimento quando
   disponível. Os atributos não são usados como peso nesta fase.

Arquivos gerados:

- `data/forcel_2025_circuito_1/vertices.csv`:
  `id,tipo,subestacao,circuito`;
- `data/forcel_2025_circuito_1/arestas.csv`:
  `id,origem,destino,tipo,comprimento_m,status,circuito`.

## Reprodução

```sh
python -m pip install geopandas pyogrio
python scripts/preparar_bdgd.py
```

Também é possível usar um ZIP já baixado:

```sh
python scripts/preparar_bdgd.py --origem caminho/Forcel_2025.gdb.zip
```

Python/GDAL são necessários apenas para preparar os CSVs. O carregamento e
todo processamento do grafo são realizados em C, sem NetworkX ou biblioteca
pronta de grafos.
