# Desenvolvimento — grafos e conectividade

## Simulação de falhas — issue #10 (01/10/2026)

Foram adicionados `include/simulacao_falha.h`, `src/simulacao_falha.c`,
`tests/test_simulacao_falha.c` e `docs/simulacao-falhas.md`. A branch da issue #10
foi avançada diretamente até o commit da dependência #9 antes da implementação.
A simulação compartilha um único fluxo para lista e matriz e chama a API existente
de componentes conexos; não duplica BFS, DFS nem estrutura de grafo.

Comandos executados:

```powershell
$env:PATH = 'D:\smart-grids-tools\llvm-mingw-20260922-ucrt-x86_64\bin;' + $env:PATH
$env:TEMP = 'D:\smart-grids-tools\temp'
$env:TMP = $env:TEMP
mingw32-make CC=clang BUILD_DIR=build/issue10 test-simulacao
mingw32-make CC=clang BUILD_DIR=build/issue10-regressao test
mingw32-make CC=clang BUILD_DIR=build/issue10-sanitize sanitize
```

O teste controlado obteve 1 componente antes, 2 depois da falha `1 -- 2`, 4
vértices fora da componente da subestação `0` e novamente 1 componente após a
restauração, tanto na lista quanto na matriz. A suíte normal registrou 54
verificações sem falhas; a instrumentada, 153 verificações sem falhas e 19 pontos
de alocação exercitados. O alvo geral confirmou as suítes anteriores de matriz,
lista, BFS, DFS e componentes conexos. O log local está em
`build/issue10-testes.log` e é ignorado pelo Git.

O comando `dot` não estava disponível; a saída DOT textual foi validada pela
suíte. O teste real não foi executado porque continuam ausentes o carregador e os
arquivos de dataset da issue #4. Consulte [simulacao-falhas.md](simulacao-falhas.md)
para contrato, Graphviz, erros e resultado esperado/obtido.

## Componentes conexos — execução da issue #9 (01/10/2026)

O [guia de componentes](componentes-conexos.md) registra contratos, explicação,
critérios e resultados. O Makefile compila e executa dez suítes: lista, matriz,
BFS, DFS e componentes, cada qual normal e com alocação instrumentada.
O checkout recebido tinha os testes da BFS, mas eles não integravam mais os alvos
gerais; foram reintegrados para validar a dependência adaptada nesta issue.
`test-componentes` seleciona as duas novas suítes; `test-bfs`, as duas da BFS;
`test-falhas`, as cinco com falhas de alocação. `sanitize` executa todas as dez.

Preparação e comandos efetivamente executados em PowerShell:

```powershell
Set-Location 'C:\Users\lipec\Desktop\smart_grids\Projeto_Smart_Grids'
$env:PATH = 'D:\smart-grids-tools\llvm-mingw-20260922-ucrt-x86_64\bin;' + $env:PATH
$env:TEMP = 'D:\smart-grids-tools\temp'
$env:TMP = $env:TEMP
mingw32-make CC=clang BUILD_DIR=build/componentes all
mingw32-make CC=clang BUILD_DIR=build/componentes test-componentes
mingw32-make CC=clang BUILD_DIR=build/componentes test
$env:ASAN_OPTIONS = 'halt_on_error=1'
$env:UBSAN_OPTIONS = 'halt_on_error=1'
mingw32-make CC=clang BUILD_DIR=build/componentes sanitize
```

Todos retornaram **0**. Compilador Clang 23.1.2 (LLVM-MinGW), GNU Make 4.4.1,
C11, `-Wall -Wextra -Wpedantic`; nenhuma advertência de compilação.
Componentes: **35.295 verificações / zero falhas**; componentes com falhas de
alocação: **37.794 verificações / zero falhas**. Regressões de lista, matriz,
BFS e DFS passaram. ASan/UBSan passaram sem diagnósticos; a instrumentação de
componentes e BFS terminou com zero blocos vivos. Não foi usado LeakSanitizer.

Linha de compilação do teste normal (os espaços entre argumentos são equivalentes):

```powershell
clang -Iinclude -std=c11 -Wall -Wextra -Wpedantic -O2 src/componentes_conexos.c src/bfs.c src/grafo_lista.c src/matriz_adjacencia.c tests/test_componentes_conexos.c -o build/componentes/teste_componentes_conexos.exe
```

Após compilar, o binário específico também pode ser executado diretamente:

```powershell
.\build\componentes\teste_componentes_conexos.exe
```

Logs locais, ignorados pelo Git: `build/componentes-compilacao.log`,
`build/componentes-especificos.log`, `build/componentes-testes.log` e
`build/componentes-sanitizadores.log`. Antes das alterações, as seis suítes que
integravam `test` também passaram; registro em `build/componentes-base.log`.
O diretório próprio preserva builds anteriores. Em Linux/macOS, os alvos
equivalentes são `make all`, `make test-componentes`, `make test` e
`make CC=clang sanitize`; não se alega execução nesses sistemas nesta entrega.

**Dataset real: não executado.** Faltam arquivos, recorte e carregador. Os testes
pequenos e os de 1.000 vértices são sintéticos. Subestação: informação indisponível
nos grafos atuais. Os registros das entregas anteriores são preservados abaixo.

