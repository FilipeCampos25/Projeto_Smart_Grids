#!/usr/bin/env python3
"""Valida, consolida e desenha os resultados medidos da Fase II."""

import argparse
import csv
import math
import statistics
import sys
from collections import defaultdict
from pathlib import Path
from xml.sax.saxutils import escape

CAMPOS_BRUTOS = [
    "cenario", "problema", "algoritmo", "representacao", "N", "arestas",
    "repeticao", "semente", "parametros", "tempo_ms",
    "memoria_representacao_bytes", "memoria_auxiliar_pico_bytes",
    "memoria_total_estimada_bytes", "valor_solucao", "otimo_referencia",
    "razao_aproximacao", "status",
]
CAMPOS_CONSOLIDADOS = [
    "cenario", "problema", "algoritmo", "representacao", "N", "arestas",
    "semente", "parametros", "tempo_mediana_ms", "tempo_media_ms",
    "memoria_representacao_bytes", "memoria_auxiliar_pico_mediana_bytes",
    "memoria_auxiliar_pico_media_bytes", "memoria_total_estimada_mediana_bytes",
    "valor_solucao_mediana", "otimo_referencia", "razao_aproximacao_mediana",
    "repeticoes", "validacao",
]
COMBINACOES = {
    ("AGM", "Prim"), ("AGM", "Kruskal"),
    ("CaminhoMinimo", "Dijkstra"),
    ("CoberturaVertices", "BacktrackingExato"),
    ("CoberturaVertices", "Aproximacao2"),
}


def inteiro(valor, campo, numero_linha):
    try:
        resultado = int(valor)
    except (TypeError, ValueError) as erro:
        raise ValueError(f"Linha {numero_linha}: {campo} nao e inteiro.") from erro
    if resultado < 0:
        raise ValueError(f"Linha {numero_linha}: {campo} negativo.")
    return resultado


def real(valor, campo, numero_linha, permite_menos_um=False):
    try:
        resultado = float(valor)
    except (TypeError, ValueError) as erro:
        raise ValueError(f"Linha {numero_linha}: {campo} invalido.") from erro
    minimo = -1.0 if permite_menos_um else 0.0
    if not math.isfinite(resultado) or resultado < minimo:
        raise ValueError(f"Linha {numero_linha}: {campo} nao finito/fora da faixa.")
    return resultado


def carregar(caminho):
    linhas = []
    with caminho.open(newline="", encoding="utf-8") as arquivo:
        leitor = csv.DictReader(arquivo)
        if leitor.fieldnames != CAMPOS_BRUTOS:
            raise ValueError("Cabecalho CSV bruto invalido ou incompleto.")
        for numero, linha in enumerate(leitor, 2):
            if None in linha or any(valor is None or valor == "" for valor in linha.values()):
                raise ValueError(f"Linha {numero}: registro CSV incompleto.")
            if linha["status"] != "OK":
                raise ValueError(f"Linha {numero}: status experimental {linha['status']!r}.")
            par = (linha["problema"], linha["algoritmo"])
            if par not in COMBINACOES or linha["representacao"] not in {"lista", "matriz"}:
                raise ValueError(f"Linha {numero}: metodo/representacao desconhecido.")
            n = inteiro(linha["N"], "N", numero)
            arestas = inteiro(linha["arestas"], "arestas", numero)
            repeticao = inteiro(linha["repeticao"], "repeticao", numero)
            semente = inteiro(linha["semente"], "semente", numero)
            estrutural = inteiro(linha["memoria_representacao_bytes"], "memoria estrutural", numero)
            auxiliar = inteiro(linha["memoria_auxiliar_pico_bytes"], "memoria auxiliar", numero)
            total = inteiro(linha["memoria_total_estimada_bytes"], "memoria total", numero)
            if n < 1 or repeticao < 1 or semente > 0xFFFFFFFF or arestas != 3 * n - 1:
                raise ValueError(f"Linha {numero}: dimensoes, semente ou E=3N-1 invalidos.")
            if total != estrutural + auxiliar:
                raise ValueError(f"Linha {numero}: memoria total inconsistente.")
            tempo = real(linha["tempo_ms"], "tempo_ms", numero)
            solucao = real(linha["valor_solucao"], "valor_solucao", numero)
            referencia = real(linha["otimo_referencia"], "otimo_referencia", numero, True)
            razao = real(linha["razao_aproximacao"], "razao_aproximacao", numero, True)
            if linha["algoritmo"] == "Aproximacao2" and referencia == -1.0:
                if razao != -1.0:
                    raise ValueError(f"Linha {numero}: razao sem otimo de referencia.")
            elif referencia < 0.0 or razao < 1.0 - 1e-12:
                raise ValueError(f"Linha {numero}: referencia/razao invalida.")
            linhas.append({
                **linha, "N": n, "arestas": arestas, "repeticao": repeticao,
                "semente": semente, "tempo_ms": tempo,
                "memoria_representacao_bytes": estrutural,
                "memoria_auxiliar_pico_bytes": auxiliar,
                "memoria_total_estimada_bytes": total,
                "valor_solucao": solucao, "otimo_referencia": referencia,
                "razao_aproximacao": razao,
            })
    if not linhas:
        raise ValueError("CSV bruto sem medicoes.")
    verificar_comparacoes(linhas)
    return linhas


