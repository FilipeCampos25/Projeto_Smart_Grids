# Busca em largura — F1-06 / issue #7

## Inspeção, dependências e escopo

Em 01/10/2026 foram lidas integralmente a [issue #7](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/7),
a [#5 (lista)](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/5),
a [#6 (matriz)](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/6)
e a [#4 (carregador)](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/4),
inclusive os endpoints de comentários: nenhuma tinha comentários. #5 e #6 estavam
fechadas; #4 e #7, abertas.

O checkout estava limpo na branch `feat/issue-7-bfs`, commit `acde247`, já com a
lista integrada. A matriz também já havia sido integrada à `main` remota, em
`8007383`. Após inspecionar o histórico, a branch local avançou por fast-forward
até esse commit para reutilizar a implementação real. Não foi criada uma matriz
provisória, convertida a lista nem inventada uma API para a dependência.
Não foram encontrados `AGENTS.md` aplicáveis.

O módulo preserva `grafo_lista.h` e `matriz_adjacencia.h`, seus fontes e testes.
O Makefile recebido só executava os testes da matriz; a suíte conjunta agora
executa também os testes já existentes da lista e os novos testes da BFS.

**BFS em lista e matriz: implementada e testada também com dataset real.**
O carregador traduz IDs externos para índices internos e compartilha o mesmo
mapeamento nas duas representações; veja `docs/dataset.md`.

## Arquivos e API

| Arquivo | Função |
| --- | --- |
| `include/bfs.h` | Contrato público, códigos de retorno e estrutura de resultado |
| `src/bfs.c` | Duas buscas, inicialização, fila e liberação |
| `tests/test_bfs.c` | Exemplos conhecidos, escala, erros e falhas de alocação |
| `Makefile` | Integração das seis suítes de testes |

As funções públicas são:

- `executar_bfs_lista(grafo, origem, &resultado)`: usa a iteração de vizinhos da lista.
- `executar_bfs_matriz(matriz, origem, &resultado)`: consulta os destinos da linha atual.
- `liberar_resultado_bfs(&resultado)`: libera os recursos e define o dono como `NULL`.

`ResultadoBfs` contém:

| Campo | Significado |
| --- | --- |
| `quantidade_vertices` | Número de vértices do grafo no momento da busca |
| `quantidade_alcancados` | Quantidade de vértices alcançados, incluindo a origem |
| `ordem_visita` | Vetor com os alcançados nas primeiras `quantidade_alcancados` posições |
| `distancias` | Vetor indexado por vértice, com distância mínima em número de arestas |

`distancias[vertice] == SIZE_MAX` significa **não alcançado/não visitado**. A origem
tem distância zero. As demais posições de `ordem_visita`, além da quantidade
alcançada, não são inicializadas e não devem ser lidas. O header inclui a definição
de `SIZE_MAX`. O resultado é independente do grafo e continua válido após sua
liberação; não é atualizado automaticamente se conexões forem alteradas depois.

A saída deve começar em `NULL`. Em sucesso, o chamador recebe a propriedade de
um resultado e seus dois vetores. Deve liberá-los pela função pública, sem mudar
ponteiros/contagens nem liberar vetores separadamente. A liberação aceita `NULL`,
zera o dono e pode ser repetida sobre esse dono. Cópias de ponteiros antigos não
podem ser utilizadas depois.

| Retorno | Quando ocorre |
| --- | --- |
| `BFS_SUCESSO` (0) | Busca concluída e resultado entregue |
| `BFS_ARGUMENTO_INVALIDO` (1) | Grafo/saída nulos, saída já ocupada ou origem fora do intervalo |
| `BFS_SEM_MEMORIA` (2) | Alguma das três alocações falhou |
| `BFS_LIMITE_EXCEDIDO` (3) | Tamanho de um vetor não cabe em `size_t` |

Um grafo vazio não possui origem válida, portanto retorna argumento inválido.
Todos os erros preservam a saída do chamador. A construção parcial é liberada
internamente. O grafo deve permanecer válido e sem modificações durante a chamada;
não há sincronização para alterações concorrentes.

## Como o algoritmo funciona

### Reutilização pelos componentes conexos (#9)

A API pública e as três alocações por chamada permanecem iguais. A inicialização
foi extraída para `criar_busca_bfs`, e os laços existentes de travessia para
`continuar_bfs_lista`/`continuar_bfs_matriz`, declarados somente no header interno
`src/bfs_interno.h`. As funções públicas continuam validando entradas, criando
uma busca nova e executando apenas a componente da origem.

O módulo de componentes cria a busca uma vez e continua somente a partir de
origens não visitadas. Cada continuação começa a fila no antigo final de
`ordem_visita`, preserva as distâncias/marcações anteriores e acrescenta somente
os novos vértices. Não há inicialização de V posições nem alocação por componente.
As distâncias internas partem de zero em cada nova origem; componentes não as
expõe como distâncias globais. Não há segunda implementação da BFS.
Veja [componentes-conexos.md](componentes-conexos.md) para o contrato e a validação
conjunta, incluindo a transferência da ordem para a lista de membros.

### Busca pública a partir de uma origem

1. Reserva o resultado, a ordem/fila e as distâncias; inicializa as distâncias em
   `SIZE_MAX`, indicando que nenhum vértice foi visitado.
2. Marca a origem com distância zero e a coloca no fim da fila.
3. Retira o vértice de `ordem_visita[inicio_fila]` e incrementa `inicio_fila`.
4. Para cada vizinho presente ainda não visitado, marca a distância como
   `distancias[vertice_atual] + 1` **antes de enfileirar**.
5. Termina quando `inicio_fila == quantidade_alcancados`. Não inicia outra componente.

A fila FIFO autoral usa o próprio vetor `ordem_visita`. Seu início avança e seu
fim é `quantidade_alcancados`; os itens retirados permanecem no vetor para serem
entregues como ordem de visita. Não são necessários deslocamentos, lista de nós,
fila circular ou uma cópia adicional do resultado. Cada vértice entra no máximo
uma vez, então a capacidade de V posições é suficiente, mesmo em ciclos e laços.

O vetor de distâncias também registra os visitados, evitando um segundo vetor
com a mesma informação. Marcar ao enfileirar impede que dois vizinhos descubram
e coloquem o mesmo vértice na fila antes de ele ser retirado.

Como a fila retira os elementos na ordem de entrada, todos os vértices da camada
anterior são processados antes dos da próxima. A primeira descoberta fornece a
menor quantidade de arestas desde a origem. No exemplo, os vértices 1 e 2 precisam
aparecer antes de 3; não basta conferir apenas o conjunto final.

## Integração com as representações

A lista fornece `buscar_vizinhos_lista`, `indice_vizinho_lista` e
`proximo_vizinho_lista`; a BFS não conhece seus nós ou alocações internas.
A matriz fornece `quantidade_vertices_matriz` e `consultar_adjacencia`; a BFS
examina os destinos de 0 a V−1 para cada vértice retirado da fila.

Não há ordenação de vizinhos na busca. A lista não promete desempate crescente;
a matriz enumera destinos em ordem crescente. Os testes comparam alcançabilidade,
quantidade e distâncias, permitindo ordens diferentes dentro da mesma camada.

Os contratos existentes diferem: a lista rejeita laços e duplicatas; a matriz
aceita laços e trata duplicatas de forma idempotente. A BFS respeita essas decisões
sem alterá-las; um laço da matriz não reenfileira seu vértice. As comparações das
duas representações usam os mesmos pares válidos em ambas. A compatibilização da
política de domínio ainda pertence à modelagem, não à BFS.

Nas duas APIs, uma conexão desativada é removida fisicamente. Ela não aparece
nas consultas da BFS. Reinserir o par e executar uma **nova** busca restaura a
alcançabilidade correspondente. A BFS não insere nem remove conexões.

| Representação | Tempo | Memória adicional da BFS |
| --- | --- | --- |
| Lista | O(V + E) | O(V) |
| Matriz | O(V²) no pior caso | O(V) |

Mais precisamente, a inicialização custa O(V); a lista examina apenas as
adjacências da componente alcançada, e a matriz examina V destinos para cada
vértice alcançado. O resultado ocupa `sizeof(ResultadoBfs) + 2*V*sizeof(size_t)`
bytes solicitados, fora metadados do alocador e memória do grafo. Não há limite
fixo de 1.000 vértices. As multiplicações são verificadas antes das alocações.

## Exemplo de chamada

O grafo deve ter sido construído usando uma das APIs existentes:

```c
#include "bfs.h"
#include <stdio.h>

/* O chamador continua sendo dono do grafo; esta funcao possui o resultado. */
int mostrar_alcancados(const GrafoLista *grafo, size_t origem)
{
    ResultadoBfs *resultado = NULL;
    StatusBfs status = executar_bfs_lista(grafo, origem, &resultado);
    if (status != BFS_SUCESSO) {
        return 1;
    }
    for (size_t posicao = 0; posicao < resultado->quantidade_alcancados; posicao++) {
        size_t vertice = resultado->ordem_visita[posicao];
        printf("Vertice %zu, distancia %zu\n", vertice, resultado->distancias[vertice]);
    }
    liberar_resultado_bfs(&resultado);
    return 0;
}
```

Para uma `MatrizAdjacencia`, use `executar_bfs_matriz` com o mesmo contrato de
resultado. O exemplo não interpreta arquivos ou IDs externos.

## Compilação e execução reproduzíveis

Na raiz, com compilador C11 e GNU Make:

```sh
make CC=gcc all
make CC=gcc test
```

`all` compila seis executáveis; `test` executa matriz, alocação da matriz, lista,
alocação da lista, BFS e alocação da BFS. `make CC=gcc test-bfs` executa somente
as duas suítes BFS; `test-lista` seleciona as duas da lista e `test-falhas` seleciona
as três suítes de falhas de alocação. Não há biblioteca externa de grafos.

Comandos exatos executados no PowerShell, com GCC 16.2.0 e GNU Make 4.4.1 do MSYS2:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
make OS= EXT=.exe CC=gcc BUILD_DIR=build/bfs-windows all
make OS= EXT=.exe CC=gcc BUILD_DIR=build/bfs-windows test
```

`OS=` seleciona as receitas POSIX para o **Make do MSYS2**, e `EXT=.exe` conserva
a extensão dos binários Windows. A variável é passada apenas ao Make. Para o
Make nativo do MinGW, conserve as receitas Windows: `mingw32-make CC=gcc test`.
A primeira tentativa com o Make do MSYS2 e as receitas `cmd.exe` não produziu
binários; ela não foi contabilizada como compilação/teste bem-sucedido.

A linha efetiva de compilação do teste BFS normal foi:

```sh
gcc -Iinclude -std=c11 -Wall -Wextra -Wpedantic -O2 src/bfs.c src/grafo_lista.c src/matriz_adjacencia.c tests/test_bfs.c -o build/bfs-windows/teste_bfs.exe
```

A versão instrumentada substitui `malloc`/`free` somente em um objeto separado
de `src/bfs.c`, reaproveitando `tests/alocacao_teste.h`. As estruturas de grafos e
o próprio teste continuam usando a biblioteca padrão normalmente; a API de
produção não recebe ganchos de teste.

No Ubuntu/WSL, GCC 13.3.0, foram executadas as seis suítes com sanitizadores:

```powershell
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/Users/jotar/OneDrive/Desktop/ProjetoSeleniumPython/Projeto_Smart_Grids && mkdir -p build/bfs-linux && ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 make CC=gcc BUILD_DIR=build/bfs-linux sanitize > build/bfs-linux/sanitizadores.log 2>&1'
```

O alvo aplica `-fsanitize=address,undefined`, `-fno-omit-frame-pointer` e
`-fno-sanitize-recover=all`, em diretório separado. O GCC MinGW deste ambiente não
disponibilizava os runtimes necessários, por isso a instrumentação foi feita no WSL.
Ao trocar flags/compilador, use outro `BUILD_DIR` para evitar objetos antigos.

## Resultados executados em 01/10/2026

### Pequeno obrigatório: 6 vértices, 5 arestas

Arestas: `(0,1), (0,2), (1,3), (2,3), (3,4)`; 5 é isolado.
`{1,2}` na coluna esperada significa qualquer ordem entre esses vértices na mesma camada.

| Caso | Esperado | Obtido na lista | Obtido na matriz |
| --- | --- | --- | --- |
| Origem 0 | `[0,{1,2},3,4]`, quantidade 5 | `[0,2,1,3,4]`, 5 | `[0,1,2,3,4]`, 5 |
| Origem 5 | `[5]`, quantidade 1 | `[5]`, 1 | `[5]`, 1 |
| Removida (3,4), origem 0 | `[0,{1,2},3]`, quantidade 4 | `[0,2,1,3]`, 4 | `[0,1,2,3]`, 4 |
| Restaurada (4,3), origem 0 | Estado inicial | Estado inicial | Estado inicial |
| Origem 4 | `[4,3,{1,2},0]`, quantidade 5 | `[4,3,2,1,0]`, 5 | `[4,3,1,2,0]`, 5 |

**PASSOU.** As camadas a partir de 0 foram `{0}`, `{1,2}`, `{3}`, `{4}`.
Distâncias esperadas/obtidas por índice: `[0,1,1,2,3,SIZE_MAX]`. Após remover a
conexão: `[0,1,1,2,SIZE_MAX,SIZE_MAX]`. Nenhuma busca alcançou vértices indevidos.
Os 36 pares de adjacência foram conferidos contra o grafo conhecido após as
buscas: esperado zero alterações, obtido zero, nas duas representações.

### Escala e outros casos sintéticos

| Caso nas duas representações | Esperado | Obtido |
| --- | --- | --- |
| Cadeia de 1.000 vértices, 999 arestas; origem 0 | 1.000 alcançados; distância de i igual a i | Correspondência completa |
| Estrela de 1.000 vértices, 999 arestas; centro 0 | 1.000 alcançados; folhas na camada 1 | Correspondência completa |
| Componentes 0–1–2 e 3–4–5; origem 4 | Somente `{3,4,5}`, quantidade 3 | Lista `[4,5,3]`; matriz `[4,3,5]` |
| Grafo unitário, inclusive laço na matriz | `[0]`, sem repetição | `[0]` em ambas |
| Origem igual a V ou `SIZE_MAX`; vazio; ponteiros nulos | `BFS_ARGUMENTO_INVALIDO` e saída preservada | Correspondência completa |
| Saída já ocupada | Rejeitar sem sobrescrever/vazar resultado anterior | Correspondência completa |
| Resultado após liberar o grafo | Dados ainda válidos | Correspondência completa |

**PASSOU.** São comparados conjuntos, quantidades, unicidade, todas as distâncias
e a ordem não decrescente das camadas. A estrela exercita 999 entradas pendentes
na fila. A cadeia exercita profundidade 999. Os oráculos são constantes/fórmulas
dos grafos conhecidos, não outra BFS. Não se exige o mesmo desempate entre APIs.

### Falhas de alocação, memória e regressão

Foram forçadas falhas nas três alocações da BFS (resultado, ordem/fila e distâncias)
em cada representação, totalizando **seis cenários de falha**. Em cada um:

- Esperado `BFS_SEM_MEMORIA`, obtido `BFS_SEM_MEMORIA`.
- Esperado saída `NULL`, obtido `NULL`.
- Esperado zero blocos restantes após o erro, obtido zero.
- Esperado topologia preservada, obtido zero divergências.
- Nova tentativa sem falha: sucesso, camadas corretas e zero blocos após liberação.

| Suíte | Windows/GCC 16.2.0 | WSL/GCC 13.3.0 + sanitizadores |
| --- | --- | --- |
| BFS normal | PASSOU: 8.331 verificações, zero falhas | PASSOU: 8.331 verificações, zero falhas |
| BFS com falhas | PASSOU: 8.471 verificações, zero falhas | PASSOU: 8.471 verificações, zero falhas |
| Lista normal | PASSOU: 4.130 verificações, zero falhas | PASSOU: 4.130 verificações, zero falhas |
| Lista com falhas | PASSOU: 5.587 verificações, zero falhas | PASSOU: 5.587 verificações, zero falhas |
| Matriz normal | PASSOU: zero falhas | PASSOU: zero falhas |
| Alocação da matriz | PASSOU: zero falhas | PASSOU: zero falhas |

Os testes instrumentados incluem novamente seus testes funcionais: os totais não
representam conjuntos disjuntos de casos. Todos os processos retornaram **0**.
Não houve avisos de compilação com `-Wall -Wextra -Wpedantic`, nem relatórios de
acessos inválidos, comportamento indefinido ou vazamentos pelos sanitizadores.
A evidência cobre os caminhos executados, não todas as entradas possíveis. O
limite aritmético é protegido no código, sem tentar construir um grafo de tamanho
impraticável para atingi-lo em teste.

Os logs locais completos estão em `build/bfs-windows/testes.log` e
`build/bfs-linux/sanitizadores.log`, ignorados pelo Git. Os executáveis imprimem
esperado/obtido e `PASSOU`/`FALHOU`, e retornam `EXIT_FAILURE` se uma comparação
falhar. Não dependem de `assert` ou de `NDEBUG`.

### Dataset real

**PASSOU:** partindo de `PAC_INI=1_SFOR_1`, a BFS alcançou os 1.838 vértices do
circuito FORCEL/BDGD tanto na Lista quanto na Matriz. O teste compara o conjunto
alcançado, sem exigir a mesma ordem de desempate dos vizinhos.

## Critérios de aceitação da issue #7

| Critério | Estado e evidência |
| --- | --- |
| BFS autoral em C | Atendido: `src/bfs.c`, C11, apenas biblioteca padrão |
| Fila adequada | Atendido: FIFO em vetor de V posições, O(1) por entrada/retirada |
| Lista de Adjacência | Atendido: API real da #5, pequeno e escala executados |
| Matriz de Adjacência | Atendido: API real da #6, pequeno e escala executados |
| Não visita repetidamente | Atendido: marcação ao enfileirar; ciclos, laço e unicidade testados |
| Alcançáveis corretos | Atendido: conjunto exato, quantidade e distâncias comparados |
| Grafos desconectados | Atendido: isolado e duas componentes com arestas, sem reinício |
| Pelo menos 1.000 vértices | Atendido: cadeia e estrela em ambas as representações |
| Grafo pequeno conhecido | Atendido: exemplo obrigatório, camadas e remoção/restauração |
| Dataset real | Atendido: FORCEL/BDGD, 1.838 vértices nas duas representações |

A integração e a validação real estão concluídas; as políticas de IDs, laços,
duplicatas e estados estão em `docs/modelagem-grafo.md`.