## DFS — execução da issue #8 (01/10/2026)

O contrato e as evidências estão em [dfs.md](dfs.md). Foram acrescentados
`include/dfs.h`, `src/dfs.c`, `tests/test_dfs.c` e esse guia. As representações
publicadas são usadas sem alterações. O Makefile agora compila e executa seis
suites: matriz, alocação da matriz, lista, alocação da lista, DFS e alocação da DFS.
Os testes preexistentes da lista voltaram a integrar o alvo geral após a integração
das duas representações. `test-dfs` executa só as duas suites da nova travessia;
`test-falhas` reúne as três suites com alocação instrumentada.

Na sessão PowerShell, usando o LLVM-MinGW portátil já disponível:

```powershell
Set-Location 'C:\Users\lipec\Desktop\smart_grids\Projeto_Smart_Grids'
$env:PATH = 'D:\smart-grids-tools\llvm-mingw-20260922-ucrt-x86_64\bin;' + $env:PATH
$env:TEMP = 'D:\smart-grids-tools\temp'
$env:TMP = $env:TEMP
mingw32-make CC=clang BUILD_DIR=build/dfs all
mingw32-make CC=clang BUILD_DIR=build/dfs test
$env:ASAN_OPTIONS = 'halt_on_error=1'
$env:UBSAN_OPTIONS = 'halt_on_error=1'
mingw32-make CC=clang BUILD_DIR=build/dfs sanitize
```

Os três alvos retornaram **0**. Clang 23.1.2, GNU Make 4.4.1, C11,
`-Wall -Wextra -Wpedantic`; compilação sem avisos e ASan/UBSan sem diagnósticos.
DFS normal: **4.755 verificações / zero falhas**; DFS instrumentada:
**5.743 verificações / zero falhas**, incluindo quatro pontos de falha por
representação e zero blocos vivos ao final. As quatro suites anteriores passaram.
Logs locais, ignorados pelo Git: `build/dfs-compilacao.log`,
`build/dfs-testes.log` e `build/dfs-sanitizadores.log`.

O uso de `build/dfs` preservou os artefatos anteriores da matriz. Dados reais
continuam **não executados**, pois faltam dataset e carregador. As seções abaixo
preservam o registro da entrega anterior da matriz, com seus comandos e ambiente.

## Arquivos e dependências

- `include/matriz_adjacencia.h`: contrato público, retornos e propriedade.
- `src/matriz_adjacencia.c`: matriz binária não direcionada.
- `tests/teste_matriz_adjacencia.c`: exemplo obrigatório, escala e entradas inválidas.
- `tests/teste_alocacao.c`: falhas controladas, contabilidade e liberação.
- `Makefile`: compilação, testes e sanitizadores.

Requer compilador C11 e GNU Make. O módulo usa somente a biblioteca padrão C.
Sanitizadores requerem suporte do compilador/runtime e são ferramentas de teste.
Não há parser ou dataset disponível; `make test` valida apenas entradas sintéticas.
O [guia da matriz](matriz-adjacencia.md) registra contrato, limitações, resultados
e critérios de aceitação, incluindo a integração real ainda pendente.

## Compilação e teste em Linux/macOS

Na raiz do repositório, com `cc` e `make` no PATH:

```sh
make all
make test
make CC=clang sanitize
```

Pode-se selecionar outro compilador compatível, por exemplo `make CC=gcc test`.
Esses comandos POSIX são instruções de reprodução; a execução registrada nesta
entrega foi no Windows abaixo, sem afirmar validação em outro sistema operacional.
Se trocar compilador/flags, use outro diretório de build para evitar objetos antigos:
`make CC=gcc BUILD_DIR=build/gcc test`.

## Ambiente Windows efetivamente utilizado