def proximos(a, b):
    return math.isclose(a, b, rel_tol=1e-12, abs_tol=1e-12)


def verificar_comparacoes(linhas):
    grupos = defaultdict(dict)
    for linha in linhas:
        chave = (linha["cenario"], linha["N"], linha["repeticao"],
                 linha["semente"], linha["parametros"], linha["problema"])
        metodo = (linha["algoritmo"], linha["representacao"])
        if metodo in grupos[chave]:
            raise ValueError(f"Medicao duplicada em {chave}: {metodo}.")
        grupos[chave][metodo] = linha
    for chave, grupo in grupos.items():
        problema = chave[-1]
        if problema == "AGM":
            esperado = {(a, r) for a in ("Prim", "Kruskal") for r in ("lista", "matriz")}
            if set(grupo) != esperado:
                raise ValueError(f"Comparacao AGM incompleta em {chave}.")
            valores = [x["valor_solucao"] for x in grupo.values()]
            if any(not proximos(valores[0], x) for x in valores[1:]):
                raise ValueError(f"Custos de AGM divergentes em {chave}.")
            if any(not proximos(x["valor_solucao"], x["otimo_referencia"]) or
                   not proximos(x["razao_aproximacao"], 1.0)
                   for x in grupo.values()):
                raise ValueError(f"Referencia de AGM inconsistente em {chave}.")
        elif problema == "CaminhoMinimo":
            esperado = {("Dijkstra", r) for r in ("lista", "matriz")}
            if set(grupo) != esperado:
                raise ValueError(f"Comparacao Dijkstra incompleta em {chave}.")
            valores = [x["valor_solucao"] for x in grupo.values()]
            for linha in grupo.values():
                if not proximos(linha["valor_solucao"], linha["otimo_referencia"]):
                    raise ValueError(f"Caminho nao minimo em {chave}.")
                if not proximos(linha["razao_aproximacao"], 1.0):
                    raise ValueError(f"Razao de Dijkstra inconsistente em {chave}.")
            if not proximos(valores[0], valores[1]):
                raise ValueError(f"Lista/Matriz divergentes em Dijkstra: {chave}.")
        else:
            exatas = [x for (a, _), x in grupo.items() if a == "BacktrackingExato"]
            aproximadas = [x for (a, _), x in grupo.items() if a == "Aproximacao2"]
            if exatas:
                if len(exatas) != 2 or len(aproximadas) != 2:
                    raise ValueError(f"Comparacao exata/aproximada incompleta em {chave}.")
                otimo = exatas[0]["valor_solucao"]
                if any(not proximos(x["valor_solucao"], otimo) for x in exatas):
                    raise ValueError(f"Otimos exatos divergentes em {chave}.")
                if any(not proximos(x["otimo_referencia"], otimo) or
                       not proximos(x["razao_aproximacao"], 1.0)
                       for x in exatas):
                    raise ValueError(f"Certificado exato inconsistente em {chave}.")
                for linha in aproximadas:
                    if (not proximos(linha["otimo_referencia"], otimo) or
                            linha["valor_solucao"] < otimo or
                            linha["razao_aproximacao"] > 2.0 + 1e-12 or
                            not proximos(linha["razao_aproximacao"], linha["valor_solucao"] / otimo)):
                        raise ValueError(f"Garantia da aproximacao 2 violada em {chave}.")
            elif set(grupo) != {("Aproximacao2", "lista"), ("Aproximacao2", "matriz")}:
                raise ValueError(f"Comparacao aproximativa incompleta em {chave}.")
            elif any(x["otimo_referencia"] != -1.0 or
                     x["razao_aproximacao"] != -1.0 for x in aproximadas):
                raise ValueError(f"Otimo indevido na escala polinomial em {chave}.")
    return True


