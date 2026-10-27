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

## Rastreabilidade da estrutura (SPIKE-01)

As conclusões abaixo usam três classes de evidência, para não confundir o que
está versionado com uma suposição sobre a BDGD completa:

- **observado no recorte versionado:** conteúdo dos dois CSVs;
- **observado na implementação versionada:** campos e filtros explícitos em
  `scripts/preparar_bdgd.py`, além das validações de `src/dataset.c` e dos
  casos definidos em `tests/test_dataset.c`;
- **registro da auditoria F1-01:** contagens e reprodução do GDB registradas na
  seção final deste documento. O GDB não está no repositório e, nesta revisão,
  uma nova obtenção não foi concluída; portanto, não se atribuem ao arquivo
  original verificações que só podem ser feitas nos CSVs.

### Entidades e relacionamentos efetivamente usados

| Entidade/camada | Papel no processo | Resultado no CSV final |
|---|---|---|
| `CTMT` | Seleciona `COD_ID=1_SFOR_1`; fornece `SUB=SFOR` e `PAC_INI=1_SFOR_1` | circuito, subestação e vértice inicial |
| `SSDMT` | Segmentos de média tensão filtrados por `CTMT` e, quando disponível, `SIT_ATIV` | 1.533 arestas `segmento_mt` |
| `UNSEMT` | Chaves filtradas por `CTMT`, `SIT_ATIV` e `P_N_OPE` | 304 arestas `chave_mt` fechadas |
| `UNREMT` e `UNCRMT` | Candidatas a reguladores e compensadores, com os mesmos campos de conexão | nenhum registro desses tipos permaneceu no recorte final |
| `PONNOT`, `UNTRMT` e `SUB` | Contadas na auditoria F1-01, mas não lidas pelo normalizador | nenhum campo carregado |

O relacionamento é `CTMT.COD_ID = <camada>.CTMT`. Em cada conexão,
`PAC_1` e `PAC_2` são os identificadores textuais dos extremos. A união dos
extremos do componente alcançável desde `CTMT.PAC_INI` gera `vertices.csv`; as
arestas mantêm esses identificadores em `origem` e `destino`. Para este recorte,
`PAC_INI` e o código do circuito têm o mesmo valor (`1_SFOR_1`), que é marcado
como `tipo=subestacao`; os demais extremos são `ponto_conexao`.

| Campo BDGD | Uso na normalização | Campo normalizado |
|---|---|---|
| `COD_ID` das camadas de conexão | identidade do ativo | `arestas.id` |
| `PAC_1`, `PAC_2` | extremos da conexão | `arestas.origem`, `arestas.destino` e `vertices.id` |
| `CTMT` | filtro e vínculo com o circuito | `circuito` nos dois CSVs |
| `SIT_ATIV` | mantém apenas `AT`, se o campo existir | `status=ativo` |
| `P_N_OPE` | remove chave normalmente aberta (`A`), se existir | não preservado |
| `COMP` | comprimento opcional | `comprimento_m` |
| `CTMT.SUB` | código da subestação | `vertices.subestacao` |
| `CTMT.PAC_INI` | raiz da busca do componente | vértice inicial do recorte |
| `CTMT.NOME` | identificação humana na seleção | não preservado |

### Evidências do recorte normalizado

Auditoria direta dos CSVs versionados em 26/10/2026:

| Verificação | Resultado observado |
|---|---:|
| Registros em `vertices.csv` | 1.838 |
| Registros em `arestas.csv` | 1.837 |
| IDs de vértice duplicados | 0 |
| IDs de aresta duplicados | 0 |
| Pares de extremos duplicados, sem considerar direção | 0 |
| Self-loops | 0 |
| Arestas com extremo ausente de `vertices.csv` | 0 |
| Campos obrigatórios vazios | 0 |
| Circuitos distintos | 1 (`1_SFOR_1`) |
| Subestações distintas nos vértices | 1 (`SFOR`) |

Todos os 1.533 segmentos têm comprimento: mínimo de 2,24 m, máximo de
435,06 m e soma de 104.500,97 m. As 304 chaves não têm comprimento. Nenhum
campo de resistência existe nos CSVs, no modelo `Dataset` ou na seleção de
campos feita pelo normalizador; logo, resistência **não está disponível para a
Fase II com os artefatos atuais**.

