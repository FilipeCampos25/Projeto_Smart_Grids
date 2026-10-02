# Smart Grid Graph Analysis

Projeto integrador desenvolvido para a disciplina de Teoria dos Grafos.

## Tema

Smart Grids / Redes de Distribuição de Energia Elétrica.

## Objetivo

Modelar uma rede de distribuição elétrica real utilizando grafos e aplicar
algoritmos de conectividade e otimização para analisar contingências,
reconfiguração da rede e rotas de menor custo.

## Tecnologias

- Linguagem C
- Git/GitHub
- Dataset real de rede elétrica
- Implementações autorais dos algoritmos de grafos

## Fase I — Topologia e Conectividade

A primeira fase contempla:

- carregamento do dataset;
- lista de adjacência;
- matriz de adjacência;
- BFS;
- DFS;
- componentes conexos;
- simulação de falhas;
- comparação de tempo e memória.

## Fase II — Otimização e Complexidade

Planejado:

- Prim;
- Dijkstra;
- problema NP-Difícil a definir/validar;
- comparação entre soluções exatas e heurísticas.

## Dataset

A fonte prioritária em avaliação é a Base de Dados Geográfica da
Distribuidora (BDGD), disponibilizada pela ANEEL.

## Organização

As tarefas do projeto são gerenciadas utilizando GitHub Issues e GitHub Projects.

## Lista de adjacência — F1-04

O núcleo em C11 está em `src/grafo_lista.c`, com API documentada em
`include/grafo_lista.h`. Suporta vértices isolados, inserção e remoção de conexões
não direcionadas, consulta de vizinhos e liberação da memória.

Com GCC e GNU Make instalados, em Linux ou no terminal MSYS2 UCRT64:

```sh
make all
make test test-falhas
```

Os testes comparam o exemplo de seis vértices, uma cadeia sintética de 1.000
vértices e cenários de entradas inválidas e falhas de alocação. Para sanitizadores
em um ambiente com suporte (Linux/WSL), execute `make sanitize`.

Consulte [desenvolvimento e resultados](docs/development.md) para comandos
PowerShell, funcionamento da estrutura, evidências e critérios da issue.
As [hipóteses locais de modelagem](docs/modelagem-grafo.md) são provisórias.
**A integração com o dataset real permanece pendente**, pois o repositório ainda
não contém o recorte nem o carregador das issues anteriores.