def verificar_escalas_protocolo(linhas):
    esperadas = set()
    for n in (100, 500, 1000):
        for algoritmo in ("Prim", "Kruskal"):
            for representacao in ("lista", "matriz"):
                esperadas.add(("AGM", algoritmo, representacao, n))
        for representacao in ("lista", "matriz"):
            esperadas.add(("CaminhoMinimo", "Dijkstra", representacao, n))
            esperadas.add(("CoberturaVertices", "Aproximacao2", representacao, n))
    for n in (9, 11, 13):
        for algoritmo in ("BacktrackingExato", "Aproximacao2"):
            for representacao in ("lista", "matriz"):
                esperadas.add(("CoberturaVertices", algoritmo, representacao, n))
    observadas = defaultdict(set)
    for linha in linhas:
        chave = (linha["problema"], linha["algoritmo"],
                 linha["representacao"], linha["N"])
        observadas[chave].add(linha["repeticao"])
    if set(observadas) != esperadas:
        faltantes = sorted(esperadas - set(observadas))
        extras = sorted(set(observadas) - esperadas)
        raise ValueError(f"Protocolo incompleto: faltantes={faltantes}, extras={extras}.")
    conjuntos = list(observadas.values())
    if any(conjunto != conjuntos[0] for conjunto in conjuntos[1:]):
        raise ValueError("Quantidade/conjunto de repeticoes difere entre metodos.")
    return True


def consolidar(linhas, diretorio):
    grupos = defaultdict(list)
    for linha in linhas:
        chave = tuple(linha[c] for c in (
            "cenario", "problema", "algoritmo", "representacao", "N",
            "arestas", "semente", "parametros"))
        grupos[chave].append(linha)
    saida = []
    for chave, amostras in sorted(grupos.items()):
        repeticoes = [x["repeticao"] for x in amostras]
        if len(set(repeticoes)) != len(repeticoes) or set(repeticoes) != set(range(1, len(amostras) + 1)):
            raise ValueError(f"Repeticoes duplicadas ou com lacunas em {chave}.")
        tempos = [x["tempo_ms"] for x in amostras]
        auxiliares = [x["memoria_auxiliar_pico_bytes"] for x in amostras]
        totais = [x["memoria_total_estimada_bytes"] for x in amostras]
        valores = [x["valor_solucao"] for x in amostras]
        razoes = [x["razao_aproximacao"] for x in amostras]
        referencia = amostras[0]["otimo_referencia"]
        if any(not proximos(x["otimo_referencia"], referencia) for x in amostras):
            raise ValueError(f"Referencia variou entre repeticoes em {chave}.")
        saida.append(dict(zip(CAMPOS_CONSOLIDADOS, [
            *chave, f"{statistics.median(tempos):.9f}", f"{statistics.fmean(tempos):.9f}",
            amostras[0]["memoria_representacao_bytes"], int(statistics.median(auxiliares)),
            f"{statistics.fmean(auxiliares):.3f}", int(statistics.median(totais)),
            f"{statistics.median(valores):.17g}", f"{referencia:.17g}",
            f"{statistics.median(razoes):.17g}", len(amostras), "sim",
        ])))
    diretorio.mkdir(parents=True, exist_ok=True)
    with (diretorio / "fase2_consolidado.csv").open("w", newline="", encoding="utf-8") as arquivo:
        escritor = csv.DictWriter(arquivo, fieldnames=CAMPOS_CONSOLIDADOS, lineterminator="\n")
        escritor.writeheader()
        escritor.writerows(saida)
    return saida


