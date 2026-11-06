#!/usr/bin/env python3
"""Valida e consolida os experimentos da Fase II e gera SVGs sem dependencias."""

import argparse
import csv
import math
import statistics
import sys
from collections import defaultdict
from pathlib import Path
from xml.sax.saxutils import escape


CAMPOS_BRUTOS = [
    "cenario",
    "algoritmo",
    "representacao",
    "N",
    "arestas",
    "repeticao",
    "semente",
    "parametros",
    "tempo_ms",
    "memoria_representacao_bytes",
    "memoria_auxiliar_pico_bytes",
    "valor_solucao",
    "status",
]
CAMPOS_CONSOLIDADOS = [
    "cenario",
    "algoritmo",
    "representacao",
    "N",
    "arestas",
    "semente",
    "parametros",
    "tempo_mediana_ms",
    "tempo_media_ms",
    "memoria_representacao_bytes",
    "memoria_auxiliar_pico_mediana_bytes",
    "memoria_auxiliar_pico_media_bytes",
    "valor_solucao_mediana",
    "valor_solucao_minimo",
    "valor_solucao_maximo",
    "repeticoes",
    "equivalencia_validada",
]


def numero_inteiro(valor, campo, linha):
    try:
        resultado = int(valor)
    except (TypeError, ValueError) as erro:
        raise ValueError(f"Linha {linha}: {campo} nao e inteiro.") from erro
    if resultado < 0:
        raise ValueError(f"Linha {linha}: {campo} nao pode ser negativo.")
    return resultado


def carregar(caminho):
    with caminho.open(newline="", encoding="utf-8") as arquivo:
        leitor = csv.DictReader(arquivo)
        if leitor.fieldnames != CAMPOS_BRUTOS:
            raise ValueError("Cabecalho CSV bruto invalido ou incompleto.")
        linhas = []
        for numero_linha, linha in enumerate(leitor, start=2):
            if None in linha or any(valor is None for valor in linha.values()):
                raise ValueError(f"Linha {numero_linha}: formato CSV invalido.")
            if linha["status"] != "OK":
                raise ValueError(
                    f"Linha {numero_linha}: status experimental "
                    f"{linha['status']!r} nao e valido."
                )
            if not linha["cenario"] or not linha["parametros"]:
                raise ValueError(f"Linha {numero_linha}: cenario/parametros vazios.")
            if linha["algoritmo"] not in {"Prim", "Kruskal"}:
                raise ValueError(
                    f"Linha {numero_linha}: algoritmo desconhecido."
                )
            if linha["representacao"] not in {"lista", "matriz"}:
                raise ValueError(
                    f"Linha {numero_linha}: representacao desconhecida."
                )
            n = numero_inteiro(linha["N"], "N", numero_linha)
            arestas = numero_inteiro(linha["arestas"], "arestas", numero_linha)
            repeticao = numero_inteiro(
                linha["repeticao"], "repeticao", numero_linha
            )
            semente = numero_inteiro(linha["semente"], "semente", numero_linha)
            if semente > 0xFFFFFFFF:
                raise ValueError(f"Linha {numero_linha}: semente fora de uint32_t.")
            memoria = numero_inteiro(
                linha["memoria_representacao_bytes"],
                "memoria_representacao_bytes",
                numero_linha,
            )
            memoria_auxiliar = numero_inteiro(
                linha["memoria_auxiliar_pico_bytes"],
                "memoria_auxiliar_pico_bytes",
                numero_linha,
            )
            if n < 1 or arestas <= n - 1 or repeticao < 1:
                raise ValueError(
                    f"Linha {numero_linha}: instancias AGM devem ser conectadas "
                    "e ciclicas, com dimensoes validas."
                )
            if (
                linha["cenario"] == "sintetico_conectado_ciclico_pesos_assumidos"
                and arestas != 3 * n - 1
            ):
                raise ValueError(
                    f"Linha {numero_linha}: E esperado deve ser 3N-1."
                )
            try:
                tempo = float(linha["tempo_ms"])
                solucao = float(linha["valor_solucao"])
            except ValueError as erro:
                raise ValueError(
                    f"Linha {numero_linha}: tempo/solucao invalido."
                ) from erro
            if not math.isfinite(tempo) or tempo < 0.0:
                raise ValueError(f"Linha {numero_linha}: tempo nao finito/negativo.")
            if not math.isfinite(solucao) or solucao < 0.0:
                raise ValueError(
                    f"Linha {numero_linha}: valor de solucao invalido."
                )
            linhas.append(
                {
                    **linha,
                    "N": n,
                    "arestas": arestas,
                    "repeticao": repeticao,
                    "semente": semente,
                    "tempo_ms": tempo,
                    "memoria_representacao_bytes": memoria,
                    "memoria_auxiliar_pico_bytes": memoria_auxiliar,
                    "valor_solucao": solucao,
                }
            )
    if not linhas:
        raise ValueError("CSV bruto sem medicoes.")
    return linhas


