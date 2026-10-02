# Desenvolvimento e validação da lista de adjacência

## Estado inspecionado e escopo

A [issue #5 — F1-04](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/5)
e as issues #1 a #4 foram lidas integralmente via API do GitHub em 01/10/2026;
todas estavam abertas, sem comentários. A única branch remota era `main`, em
`2e0eba72b3b19a4911c6cb9de2838f47093118bd`. O checkout local estava limpo na branch
`feature/issue-5-lista-adjacencia`, nesse mesmo commit. Não foram encontrados
`AGENTS.md` aplicáveis. Havia somente README, `.gitignore` e seis documentos vazios;
não existiam estruturas, headers, build, testes, dependências C, parser ou dataset.

A entrega implementa o núcleo autoral em C11 e seus testes. As hipóteses locais e
os limites da integração estão em [modelagem-grafo.md](modelagem-grafo.md).
BFS, DFS, matriz, parser e seleção de dados continuam fora desta issue.

## Organização e funcionamento

| Arquivo | Responsabilidade |
| --- | --- |
| `include/grafo_lista.h` | Contratos públicos, tipos opacos e códigos de retorno |
| `src/grafo_lista.c` | Vetor de listas encadeadas, inserção, consulta, remoção e liberação |
| `tests/test_grafo_lista.c` | Comparações programáticas, exemplos e injeção de falhas |
| `tests/alocacao_teste.h` | Declarações de alocadores exclusivos do teste instrumentado |
| `Makefile` | Compilação normal, testes e sanitizadores |

O vetor armazena a cabeça da lista de cada vértice. A conexão `(0,1)` cria um nó
com índice 1 na lista de 0 e outro com índice 0 na lista de 1. A contagem de arestas
cresce **uma única vez**. Antes de ligar os nós às listas, a inserção aloca ambos.
Se a segunda alocação falhar, libera a primeira e preserva as vizinhanças.

O vetor cresce geometricamente conforme novos vértices são inseridos. A reserva
usa um ponteiro temporário para preservar a memória anterior caso `realloc` falhe,
e verifica o limite de tamanho antes da multiplicação. Só posições correspondentes
a vértices inseridos são acessadas. Não há limite fixo de 1.000 vértices.

A remoção localiza os elos das duas listas antes de alterá-las, desliga ambos os
nós e os libera. Funciona na cabeça, no meio e no fim de uma lista. Uma conexão
removida não aparece na iteração. Reinseri-la restaura sua presença nos dois lados.

| Operação | Custo |
| --- | --- |
| Criar grafo; consultar contagens | O(1) |
| Inserir vértice | O(1) amortizado; O(V) quando amplia o vetor |
| Iniciar consulta; avançar para próximo vizinho | O(1) |
| Percorrer todos os vizinhos de `v` | O(grau(v)) |
| Consultar conexão; inserir com verificação de duplicata | O(grau(origem)) |
| Remover conexão | O(grau(origem) + grau(destino)) |
| Liberar grafo; memória ocupada | O(V + E) |

A capacidade do vetor acompanha V geometricamente e cada aresta ocupa dois nós.
Não há cópia da vizinhança nem vetor temporário para a consulta.

## Uso da API

As funções públicas usam o sufixo `_lista` para evitar colisão com a futura matriz.
O header documenta parâmetros, retornos, casos especiais e propriedade da memória.

```c
#include "grafo_lista.h"
#include <stdio.h>

int exemplo(void)
{
    GrafoLista *grafo = criar_grafo_lista();
    size_t origem;
    size_t destino;
    const VizinhoLista *vizinho;

    if (grafo == NULL) {
        return 1;
    }
    if (inserir_vertice_lista(grafo, &origem) != LISTA_SUCESSO ||
        inserir_vertice_lista(grafo, &destino) != LISTA_SUCESSO ||
        inserir_aresta_lista(grafo, origem, destino) != LISTA_SUCESSO ||
        buscar_vizinhos_lista(grafo, origem, &vizinho) != LISTA_SUCESSO) {
        liberar_grafo_lista(grafo);
        return 1;
    }
    for (; vizinho != NULL; vizinho = proximo_vizinho_lista(vizinho)) {
        printf("%zu\n", indice_vizinho_lista(vizinho));
    }
    liberar_grafo_lista(grafo);
    return 0;
}
```

BFS/DFS poderão consumir os índices usando essa mesma iteração e seus próprios
vetores de visitados. A lista é emprestada e não promete ordem. O chamador não
libera os vizinhos e deve encerrar a iteração antes de modificar ou liberar o grafo.
Não é uma API com sincronização para múltiplas threads.

O módulo valida ponteiros nulos e limites dos índices. Índices são `size_t`;
valores negativos vindos de arquivos devem ser rejeitados pelo carregador antes
da conversão. Um índice equivalente a `SIZE_MAX` é rejeitado nos grafos testados.
Erros de inserção e remoção preservam o grafo; erros de consulta preservam a saída.
Os retornos são `LISTA_SUCESSO`, `LISTA_ARGUMENTO_INVALIDO`, `LISTA_SEM_MEMORIA`,
`LISTA_ARESTA_DUPLICADA`, `LISTA_LACO_NAO_PERMITIDO`, `LISTA_ARESTA_INEXISTENTE`
e `LISTA_LIMITE_EXCEDIDO`.

## Compilar e executar

Requisitos: compilador C11 e GNU Make; os alvos usam shell POSIX, disponível em
Linux e no MSYS2. Não há bibliotecas de grafos ou pacotes de teste externos.
Execute os comandos na raiz do repositório.

Linux ou terminal MSYS2 UCRT64:

```sh
make all
make test test-falhas
```

Comandos exatos usados no PowerShell deste ambiente, com GCC 16.2.0 do MSYS2:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
make all
make test test-falhas
```

O alvo normal executou estas linhas de compilação no Windows:

```sh
gcc -Iinclude -std=c11 -Wall -Wextra -Wpedantic -g -c src/grafo_lista.c -o build/grafo_lista.o
gcc -Iinclude -std=c11 -Wall -Wextra -Wpedantic -g tests/test_grafo_lista.c build/grafo_lista.o -o build/teste_grafo_lista.exe
```

O executável pode ser chamado diretamente no PowerShell:

```powershell
.\build\teste_grafo_lista.exe
.\build\teste_grafo_lista_falhas.exe
```

`test-falhas` compila um objeto separado do módulo, substituindo somente nele
`malloc`, `realloc` e `free` pelos alocadores do teste. A biblioteca padrão usada
pelo próprio teste não é substituída. Não há configuração de alocadores na API
de produção. Todos os binários e objetos ficam em `build/`, ignorado pelo Git.

Para AddressSanitizer, UndefinedBehaviorSanitizer e detecção de vazamentos, use
um GCC/Clang com os respectivos runtimes, por exemplo no Linux:

```sh
make sanitize
```

O GCC MinGW instalado não tinha esses runtimes. A validação foi realizada no
Ubuntu/WSL, com GCC 13.3.0, usando exatamente:

```powershell
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/Users/jotar/OneDrive/Desktop/ProjetoSeleniumPython/Projeto_Smart_Grids && make BUILD_DIR=build/linux sanitize'
```

O alvo usa `-fsanitize=address,undefined -fno-omit-frame-pointer`,
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` e `UBSAN_OPTIONS=halt_on_error=1`.
Os objetos instrumentados ficam separados dos normais. Ao trocar compilador ou
flags manualmente, use um novo `BUILD_DIR` para evitar reaproveitar objetos antigos.

## Testes e resultados

Cada comparação imprime esperado/obtido quando exibida ou quando falha. As
vizinhanças do exemplo são sempre exibidas; as mil vizinhanças da cadeia são
comparadas individualmente, com resumo e detalhes em caso de divergência.
Os testes imprimem `PASSOU`/`FALHOU` e retornam código diferente de zero se houver
qualquer falha. As comparações não dependem de `assert` nem da flag `NDEBUG`.

### Exemplo pequeno obrigatório

Seis vértices e conexões `(0,1), (0,2), (1,3), (2,3), (3,4)`.
Cada célula mostra **esperado / obtido**; a ordem não faz parte do contrato.

| Vértice | Inicial | Após remover (3,4) | Após reinserir (4,3) |
| --- | --- | --- | --- |
| 0 | `{1,2}` / `{2,1}` | `{1,2}` / `{2,1}` | `{1,2}` / `{2,1}` |
| 1 | `{0,3}` / `{3,0}` | `{0,3}` / `{3,0}` | `{0,3}` / `{3,0}` |
| 2 | `{0,3}` / `{3,0}` | `{0,3}` / `{3,0}` | `{0,3}` / `{3,0}` |
| 3 | `{1,2,4}` / `{4,2,1}` | `{1,2}` / `{2,1}` | `{1,2,4}` / `{4,2,1}` |
| 4 | `{3}` / `{3}` | `{}` / `{}` | `{3}` / `{3}` |
| 5 | `{}` / `{}` | `{}` / `{}` | `{}` / `{}` |
| Vértices | 6 / 6 | 6 / 6 | 6 / 6 |
| Arestas | 5 / 5 | 4 / 4 | 5 / 5 |

Resultado: **PASSOU** em todas as comparações, inclusive consulta da conexão
nos dois sentidos e rejeição da segunda remoção.

### Escala sintética e casos especiais

Cadeia com vértices de 0 a 999 e arestas `(i,i+1)` para `0 <= i < 999`:

- Vértices: esperado **1.000**, obtido **1.000**.
- Arestas: esperado **999**, obtido **999**.
- Extremos: 0 tem `{1}`; 999 tem `{998}`. Cada vértice interno tem `{i-1,i+1}`.
- Todas as 1.000 vizinhanças: esperado **zero divergências**, obtido **zero**.
- Resultado: **PASSOU**. A memória foi liberada ao final.

Também são testados grafo vazio, vértices isolados, ponteiros nulos, índices fora
da faixa e `SIZE_MAX`, saídas preservadas em erro, duplicata direta e invertida,
laços, remoção inexistente e remoção na cabeça, meio e cauda de uma lista.

### Falhas de alocação e memória

A versão instrumentada força falhas em cada uma das **25 alocações** de uma cadeia
de 12 vértices e 11 arestas: criação do grafo, reserva inicial, ampliação do vetor
e os dois nós de cada conexão. Para cada ponto, verifica tanto o abandono imediato
com liberação da construção parcial quanto a recuperação por nova tentativa.
A ampliação acontece com arestas já presentes, conferindo sua preservação.

O teste compara vizinhanças, contagens e saídas após o erro e verifica que falhar
no segundo nó não deixa meia aresta nem um bloco perdido. O contador de blocos
vivos deve terminar em zero após cada cenário. Esse contador complementa os
sanitizadores; isoladamente ele não detectaria todo acesso inválido.

Os sanitizadores verificam acessos de memória, comportamento indefinido coberto
pelo UBSan e vazamentos nos caminhos realmente executados, incluindo as falhas
injetadas. Isso não constitui prova sobre todas as possíveis entradas. O limite
aritmético de capacidade é protegido no código, mas não foi atingido artificialmente
com uma alocação de tamanho impraticável.

### Registro final de execução (01/10/2026)

| Ambiente | Teste normal | Teste com falhas | Código de saída |
| --- | --- | --- | --- |
| Windows, GCC 16.2.0 / MSYS2 | PASSOU: 4.130 verificações, zero falhas | PASSOU: 5.587 verificações, zero falhas | 0 |
| Ubuntu/WSL, GCC 13.3.0, ASan + UBSan + leaks | PASSOU: 4.130 verificações, zero falhas | PASSOU: 5.587 verificações, zero falhas | 0 |

As compilações não emitiram avisos com `-Wall -Wextra -Wpedantic`. Não houve
relatórios de acesso inválido, comportamento indefinido ou vazamentos pelos
sanitizadores. Nos testes instrumentados: esperado **zero blocos vivos** ao final,
obtido **zero**. Os totais da segunda coluna instrumentada incluem novamente os
testes funcionais, além dos cenários de falha; não são casos independentes somáveis.

Os logs completos desta execução estão em `build/testes-windows.log` e
`build/testes-sanitizadores.log` (artefatos locais ignorados pelo Git). Este documento
preserva as evidências e os comandos para reproduzi-las.

### Dados reais: pendente

**Não executado:** não havia dataset nem carregador no repositório. Origem,
versão, distribuidora, circuito/recorte, número de vértices e número de arestas
reais não estão definidos/disponíveis para esta validação. A referência à BDGD
no README é uma fonte prioritária em avaliação, não um dataset carregado.
É necessário concluir as dependências #1/#2, #3 e #4 e então executar a integração.
Nenhum resultado sintético foi contabilizado como teste de dados reais.

## Critérios da issue #5

| Critério | Estado e evidência |
| --- | --- |
| Estrutura implementada em C | Atendido: módulo C11 compilado em GCC no Windows e Linux |
| Pelo menos 1.000 vértices | Atendido: cadeia 1.000/999, todas as vizinhanças verificadas |
| Inserção de arestas | Atendido: exemplo e cadeia; falha de alocação preserva os dois lados |
| Consulta eficiente de vizinhos | Atendido: início O(1), iteração O(grau), sem cópia |
| Grafo não direcionado | Atendido: vizinhanças simétricas e contagem única por conexão |
| Identificar/desativar conexão | Atendido por consulta e remoção bilateral; restauração testada |
| Funciona com dataset carregado | **Pendente:** dataset, modelo definitivo e carregador ausentes |
| Memória liberada corretamente | Atendido nos testes: liberação normal/parcial, contador e sanitizadores |
| Sem biblioteca pronta de grafos | Atendido: implementação autoral usando apenas biblioteca padrão C |
| Teste simples executado | Atendido: exemplo obrigatório com comparações de conjuntos |

A issue não deve ser considerada integralmente concluída enquanto a integração
real permanecer pendente e as hipóteses locais não forem conciliadas com a #3.
