# Lista de adjacência: hipóteses locais da F1-04

Estas decisões são provisórias e permitem testar o núcleo da issue #5.
**Não constituem a conclusão da modelagem de domínio da issue #3.**

Na inspeção de 01/10/2026, as issues [#1](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/1),
[#2](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/2),
[#3](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/3) e
[#4](https://github.com/FilipeCampos25/Projeto_Smart_Grids/issues/4) estavam abertas,
sem comentários. Não havia política aprovada de duplicatas, laços, IDs ou
atributos, nem modelo implementado, carregador ou dados no checkout.

## Contrato provisório

- O núcleo representa um **grafo simples, não direcionado e não ponderado**.
- Cada inserção de vértice cria um vértice isolado e devolve um índice interno
  consecutivo, começando em zero. Não há remoção de vértices; índices são estáveis.
- Uma conexão é identificada pelo par não ordenado de índices de suas extremidades.
  Existe no máximo uma conexão por par. Não há identificador de segmento físico.
- A duplicata, inclusive com extremos invertidos, retorna `LISTA_ARESTA_DUPLICADA`.
  Um laço retorna `LISTA_LACO_NAO_PERMITIDO`. Nenhum dos dois altera o grafo.
- Para simular uma falha, `remover_aresta_lista` remove fisicamente a conexão
  nos dois sentidos. O total de arestas representa somente conexões presentes.
- A restauração usa `inserir_aresta_lista` com o mesmo par; pode falhar por falta
  de memória. O simulador futuro deve guardar os pares que desejar restaurar.
- Pesos, estado histórico, subestações, transformadores, circuito, comprimento
  e resistência não são armazenados neste núcleo. Sua definição depende da #3.

## IDs externos e integração futura

**ID do dataset não é índice de vetor.** Um futuro carregador deve manter uma
associação entre cada ID externo e o índice devolvido por `inserir_vertice_lista`.
Por exemplo, os IDs textuais `PAC-A17` e `900000` podem corresponder aos índices
internos 0 e 1; não se deve converter diretamente `900000` em posição de vetor.
Essa associação e a validação de IDs externos pertencem à #4 e não foram
implementadas como um segundo parser nesta issue.

O fluxo esperado é: validar os registros, inserir inclusive os vértices isolados,
registrar a associação de IDs, resolver os dois extremos de cada conexão e chamar
`inserir_aresta_lista`. Toda falha deve ser tratada pelo carregador; se ele abandonar
a construção, `liberar_grafo_lista` aceita o grafo parcialmente construído.

As hipóteses de grafo simples precisam ser confrontadas com a BDGD. Se o recorte
contiver segmentos paralelos, rejeitá-los pode perder informação física. A #3 deve
definir se eles serão agregados ou se a estrutura precisará de identidade por
segmento e múltiplas conexões. Não há decisão de agregação silenciosa nesta API.
A política de laços também deverá ser revisada com os dados reais.

O critério **“funciona com o dataset carregado” permanece pendente**. Para validá-lo,
são necessários o recorte e a origem/versão da #1/#2, as decisões da #3 e o
carregador da #4, seguidos de teste que registre vértices, arestas e verificações
sobre os dados reais. Os grafos sintéticos não fornecem essa evidência.
