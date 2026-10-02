# Desenvolvimento — módulo de Matriz de Adjacência

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
