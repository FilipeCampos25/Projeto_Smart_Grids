# Artigo científico — issue #37

Estado: **versão preliminar; não apta a encerrar a issue #37**.

O texto usa o estilo de conferências da SBC e limita suas conclusões aos
artefatos auditados. Autores, afiliação e e-mail permanecem como marcadores
porque esses dados não estão confirmados no repositório.

## Compilação esperada

O diretório contém `sbc-template.sty` e `sbc.bst`. As figuras SVG exigem o
pacote LaTeX `svg` e uma instalação do Inkscape acessível ao compilador.

```sh
pdflatex -shell-escape artigo.tex
bibtex artigo
pdflatex -shell-escape artigo.tex
pdflatex -shell-escape artigo.tex
```

Não havia compilador LaTeX nem Inkscape disponíveis no ambiente auditado em
06/11/2026. Consequentemente, `artigo.pdf` não foi criado e a quantidade de
páginas não pôde ser validada. Não adicionar um PDF sem compilar, conferir as
referências, contar 4–6 páginas e inspecionar visualmente todas as páginas.

## Evidências usadas

- `README.md` e documentação em `docs/`;
- `data/forcel_2025_circuito_1/*.csv`;
- `resultados/fase1_metricas.csv` e `fase1_consolidado.csv`;
- figuras da Fase I geradas pelo script versionado;
- módulos e testes C de BFS, DFS, componentes, Dijkstra, AGM e Cobertura;
- issues #33, #34, #35, #36 e #37 no GitHub.

Consulte `AUDITORIA.md` para os resultados da validação desta revisão.