Não havia compilador C nem Make no PATH, nem distribuição WSL instalada.
Foi baixado o pacote portátil oficial
[LLVM-MinGW 20260922 UCRT x86_64](https://github.com/mstorsjo/llvm-mingw/releases/tag/20260922),
que forneceu Clang 23.1.2 e GNU Make 4.4.1. O SHA-256 do ZIP foi conferido contra
o digest publicado na release:

```text
e3ad77d117a4bea19a7a3b333341824d79a5a371004a10e25b8504e7b3047666
```

O compilador foi extraído em `D:\smart-grids-tools\llvm-mingw-20260922-ucrt-x86_64`.
O download inicial em C: foi interrompido por falta de espaço; somente o ZIP parcial
criado nessa tentativa foi removido. Como C: voltou a registrar zero bytes livres,
o trabalho foi feito em um novo checkout em `D:\smart-grids-issue-6`, na branch
`feat/issue-6-matriz-adjacencia`, partindo do mesmo commit `2e0eba7`.
Quando o espaço de C: voltou a estar disponível, os nove arquivos da alteração
foram copiados para o checkout inicial em
`C:\Users\lipec\Desktop\smart_grids\Projeto_Smart_Grids`, após confirmar que
continuava limpo. Os hashes foram comparados e as suites normal e instrumentada
foram executadas também nesse checkout, que contém a entrega final.
A restrição foi de disco, não de RAM para o teste de 1.000 vértices.

Preparação de uma sessão PowerShell (ajuste caminhos se instalar em outro local;
essas variáveis não modificam permanentemente o ambiente do Windows):

```powershell
Set-Location 'C:\Users\lipec\Desktop\smart_grids\Projeto_Smart_Grids'
$env:PATH = 'D:\smart-grids-tools\llvm-mingw-20260922-ucrt-x86_64\bin;' + $env:PATH
New-Item -ItemType Directory -Force -Path 'D:\smart-grids-tools\temp' | Out-Null
$env:TEMP = 'D:\smart-grids-tools\temp'
$env:TMP = $env:TEMP
```

Comando de compilação executado:

```powershell
mingw32-make CC=clang all
```

Ele executou as três linhas abaixo (saídas em `build/`):

```text
clang -Iinclude -std=c11 -Wall -Wextra -Wpedantic -O2 src/matriz_adjacencia.c tests/teste_matriz_adjacencia.c -o build/teste_matriz_adjacencia.exe
clang -Iinclude -std=c11 -Wall -Wextra -Wpedantic -O2 -Dmalloc=teste_alocar -Dcalloc=teste_alocar_zerado -Dfree=teste_liberar -c src/matriz_adjacencia.c -o build/matriz_alocacao.o
clang -Iinclude -std=c11 -Wall -Wextra -Wpedantic -O2 tests/teste_alocacao.c build/matriz_alocacao.o -o build/teste_alocacao.exe
```

Teste e registro de saída executados:

```powershell
mingw32-make CC=clang test 2>&1 | Tee-Object -FilePath build/testes.txt
$LASTEXITCODE
```

Para executar só o teste obrigatório (que inclui também escala e limites):

```powershell
.\build\teste_matriz_adjacencia.exe
```

Sanitizadores e registro de saída executados:

```powershell
mingw32-make CC=clang sanitize 2>&1 | Tee-Object -FilePath build/sanitizadores.txt
$LASTEXITCODE
```

O alvo usa outro diretório (`build/sanitizers`) e as flags:

```text
-std=c11 -Wall -Wextra -Wpedantic -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
```

O diretório `bin` do LLVM-MinGW precisa estar no PATH também durante a execução,
pois contém a DLL do AddressSanitizer. As duas suites são executadas tanto no alvo
`test` quanto no `sanitize`. Cada comparação imprime esperado, obtido e
`PASSOU`/`FALHOU`; qualquer falha faz o executável retornar `EXIT_FAILURE` e o Make
falhar. Retorno zero significa aprovação das suites sintéticas, **não** aprovação
da integração com dados reais.

Os logs locais `build/testes.txt` e `build/sanitizadores.txt`, objetos e executáveis
são ignorados pelo Git. O relato dos resultados está versionável em
`docs/matriz-adjacencia.md`. Nenhum teste preexistente havia no commit inicial.


## Integração da BFS e suíte conjunta — F1-06

Os registros anteriores descrevem a entrega da matriz. Na integração da BFS,
a lista e a matriz já estavam implementadas; o Makefile passou a compilar e
executar **seis suítes**: matriz normal/alocação, lista normal/alocação e BFS
normal/alocação. Os testes preexistentes foram preservados.

```sh
make CC=gcc all
make CC=gcc test
make CC=gcc test-bfs
```

O último comando seleciona apenas as duas suítes BFS. `test-lista` seleciona as
duas da lista; `test-falhas` seleciona as três suítes de alocação. `sanitize`
instrumenta e executa a suíte conjunta em um diretório separado.

Nesta entrega, os testes foram efetivamente executados com GCC 16.2.0 no Windows
(MSYS2) e GCC 13.3.0 no Ubuntu/WSL, com ASan/UBSan e detecção de vazamentos no WSL.
Para o Make do MSYS2 neste ambiente, a invocação PowerShell é:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
make OS= EXT=.exe CC=gcc BUILD_DIR=build/bfs-windows all
make OS= EXT=.exe CC=gcc BUILD_DIR=build/bfs-windows test
```

Esses parâmetros selecionam receitas POSIX no Make do MSYS2. O Make nativo
`mingw32-make` continua usando as receitas Windows existentes.
Veja [bfs.md](bfs.md) para os comandos completos de sanitizadores, contrato da
API, FIFO, complexidades, resultados esperado/obtido e critérios de aceitação.
O teste do dataset real permanece pendente por ausência do carregador e arquivos.

## Suíte conjunta após a F1-15

O Makefile agora inclui explicitamente matriz, lista, BFS, DFS e algoritmos
complementares. A BFS já existia no repositório, mas não estava nos alvos após a
integração posterior da DFS; seus alvos foram recolocados sem alterar o módulo.

```sh
make CC=gcc all
make CC=gcc test
make CC=gcc test-complementares
```

Os comandos efetivamente usados no Windows/MSYS2 e no Ubuntu/WSL, os resultados
esperado/obtido e as limitações estão em
[algoritmos-complementares.md](algoritmos-complementares.md).