def grafico_svg(caminho, titulo, unidade, series):
    largura, altura = 960, 540
    esquerda, topo, area_w, area_h = 90, 70, 820, 390
    xs = sorted({x for pontos in series.values() for x, _ in pontos})
    maximo = max([y for pontos in series.values() for _, y in pontos] or [1.0]) or 1.0
    cores = ["#1665a7", "#d95f02", "#2a9d4b", "#8e44ad", "#b22222", "#008b8b"]
    partes = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{largura}" height="{altura}" viewBox="0 0 {largura} {altura}">',
              '<rect width="100%" height="100%" fill="white"/>',
              f'<text x="480" y="30" text-anchor="middle" font-family="sans-serif" font-size="20">{escape(titulo)}</text>',
              f'<line x1="{esquerda}" y1="{topo}" x2="{esquerda}" y2="{topo+area_h}" stroke="#333"/>',
              f'<line x1="{esquerda}" y1="{topo+area_h}" x2="{esquerda+area_w}" y2="{topo+area_h}" stroke="#333"/>']
    for i in range(6):
        valor = maximo * i / 5
        y = topo + area_h - area_h * i / 5
        partes += [f'<line x1="{esquerda}" y1="{y:.1f}" x2="{esquerda+area_w}" y2="{y:.1f}" stroke="#ddd"/>',
                   f'<text x="{esquerda-8}" y="{y+4:.1f}" text-anchor="end" font-family="sans-serif" font-size="11">{valor:.4g}</text>']
    for i, xvalor in enumerate(xs):
        x = esquerda + area_w * i / max(1, len(xs)-1)
        partes.append(f'<text x="{x:.1f}" y="484" text-anchor="middle" font-family="sans-serif" font-size="12">{xvalor}</text>')
    for i, (nome, pontos) in enumerate(sorted(series.items())):
        cor = cores[i % len(cores)]
        coords = []
        for xvalor, valor in sorted(pontos):
            x = esquerda + area_w * xs.index(xvalor) / max(1, len(xs)-1)
            y = topo + area_h - valor / maximo * area_h
            coords.append(f"{x:.1f},{y:.1f}")
            partes.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4" fill="{cor}"/>')
        partes += [f'<polyline points="{" ".join(coords)}" fill="none" stroke="{cor}" stroke-width="2"/>',
                   f'<text x="{esquerda+10}" y="{topo+16*i+12}" font-family="sans-serif" font-size="11" fill="{cor}">{escape(nome)}</text>']
    partes += [f'<text x="480" y="525" text-anchor="middle" font-family="sans-serif">Vertices (N)</text>',
               f'<text x="20" y="270" transform="rotate(-90 20 270)" text-anchor="middle" font-family="sans-serif">{escape(unidade)}</text>', "</svg>"]
    caminho.write_text("\n".join(partes), encoding="utf-8")


def gerar_graficos(linhas, diretorio):
    tempo, memoria, agm, dijkstra, cobertura = (defaultdict(list) for _ in range(5))
    for linha in linhas:
        rotulo = f"{linha['algoritmo']} - {linha['representacao']}"
        n = int(linha["N"])
        tempo[f"{linha['problema']}: {rotulo}"].append((n, float(linha["tempo_mediana_ms"])))
        memoria[f"{linha['problema']}: {rotulo}"].append((n, float(linha["memoria_total_estimada_mediana_bytes"])))
        if linha["problema"] == "AGM": agm[rotulo].append((n, float(linha["valor_solucao_mediana"])))
        if linha["problema"] == "CaminhoMinimo": dijkstra[rotulo].append((n, float(linha["valor_solucao_mediana"])))
        if linha["problema"] == "CoberturaVertices" and float(linha["razao_aproximacao_mediana"]) >= 0:
            cobertura[rotulo].append((n, float(linha["razao_aproximacao_mediana"])))
    graficos = [
        ("fase2_tempo.svg", "Fase II: tempo mediano", "tempo (ms)", tempo),
        ("fase2_memoria.svg", "Fase II: memoria total estimada", "bytes", memoria),
        ("fase2_agm_custo.svg", "AGM: custo validado", "peso sintetico", agm),
        ("fase2_dijkstra_custo.svg", "Dijkstra: distancia validada", "peso sintetico", dijkstra),
        ("fase2_cobertura_qualidade.svg", "Cobertura: razao para o otimo", "razao", cobertura),
    ]
    for nome, titulo, unidade, series in graficos:
        grafico_svg(diretorio / nome, titulo, unidade, series)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--entrada", type=Path, default=Path("resultados/fase2_bruto.csv"))
    parser.add_argument("--diretorio", type=Path, default=Path("resultados"))
    args = parser.parse_args(argv)
    try:
        linhas = carregar(args.entrada)
        verificar_escalas_protocolo(linhas)
        consolidadas = consolidar(linhas, args.diretorio)
        gerar_graficos(consolidadas, args.diretorio)
    except (OSError, ValueError, csv.Error) as erro:
        print(f"Erro nos resultados da Fase II: {erro}", file=sys.stderr)
        return 1
    print("Gerados: fase2_consolidado.csv e cinco graficos SVG validados.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
