# Componentes conexos — F1-08 / issue #9

## Estado inspecionado e escopo

Em 01/10/2026 (America/Sao_Paulo), foram lidas integralmente a
[#9](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/9) e sua lista
de comentários (vazia), as dependências
[#7 — BFS](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/7) e
[#8 — DFS](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/8), as estruturas
[#5 — lista](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/5) e
[#6 — matriz](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/6), a
[#3 — modelagem](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/3), a
[#4 — carregador](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/4) e a
[#10 — falhas](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/10).
#5–#8 estavam fechadas; #3/#4/#9/#10, abertas.

O checkout correto era `Projeto_Smart_Grids`, na branch
`feat/issue-9-componentes-conexos`, limpo em `de3a99f`, mesmo commit da `main`
remota consultada. Já existiam lista, matriz, BFS, DFS, Makefile e testes C11.
Não foram encontrados `AGENTS.md` aplicáveis. README, documentos, headers, fontes,
testes e configuração de build foram inspecionados. Os documentos de arquitetura,
planejamento, filosofia e orquestração estavam vazios e foram preservados.

**Implementação e testes sintéticos concluídos; validação real pendente.**
Não havia dataset nem carregador, e as representações não tinham atributos de
subestação. Não foi implementado outro parser ou modelo de domínio para suprir
essas dependências. Nenhuma estrutura de grafo paralela foi criada.

As APIs existentes são exclusivamente **não direcionadas**: os tipos são opacos,
e inserir/remover uma conexão altera ambos os sentidos. Não existe parâmetro para
orientação. Este módulo não aceita outro tipo de grafo nem oferece um algoritmo
de componentes fortemente conexas de dígrafos.

## Arquivos e funções

| Arquivo | Responsabilidade |
| --- | --- |
| `include/componentes_conexos.h` | Resultado, códigos de retorno e contrato público |
| `src/componentes_conexos.c` | Seleção de origens, rótulos, tamanhos, relatório e liberação |
| `src/bfs_interno.h` | Contrato interno para compartilhar a BFS existente |
| `src/bfs.c` | Mesmos laços de BFS, separados da inicialização por chamada |
| `tests/test_componentes_conexos.c` | Partições conhecidas, escala, contratos, relatório e alocação |
| `Makefile` | Integração das suítes e sanitizadores, incluindo regressão da BFS |

| Função pública | Objetivo |
| --- | --- |
| `identificar_componentes_conexos_lista(grafo, &resultado)` | Identificar toda a partição pela lista existente |
| `identificar_componentes_conexos_matriz(matriz, &resultado)` | Identificar toda a partição diretamente na matriz |
| `imprimir_componentes_conexos(resultado, arquivo)` | Escrever total, identificadores, tamanhos, membros e indisponibilidade de subestação |
| `liberar_componentes_conexos(&resultado)` | Liberar vetores/cabeçalho e zerar o dono |

### Resultado e propriedade

`ComponentesConexos` é independente da vida útil do grafo:

| Campo | Conteúdo válido |
| --- | --- |
| `quantidade_vertices` | V no momento do cálculo |
| `quantidade_componentes` | C, entre 0 e V |
| `componente_por_vertice[v]` | Rótulo em `[0,C)` para cada índice interno v |
| `tamanhos_componentes[c]` | Quantidade de vértices da componente c; somente `[0,C)` é inicializado |
| `membros` | V índices, cada vértice exatamente uma vez, agrupados por rótulo |

Os vetores têm capacidade V. Os membros da componente c começam na soma dos
tamanhos das componentes anteriores. Para enumerar tudo, mantenha um índice
acumulado; assim a leitura custa O(V+C). O vetor `componente_por_vertice` permite
consultar a componente de um vértice em O(1).

Os índices são os das representações existentes, não IDs externos da BDGD.
Rótulos não são identidades permanentes: podem variar com a ordem dos índices,
da travessia ou entre topologias. Compare a partição, isto é, se cada par de
vértices permanece ou não junto. A ordem dos membros também pode variar.

O chamador começa com `ComponentesConexos *resultado = NULL`. Em sucesso, passa
a possuir resultado e vetores; deve liberá-los somente pela função pública.
Não deve alterar campos, liberar vetores separadamente nem reutilizar cópias
de ponteiros após a liberação. O grafo continua sob propriedade do chamador.

| Retorno | Condição |
| --- | --- |
| `COMPONENTES_SUCESSO` | Resultado completo, inclusive para grafo vazio |
| `COMPONENTES_ARGUMENTO_INVALIDO` | Grafo/saída NULL ou saída já ocupada |
| `COMPONENTES_SEM_MEMORIA` | Falha de alocação do módulo ou da BFS |
| `COMPONENTES_LIMITE_EXCEDIDO` | V não cabe em um vetor de `size_t` sem overflow |

Todo erro preserva a saída e libera construções parciais. NULL não é um grafo
vazio. Um **objeto válido com zero vértices** produz sucesso, V=C=0 e três
ponteiros NULL. Não chama BFS, pois não há origem válida; somente o cabeçalho
do resultado é alocado. A liberação aceita NULL e repetição sobre o mesmo dono.

O relatório recebe um resultado válido do módulo e um `FILE` aberto para escrita.
Retorna 1 em sucesso e 0 para NULL ou falha de escrita; não fecha o arquivo.
O chamador verifica também erros tardios em `fflush`/`fclose`.

## Reutilização da BFS e explicação do algoritmo

Foi usada a BFS da #7, já implementada para ambas as estruturas. A adaptação
interna foi explicada antes da alteração: separar sua reserva/inicialização
dos laços de travessia permite preservar visitados entre origens. O contrato
público de `include/bfs.h` e os chamadores existentes permanecem iguais.

`criar_busca_bfs` reserva uma busca e inicializa `distancias[v] = SIZE_MAX` uma
vez. Esse vetor já representa visitados na BFS; não há outro vetor redundante.
`continuar_bfs_lista` e `continuar_bfs_matriz` contêm os laços originais de busca.
Cada uma usa `inicio_fila = quantidade_alcancados` antes de acrescentar a nova
origem. Assim não volta a processar as componentes anteriores. Essas funções
internas exigem grafo válido e inalterado, mesma quantidade de vértices e origem
ainda não visitada, condições garantidas pelos dois módulos chamadores.

O cálculo completo segue estas etapas:

1. Reservar o resultado e uma única busca BFS para todo o grafo.
2. Examinar as origens de 0 a V−1. Se a origem já foi visitada, seguir adiante.
3. Para uma origem não visitada, guardar o tamanho atual da ordem de visita.
4. Continuar a BFS a partir dela, marcando cada descoberta antes de enfileirar.
5. Atribuir o rótulo atual somente às posições acrescentadas à ordem e registrar
   seu número como tamanho da componente. Incrementar C.
6. Ao terminar, transferir o vetor de ordem da BFS para `membros`, sem copiá-lo,
   e liberar o cabeçalho e as distâncias temporárias da BFS.

A ordem/fila tem capacidade V. Uma descoberta recebe marcação antes de entrar
na fila, portanto ciclos e laços não causam entradas repetidas. Uma origem
isolada ainda entra na fila uma vez e produz uma componente de tamanho 1.
As distâncias internas de cada nova componente começam em zero na sua origem;
elas não são expostas pelo resultado de componentes.

### Por que a partição está correta

Uma BFS alcança exatamente os vértices ligados por caminhos à sua origem.
Como o grafo é não direcionado, essa região é uma componente inteira. Se
existisse aresta para uma região ainda não visitada, a própria BFS a alcançaria;
logo componentes concluídas não precisam ser revisitadas. O laço externo examina
todos os vértices e inicia uma busca para cada região ainda ausente.

A marcação global impede redescobertas e cada novo trecho da fila recebe um
único rótulo. Ao final, todos os V vértices foram incluídos exatamente uma vez,
nenhuma componente é vazia e a soma dos tamanhos é V. Essa argumentação depende
da simetria das conexões; não deve ser aplicada a componentes fortes de dígrafos.

### Complexidade e memória

| Representação | Cálculo completo | Memória adicional |
| --- | --- | --- |
| Lista | O(V+E) | O(V) |
| Matriz, V > 0 | O(V²) | O(V) |
| Grafo vazio | O(1) | O(1) |

Na lista, cada vértice é processado uma vez e cada conexão aparece nos dois
sentidos. Na matriz, cada linha é consultada uma vez, em V colunas. A seleção
de origens, marcação, atribuição de rótulos e enumeração de membros custam O(V).
Não há custo O(C·V) de reinicialização, nem reserva de vetores por componente.

Para V > 0, há seis alocações: cabeçalho de componentes, dois vetores do resultado,
cabeçalho BFS, ordem/fila e distâncias. Ao retornar, restam quatro blocos:
cabeçalho e três vetores do resultado. Bytes solicitados no pico:
`sizeof(ComponentesConexos) + sizeof(ResultadoBfs) + 4*V*sizeof(size_t)`;
depois: `sizeof(ComponentesConexos) + 3*V*sizeof(size_t)`. Essas expressões
descrevem a memória do módulo, sem grafo, metadados do alocador ou RSS. Os produtos
são validados antes de alocar. O teste não tenta construir um grafo perto de
`SIZE_MAX`; falhas de memória são exercitadas por injeção controlada.

## Conexões ativas, subestação e integração com a #10

As representações atuais não guardam um campo `status`: conexão ativa significa
**conexão presente**. A remoção bilateral existente desativa o par; reinserir o
mesmo par restaura a conexão. O módulo consulta apenas as APIs dessas estruturas.
Não interpreta CSV, não presume valores para `status` e não mantém histórico.

Depois de qualquer alteração, faça um novo cálculo. O resultado anterior é um
retrato da topologia anterior, útil para comparação, e continua válido até ser
liberado. Um dono ocupado é rejeitado: use outro ponteiro NULL para guardar o
depois, ou libere o resultado antigo se não precisar compará-lo. Não reutilize
marcações BFS de uma topologia anterior.

No exemplo obrigatório, remover `(1,2)` transforma `{0,1,2}` em `{0,1}` e `{2}`.
Restaurar a conexão retorna à partição original. Isso comprova a interface
necessária ao recálculo da #10, sem implementar sua seleção de falhas, histórico
ou classificação elétrica.

**Subestação indisponível:** as APIs não armazenam tipo, subestação, geração ou
demanda. O relatório diz isso explicitamente. Não se assume que o vértice 0
representa uma subestação e não se adicionou um atributo provisório de domínio.
Quando #3/#4 fornecerem o índice real de uma subestação, sua componente poderá
ser consultada em `componente_por_vertice[indice_subestacao]`. A identificação
de ilhas autossuficientes exige informações elétricas adicionais, conforme #10.

## Exemplo de uso

```c
#include "componentes_conexos.h"

/* Recebe um grafo ja construido; o chamador continua sendo seu dono.
 * Retorna 0 em sucesso e 1 se o calculo ou a escrita falhar. */
int mostrar_componentes(const GrafoLista *grafo)
{
    ComponentesConexos *resultado = NULL;
    StatusComponentes status = identificar_componentes_conexos_lista(grafo, &resultado);
    if (status != COMPONENTES_SUCESSO) {
        fprintf(stderr, "Falha no calculo: %d\n", (int)status);
        return 1;
    }
    int escrito = imprimir_componentes_conexos(resultado, stdout);
    liberar_componentes_conexos(&resultado);
    return escrito && fflush(stdout) == 0 ? 0 : 1;
}
```

Para matriz, use `identificar_componentes_conexos_matriz` com o mesmo padrão de
tratamento de retorno, relatório e liberação.

## Execução e evidências

Executado no Windows x64, Clang 23.1.2 (LLVM-MinGW), GNU Make 4.4.1, C11 com
`-Wall -Wextra -Wpedantic`. Preparação do ambiente e linha completa de compilação
em [development.md](development.md). Comandos na raiz do checkout:

```powershell
mingw32-make CC=clang BUILD_DIR=build/componentes all
mingw32-make CC=clang BUILD_DIR=build/componentes test-componentes
mingw32-make CC=clang BUILD_DIR=build/componentes test
mingw32-make CC=clang BUILD_DIR=build/componentes sanitize
```

Todos retornaram **0**. `test` executa dez suítes; nenhuma advertência de compilação.
`sanitize` recompila todas em `build/componentes/sanitizers`, com AddressSanitizer
e UndefinedBehaviorSanitizer, sem diagnósticos. Não se alega LeakSanitizer ou
Valgrind. A instrumentação própria cobre todas as alocações de componentes e BFS,
inclusive a transferência de propriedade do vetor de membros: zero blocos vivos
após sucesso, erros e recuperação.

### Pequenos controlados — esperado e obtido

As arestas iniciais são `(0,1), (1,2), (3,4), (4,5), (5,6)`, em 8 vértices.
Cada linha abaixo foi calculada nas duas representações e comparada por programa:

| Cenário | V / E | Partição esperada | Obtido | Resultado |
| --- | --- | --- | --- | --- |
| Original | 8 / 5 | `{0,1,2}`, `{3,4,5,6}`, `{7}`; tamanhos 3,4,1; C=3 | Igual | PASSOU |
| Adicionar `(2,3)` e `(6,7)` | 8 / 7 | `{0,1,2,3,4,5,6,7}`; tamanho 8; C=1 | Igual | PASSOU |
| Reconstruir original e remover `(1,2)` | 8 / 4 | `{0,1}`, `{2}`, `{3,4,5,6}`, `{7}`; tamanhos 2,1,4,1; C=4 | Igual | PASSOU |
| Restaurar `(1,2)` | 8 / 5 | Partição original; tamanhos 3,4,1; C=3 | Igual | PASSOU |
| Acrescentar ciclos `(0,2)` e `(3,6)` | 8 / 7 | Partição original, sem duplicar membros | Igual | PASSOU |
| Laço `(7,7)` somente na matriz, inclusive inserção repetida | 8 / 7 na lista; 8 / 8 na matriz | Mesma partição; laço não une regiões | Igual | PASSOU |
| Vazio | 0 / 0 | C=0, três vetores NULL | Igual | PASSOU |
| Unitário | 1 / 0 | `{0}`; tamanho 1; C=1 | Igual | PASSOU |

O teste usa rótulos esperados como 42, 9 e 71, diferentes dos retornados, e compara
a relação de pertencimento para todos os pares de vértices. Confere também
rótulos válidos, tamanhos contados a partir dos rótulos, membros em seu grupo,
cobertura, unicidade e equivalência lista/matriz. No original:

```text
Componentes: esperado=3 obtido=3 PASSOU
Soma dos tamanhos: esperado=8 obtido=8 PASSOU
Vertices omitidos: esperado=0 obtido=0 PASSOU
Vertices repetidos: esperado=0 obtido=0 PASSOU
Pares divergentes da particao esperada: esperado=0 obtido=0 PASSOU
Total de componentes: 3; vertices: 8
Subestacao: indisponivel (representacoes sem atributos).
Componente 0: tamanho=3; membros={0,1,2}
Componente 1: tamanho=4; membros={3,4,5,6}
Componente 2: tamanho=1; membros={7}
```

As 64 adjacências de cada representação são comparadas com um oráculo das
arestas conhecidas no original, após falha e após restauração. Os cálculos não
alteram a topologia. Resultados anteriores à falha são relidos e validados
após alterar e liberar os grafos. O relatório é gravado em arquivo temporário,
lido e comparado com total, tamanhos, membros e a informação de subestação.

### Escala — exclusivamente sintética

| Cenário | V | E | Esperado | Obtido | Resultado |
| --- | ---: | ---: | --- | --- | --- |
| Todos isolados | 1.000 | 0 | 1.000 componentes, tamanho 1 cada | Igual | PASSOU |
| Cadeia `(0,1), …, (998,999)` | 1.000 | 999 | Uma componente de tamanho 1.000 | Igual | PASSOU |
| Remover `(499,500)` | 1.000 | 998 | Duas componentes de tamanho 500 | Igual | PASSOU |
| Restaurar conexão | 1.000 | 999 | Uma componente de tamanho 1.000 | Igual | PASSOU |

As duas representações recebem os mesmos pares pelas APIs existentes. O oráculo
vem de fórmulas conhecidas, não da BFS. A instrumentação verifica exatamente seis
alocações por cálculo não vazio, tanto na cadeia quanto nos 1.000 isolados.
Isso confirma ausência de alocações por origem; a ausência de reinicialização
por componente também é explícita no código. As comparações de todos os pares
no teste custam O(V²), independentemente da complexidade linear do módulo na lista.

### Contratos, alocação e regressões

Testados argumentos NULL, saída ocupada/preservada, grafo vazio, liberação
repetida e após liberar o grafo. Entradas inválidas não alocam. No executável
instrumentado, cada um dos seis pontos de alocação falha individualmente, nas
duas representações. Testa-se também o único ponto do grafo vazio: 14 cenários
de falha no total, cada um seguido de recuperação bem-sucedida. Em erro:
`COMPONENTES_SEM_MEMORIA`, saída NULL, topologia preservada e zero blocos vivos.

| Suíte | Normal | Alocação instrumentada | Resultado |
| --- | ---: | ---: | --- |
| Componentes | 35.295 verificações | 37.794 verificações | Zero falhas em ambas |
| BFS preexistente | 8.331 verificações | 8.471 verificações | Zero falhas em ambas |
| DFS preexistente | 4.755 verificações | 5.743 verificações | Zero falhas em ambas |
| Lista preexistente | 4.130 verificações | 5.587 verificações | Zero falhas em ambas |
| Matriz preexistente | `TOTAL SINTETICO: PASSOU` | `TOTAL ALOCACAO: PASSOU` | Zero falhas em ambas |

Os mesmos resultados foram obtidos com ASan/UBSan. O teste usa comparações
explícitas, imprime esperado/obtido e `PASSOU`/`FALHOU`, e retorna `EXIT_FAILURE`
se qualquer comparação falhar, inclusive quando compilado com `NDEBUG`.

### Dataset real — NÃO EXECUTADO / PENDENTE

Fonte prevista: **BDGD/ANEEL**, conforme README. Distribuidora, versão, arquivos
e recorte não foram entregues; V e E reais são **desconhecidos**. Nenhuma
verificação de dados reais foi executada. Os casos acima não são recortes BDGD.

Para cumprir esse critério, faltam a modelagem da #3, o dataset/recorte das
dependências e o carregador da #4. Quando estiverem disponíveis, a integração deve:

1. Registrar fonte, distribuidora, versão/data e recorte reproduzível.
2. Usar o carregador existente para mapear IDs externos para os mesmos índices
   nas duas representações, incluindo vértices isolados.
3. Respeitar as decisões de laços, duplicatas/segmentos paralelos e conexões ativas,
   registrando V, segmentos e pares únicos efetivamente presentes.
4. Estimar a memória da matriz e registrar qualquer limite/recorte necessário,
   usando a mesma topologia na lista e na matriz, sem truncamento silencioso.
5. Calcular todas as componentes sem presumir rede conectada. Conferir cobertura
   exata, soma dos tamanhos, membros, equivalência das partições, subestações
   quando mapeadas e recálculo após falha/restauração.
6. Registrar comandos, contagens e resultados reais neste guia.

## Critérios de aceitação da #9

| Critério | Estado | Evidência |
| --- | --- | --- |
| Todos os componentes identificados | Atendido nos cenários testados | Partições completas conhecidas, ciclos e 1.000 isolados |
| Cada vértice em exatamente um componente | Atendido | Rótulos válidos; zero omissões/repetições; membros e tamanhos consistentes |
| Quantidade total informada | Atendido | C retornado e impresso; 3 → 1 e 3 → 4 → 3 no exemplo |
| Quantidade de vértices por componente informada | Atendido | Vetor de tamanhos e relatório; 3,4,1 / 8 / 2,1,4,1 |
| Funciona em rede totalmente conectada | Atendido | União dos oito vértices e cadeia de 1.000 |
| Funciona em rede desconectada | Atendido | Original, falha, ciclos separados e 1.000 isolados |
| Resultado validado em grafo pequeno controlado | Atendido | Exemplo obrigatório executado nas duas APIs, com comparações programáticas |
| Resultado validado com dataset real | **Pendente** | Não há arquivos/recorte nem carregador para executar |

Portanto, **7 de 8 critérios atendidos**. A issue não está integralmente concluída
enquanto a validação com o dataset real permanecer pendente.
