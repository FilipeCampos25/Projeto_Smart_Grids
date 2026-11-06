# Auditoria de evidências do artigo

Data: 06/11/2026  
Revisão: `37d417c`  
Branch: `docs/issue-37-artigo-sbc`

## Estado observado

- `make test` com GCC falhou antes de executar a suíte: `src/main.c` chama
  `ler_opcao` e `executar_dijkstra_cli` sem definições visíveis.
- O `Makefile` atual não vincula `src/agm.c` à aplicação e não contém os alvos
  `test-agm`, `test-experimentos-fase2` e `experimentos-fase2` descritos no
  README. Esses alvos existiam no commit da issue #36 e foram perdidos em uma
  integração posterior.
- Dijkstra, compilado e testado isoladamente: 46 verificações, 0 falhas.
- Cobertura de Vértices, compilada e testada isoladamente: 192 verificações,
  0 falhas.
- AGM, compilada e testada isoladamente: 127 verificações, 1 falha no caso
  `Erro preserva saida` após rejeição de peso ausente.
- Gerador sintético de AGM: validação determinística de `N=100` passou.
- Protocolo AGM: 84 linhas executadas (3 tamanhos, 7 repetições e 4
  combinações), todas com `status=ERRO_VALIDACAO`. Os tempos foram descartados
  e não aparecem no artigo.
- O ambiente não dispõe de Python funcional, compilador LaTeX, Inkscape ou
  ferramenta de inspeção/conversão PDF. Ghostscript está presente, mas não
  substitui a compilação LaTeX.

## Critérios da issue #37

- [x] Fonte LaTeX organizada no padrão SBC.
- [ ] Extensão de 4 a 6 páginas confirmada.
- [x] Metodologia e modelagem documentadas.
- [x] Resultados publicados limitados aos CSVs válidos da Fase I.
- [x] Discussão crítica e referências verificáveis.
- [ ] PDF final compilado, inspecionado e versionado.

Conclusão: a issue #37 permanece aberta e o artigo deve ser tratado como
preliminar até a correção e repetição da validação.