def custos_equivalentes(primeiro, segundo):
    return math.isclose(primeiro, segundo, rel_tol=1e-12, abs_tol=1e-12)


def verificar_equivalencia(linhas):
    grupos = defaultdict(dict)
    for linha in linhas:
        chave = (
            linha["cenario"],
            linha["N"],
            linha["repeticao"],
            linha["semente"],
            linha["parametros"],
        )
        par = (linha["algoritmo"], linha["representacao"])
        if par in grupos[chave]:
            raise ValueError(f"Medicao duplicada para {chave} / {par}.")
        grupos[chave][par] = linha["valor_solucao"]
    for chave, custos in grupos.items():
        pares_esperados = {
            (algoritmo, representacao)
            for algoritmo in ("Prim", "Kruskal")
            for representacao in ("lista", "matriz")
        }
        if set(custos) != pares_esperados:
            raise ValueError(
                f"Comparacao incompleta para cenario/tamanho/repeticao {chave}."
            )
        valores = list(custos.values())
        if any(not custos_equivalentes(valores[0], valor) for valor in valores[1:]):
            raise ValueError(
                f"Custos Prim/Kruskal ou Lista/Matriz divergentes em {chave}."
            )
    return {
        (
            linha["cenario"],
            linha["N"],
            linha["repeticao"],
            linha["semente"],
            linha["parametros"],
        )
        for linha in linhas
    }


def consolidar(linhas, diretorio):
    instancias_validadas = verificar_equivalencia(linhas)
    grupos = defaultdict(list)
    for linha in linhas:
        chave = (
            linha["cenario"],
            linha["algoritmo"],
            linha["representacao"],
            linha["N"],
            linha["arestas"],
            linha["semente"],
            linha["parametros"],
            linha["memoria_representacao_bytes"],
        )
        grupos[chave].append(linha)

    consolidadas = []
    for chave, amostras in sorted(
        grupos.items(), key=lambda item: (item[0][3], item[0][1], item[0][2])
    ):
        (
            cenario,
            algoritmo,
            representacao,
            n,
            arestas,
            semente,
            parametros,
            memoria_representacao,
        ) = chave
        tempos = [linha["tempo_ms"] for linha in amostras]
        memorias = [linha["memoria_auxiliar_pico_bytes"] for linha in amostras]
        solucoes = [linha["valor_solucao"] for linha in amostras]
        consolidadas.append(
            {
                "cenario": cenario,
                "algoritmo": algoritmo,
                "representacao": representacao,
                "N": n,
                "arestas": arestas,
                "semente": semente,
                "parametros": parametros,
                "tempo_mediana_ms": f"{statistics.median(tempos):.9f}",
                "tempo_media_ms": f"{statistics.mean(tempos):.9f}",
                "memoria_representacao_bytes": memoria_representacao,
                "memoria_auxiliar_pico_mediana_bytes": statistics.median(
                    memorias
                ),
                "memoria_auxiliar_pico_media_bytes": f"{statistics.mean(memorias):.3f}",
                "valor_solucao_mediana": f"{statistics.median(solucoes):.17g}",
                "valor_solucao_minimo": f"{min(solucoes):.17g}",
                "valor_solucao_maximo": f"{max(solucoes):.17g}",
                "repeticoes": len(amostras),
                "equivalencia_validada": "sim"
                if all(
                    (
                        cenario,
                        n,
                        amostra["repeticao"],
                        semente,
                        parametros,
                    )
                    in instancias_validadas
                    for amostra in amostras
                )
                else "nao",
            }
        )
    destino = diretorio / "fase2_consolidado.csv"
    with destino.open("w", newline="", encoding="utf-8") as arquivo:
        escritor = csv.DictWriter(
            arquivo, fieldnames=CAMPOS_CONSOLIDADOS, lineterminator="\n"
        )
        escritor.writeheader()
        escritor.writerows(consolidadas)
    return consolidadas