A auditoria direta alcançou os 1.838 vértices desde a subestação. O teste
versionado em `tests/test_dataset.c` também exige esse resultado por BFS e DFS,
em lista e matriz, e exige uma única componente. Assim, **o recorte final** é
conexo. Como também há `|E| = |V| - 1`, ele é uma árvore. Isso não comprova que
todo o circuito antes do filtro, nem que a BDGD original, seja conexo: o
normalizador conserva deliberadamente apenas o componente de `PAC_INI` e não
registra quantos componentes ou arestas ficaram fora dele.

### Integridade: normalizador versus carregador C

O normalizador elimina extremos vazios/não textuais, self-loops, pares de
extremos repetidos, ativos inativos, chaves abertas e componentes fora da raiz.
O registro da F1-01 contabilizou dois self-loops e nenhuma duplicata de par na
execução reproduzida. A quantidade de extremos incompletos e de conexões fora
do componente não foi registrada; por isso, permanece desconhecida no arquivo
original.

`src/dataset.c` não lê o GDB; ele exige cabeçalhos e ordem exatos nos CSVs. Na
carga, mapeia IDs textuais por tabela hash, rejeita ID de vértice duplicado,
extremo inexistente, self-loop, aresta cujo status não seja `ativo` e
comprimento malformado ou negativo. Em qualquer falha, libera a construção
parcial. O teste com `ID_INEXISTENTE` comprova a rejeição de referência inválida.

Limites relevantes do carregador, que hoje são cobertos pela preparação e pela
auditoria do recorte, mas não por validação em C:

- não rejeita ID de aresta nem par de extremos duplicado;
- não confere se `circuito` coincide entre vértices e arestas, nem se é vazio
  nas arestas;
- não confere uniformidade de `subestacao` e aceita esse campo vazio;
- aceita mais de um vértice `subestacao` e usa o primeiro encontrado;
- não preserva `P_N_OPE`, `SIT_ATIV` original, `NOME`, geometria ou atributos
  elétricos além do comprimento.

### Campos para a Fase II e hipóteses pendentes

Os campos comprovadamente disponíveis e úteis são: IDs externos, extremos,
tipo do ativo, circuito, subestação, status normalizado e comprimento dos
segmentos. Para pesos elétricos, resistência, impedância, material/bitola do
condutor e tensão nominal exigem nova inspeção das camadas e, possivelmente,
junções com outras entidades da BDGD. Essa possibilidade é uma **hipótese de
evolução**, não uma propriedade confirmada deste recorte nem uma exigência já
suportada pelo carregador.

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

## Verificação da issue F1-01

Auditoria realizada em 26/10/2026 sobre o item oficial e os arquivos
versionados, sem selecionar ou baixar outra base:

- `main`, `origin/main` e o checkout auditado apontavam para o commit
  `f1a29753478790c4eb382172e380afb6f214f23a`;
- o item ArcGIS `cd6c8ecb86c2427493cf4a0d129c2eb4` estava público, identificado
  como File Geodatabase da FORCEL, com 3.050.832 bytes;
- a inspeção do GDB confirmou 5.051 registros em `SSDMT`, 980 em `UNSEMT`,
  1 em `UNREMT`, 12 em `UNCRMT`, 10 em `CTMT`, 8.503 em `PONNOT`, 755 em
  `UNTRMT` e 1 em `SUB`;
- o registro `CTMT=1_SFOR_1` confirmou o nome do alimentador, `SUB=SFOR` e
  `PAC_INI=1_SFOR_1`;
- os CSVs contêm 1.838 vértices e 1.837 arestas ativas, sendo 1.533 segmentos
  MT e 304 chaves MT, sem self-loops, pares duplicados ou extremos ausentes;
- uma nova execução de `scripts/preparar_bdgd.py`, em diretório temporário,
  reproduziu as mesmas contagens, a mesma subestação e o mesmo conteúdo dos
  CSVs versionados. A diferença física observada no Windows limitou-se aos
  terminadores LF/CRLF; após a normalização feita pelo Git, os arquivos foram
  idênticos.

Saída resumida da reprodução:

```text
Vertices: 1838
Arestas: 1837
Subestacao: SFOR (indice externo 1_SFOR_1)
Registros ignorados no recorte: {'lacos': 2, 'duplicatas': 0}
```

Assim, a fonte, a versão, o recorte, as entidades, a referência da subestação,
as contagens e o procedimento de reprodução exigidos pela F1-01 foram
confirmados. Os CSVs permanecem como evidência versionada caso o serviço
externo de obtenção fique temporariamente indisponível.
