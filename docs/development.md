# Desenvolvimento, compilação e validação

## Requisitos

- Compilador compatível com C11 (`cc`, GCC ou Clang);
- GNU Make (`make` ou `mingw32-make` no Windows);
- Python 3 apenas para reproduzir a normalização e os gráficos.

## Alvos do Makefile

```sh
make                 # aplicação e todos os testes
make app             # somente build/smart_grid
make run             # CLI interativa
make test            # suíte completa, inclusive complementares e dataset real
make test-real       # integração com 1.838 vértices
make test-complementares
make test-simulacao
make experimentos    # 126 linhas de medição em CSV
make sanitize        # ASan/UBSan quando a toolchain oferecer os runtimes
```

No Windows, substitua `make` por `mingw32-make` quando necessário e informe o
compilador, por exemplo `mingw32-make CC=clang test`.

## Testes realizados em 02/10/2026

A consolidação foi compilada com GCC 15.2.0/w64devkit, C11, `-Wall -Wextra
-Wpedantic -O2`, sem warnings. `make test` executou 14 binários e terminou com
zero falhas:

- matriz e falhas de alocação;
- lista e falhas de alocação;
- BFS e falhas de alocação;
- DFS e falhas de alocação;
- componentes e falhas de alocação;
- simulação e falhas de alocação;
- dígrafos, articulações, pontes, Euler e coloração;
- carregador e integração BDGD real.

No teste real, Lista e Matriz tiveram 1.838 vértices e topologia idêntica. BFS e
DFS alcançaram 1.838 vértices em ambas. Componentes retornou uma única partição
em ambas. A falha `72352 -- 12692` produziu duas componentes, deixou 689
vértices fora da região da subestação e voltou a uma componente após restaurar.

`build/smart_grid --demo` também executou a CLI nas duas representações,
registrou métricas, simulou/restaurou a conexão e exportou o DOT. Uma entrada
textual inválida foi testada e o menu continuou normalmente.

O alvo `sanitize` foi tentado, mas a distribuição portátil de GCC disponível
no ambiente não contém `libasan`/`libubsan`; por isso não houve execução de
sanitizadores nesta máquina. Os testes de falha de alocação existentes passaram.