def grafico_svg(caminho, titulo, unidade, series):
    largura, altura = 960, 560
    esquerda, topo, direita, base = 100, 65, 40, 85
    area_w = largura - esquerda - direita
    area_h = altura - topo - base
    tamanhos = sorted({n for pontos in series.values() for n, _ in pontos})
    maximo = max((valor for pontos in series.values() for _, valor in pontos), default=0)
    limite_y = maximo * 1.12 if maximo > 0 else 1.0
    cores = ["#1565c0", "#ef6c00", "#2e7d32", "#8e24aa", "#c62828", "#00838f"]
    partes = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{largura}" '
        f'height="{altura}" viewBox="0 0 {largura} {altura}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{largura / 2:.1f}" y="30" text-anchor="middle" '
        f'font-family="sans-serif" font-size="20">{escape(titulo)}</text>',
        f'<line x1="{esquerda}" y1="{topo}" x2="{esquerda}" '
        f'y2="{topo + area_h}" stroke="#333"/>',
        f'<line x1="{esquerda}" y1="{topo + area_h}" '
        f'x2="{esquerda + area_w}" y2="{topo + area_h}" stroke="#333"/>',
    ]
    for indice in range(6):
        valor = limite_y * indice / 5
        y = topo + area_h - valor / limite_y * area_h
        partes.extend(
            [
                f'<line x1="{esquerda}" y1="{y:.1f}" '
                f'x2="{esquerda + area_w}" y2="{y:.1f}" stroke="#ddd"/>',
                f'<text x="{esquerda - 8}" y="{y + 4:.1f}" text-anchor="end" '
                f'font-family="sans-serif" font-size="12">{valor:.4g}</text>',
            ]
        )
    for indice, n in enumerate(tamanhos):
        x = esquerda + area_w * indice / max(1, len(tamanhos) - 1)
        partes.append(
            f'<text x="{x:.1f}" y="{topo + area_h + 24}" text-anchor="middle" '
            f'font-family="sans-serif" font-size="13">{n}</text>'
        )
    for indice, (nome, pontos) in enumerate(sorted(series.items())):
        cor = cores[indice % len(cores)]
        coordenadas = []
        for n, valor in sorted(pontos):
            x = esquerda + area_w * tamanhos.index(n) / max(1, len(tamanhos) - 1)
            y = topo + area_h - valor / limite_y * area_h
            coordenadas.append(f"{x:.1f},{y:.1f}")
            partes.append(
                f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4" fill="{cor}"/>'
            )
        if coordenadas:
            partes.append(
                f'<polyline points="{" ".join(coordenadas)}" fill="none" '
                f'stroke="{cor}" stroke-width="2"/>'
            )
        ly = topo + 18 * indice
        partes.extend(
            [
                f'<line x1="{esquerda + 10}" y1="{ly}" '
                f'x2="{esquerda + 32}" y2="{ly}" stroke="{cor}" stroke-width="3"/>',
                f'<text x="{esquerda + 38}" y="{ly + 4}" '
                f'font-family="sans-serif" font-size="12">{escape(nome)}</text>',
            ]
        )
    partes.extend(
        [
            f'<text x="{largura / 2}" y="{altura - 18}" text-anchor="middle" '
            'font-family="sans-serif">Vertices (N)</text>',
            f'<text x="20" y="{altura / 2}" '
            f'transform="rotate(-90 20 {altura / 2})" text-anchor="middle" '
            f'font-family="sans-serif">{escape(unidade)}</text>',
            "</svg>",
        ]
    )
    caminho.write_text("\n".join(partes), encoding="utf-8")


def gerar_graficos(linhas, diretorio):
    tempo = defaultdict(list)
    memoria_representacao = defaultdict(list)
    memoria_auxiliar = defaultdict(list)
    qualidade = defaultdict(list)
    for linha in linhas:
        algoritmo = linha["algoritmo"]
        representacao = linha["representacao"]
        rotulo = f"{algoritmo} - {representacao}"
        n = int(linha["N"])
        tempo[rotulo].append((n, float(linha["tempo_mediana_ms"])))
        memoria_representacao[representacao].append(
            (n, float(linha["memoria_representacao_bytes"]))
        )
        memoria_auxiliar[rotulo].append(
            (n, float(linha["memoria_auxiliar_pico_mediana_bytes"]))
        )
        qualidade[rotulo].append(
            (n, float(linha["valor_solucao_mediana"]))
        )
    graficos = [
        (
            "fase2_tempo.svg",
            "Fase II: tempo mediano (AGM)",
            "tempo (ms)",
            tempo,
        ),
        (
            "fase2_memoria.svg",
            "Fase II: representacao e pico auxiliar AGM",
            "bytes solicitados (nao RSS)",
            {**memoria_representacao, **memoria_auxiliar},
        ),
        (
            "fase2_qualidade.svg",
            "Fase II: custo numerico da AGM validado",
            "soma dos pesos assumidos (sem unidade fisica)",
            qualidade,
        ),
    ]
    for nome, titulo, unidade, series in graficos:
        grafico_svg(diretorio / nome, titulo, unidade, series)


def main(argv=None):
    analisador = argparse.ArgumentParser(description=__doc__)
    analisador.add_argument(
        "--entrada", type=Path, default=Path("resultados/fase2_bruto.csv")
    )
    analisador.add_argument(
        "--diretorio", type=Path, default=Path("resultados")
    )
    argumentos = analisador.parse_args(argv)
    try:
        linhas = carregar(argumentos.entrada)
        argumentos.diretorio.mkdir(parents=True, exist_ok=True)
        consolidadas = consolidar(linhas, argumentos.diretorio)
        gerar_graficos(consolidadas, argumentos.diretorio)
    except (OSError, ValueError, csv.Error) as erro:
        print(f"Erro nos resultados da Fase II: {erro}", file=sys.stderr)
        return 1
    print(
        "Gerados: fase2_consolidado.csv, fase2_tempo.svg, "
        "fase2_memoria.svg e fase2_qualidade.svg"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
