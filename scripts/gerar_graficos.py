#!/usr/bin/env python3
"""Consolida as medicoes da aplicacao C e gera dois SVGs sem dependencias."""

import csv
import statistics
from collections import defaultdict
from pathlib import Path


ENTRADA = Path("resultados/fase1_metricas.csv")
CONSOLIDADO = Path("resultados/fase1_consolidado.csv")


def carregar():
    grupos = defaultdict(list)
    with ENTRADA.open(encoding="utf-8") as arquivo:
        for linha in csv.DictReader(arquivo):
            chave = (
                linha["algoritmo"], linha["representacao"],
                int(linha["vertices"]), int(linha["arestas"]),
                int(linha["memoria_bytes"]),
            )
            grupos[chave].append(float(linha["tempo_ms"]))
    return grupos


def consolidar(grupos):
    linhas = []
    for chave, tempos in sorted(grupos.items(), key=lambda x: (x[0][2], x[0][0], x[0][1])):
        algoritmo, representacao, vertices, arestas, memoria = chave
        linhas.append({
            "algoritmo": algoritmo,
            "representacao": representacao,
            "vertices": vertices,
            "arestas": arestas,
            "tempo_mediana_ms": f"{statistics.median(tempos):.6f}",
            "tempo_media_ms": f"{statistics.mean(tempos):.6f}",
            "memoria_bytes": memoria,
            "repeticoes": len(tempos),
        })
    with CONSOLIDADO.open("w", newline="", encoding="utf-8") as arquivo:
        escritor = csv.DictWriter(arquivo, fieldnames=linhas[0].keys(), lineterminator="\n")
        escritor.writeheader()
        escritor.writerows(linhas)
    return linhas


def grafico_svg(caminho, titulo, series, unidade, logaritmico=False):
    largura, altura = 900, 520
    esquerda, topo, direita, base = 90, 55, 30, 70
    area_w = largura - esquerda - direita
    area_h = altura - topo - base
    tamanhos = sorted({x for valores in series.values() for x, _ in valores})
    maximo = max(y for valores in series.values() for _, y in valores)
    if logaritmico:
        import math
        transformar = lambda y: math.log10(max(y, 1))
        max_y = transformar(maximo) * 1.08
    else:
        transformar = lambda y: y
        max_y = maximo * 1.12
    cores = ["#1565c0", "#ef6c00", "#2e7d32", "#8e24aa", "#c62828", "#00838f"]
    partes = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{largura}" height="{altura}" viewBox="0 0 {largura} {altura}">',
              '<rect width="100%" height="100%" fill="white"/>',
              f'<text x="{largura/2}" y="28" text-anchor="middle" font-family="sans-serif" font-size="20">{titulo}</text>',
              f'<line x1="{esquerda}" y1="{topo}" x2="{esquerda}" y2="{topo+area_h}" stroke="#333"/>',
              f'<line x1="{esquerda}" y1="{topo+area_h}" x2="{esquerda+area_w}" y2="{topo+area_h}" stroke="#333"/>']
    for i in range(6):
        valor = maximo * i / 5
        y = topo + area_h - (transformar(valor) / max_y * area_h if valor else 0)
        rotulo = f"{valor:.3g}"
        partes += [f'<line x1="{esquerda}" y1="{y:.1f}" x2="{esquerda+area_w}" y2="{y:.1f}" stroke="#ddd"/>',
                   f'<text x="{esquerda-8}" y="{y+4:.1f}" text-anchor="end" font-family="sans-serif" font-size="12">{rotulo}</text>']
    for i, n in enumerate(tamanhos):
        x = esquerda + (area_w * i / max(1, len(tamanhos)-1))
        partes.append(f'<text x="{x:.1f}" y="{topo+area_h+25}" text-anchor="middle" font-family="sans-serif" font-size="13">{n}</text>')
    for indice, (nome, valores) in enumerate(series.items()):
        cor = cores[indice % len(cores)]
        pontos = []
        for n, valor in valores:
            x = esquerda + area_w * tamanhos.index(n) / max(1, len(tamanhos)-1)
            y = topo + area_h - transformar(valor) / max_y * area_h
            pontos.append(f"{x:.1f},{y:.1f}")
            partes.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4" fill="{cor}"/>')
        partes.append(f'<polyline points="{" ".join(pontos)}" fill="none" stroke="{cor}" stroke-width="2"/>')
        ly = topo + 18 * indice
        partes += [f'<line x1="{esquerda+15}" y1="{ly}" x2="{esquerda+38}" y2="{ly}" stroke="{cor}" stroke-width="3"/>',
                   f'<text x="{esquerda+44}" y="{ly+4}" font-family="sans-serif" font-size="12">{nome}</text>']
    partes += [f'<text x="{largura/2}" y="{altura-18}" text-anchor="middle" font-family="sans-serif">Vertices (N)</text>',
               f'<text x="18" y="{altura/2}" transform="rotate(-90 18 {altura/2})" text-anchor="middle" font-family="sans-serif">{unidade}</text>',
               '</svg>']
    caminho.write_text("\n".join(partes), encoding="utf-8")


def main():
    linhas = consolidar(carregar())
    tempos = defaultdict(list)
    memorias = defaultdict(list)
    for linha in linhas:
        tempos[f'{linha["algoritmo"]} - {linha["representacao"]}'].append(
            (linha["vertices"], float(linha["tempo_mediana_ms"]))
        )
        if linha["algoritmo"] == "BFS":
            memorias[linha["representacao"]].append(
                (linha["vertices"], linha["memoria_bytes"])
            )
    grafico_svg(Path("resultados/tempo_lista_matriz.svg"),
                "Tempo mediano por algoritmo e representacao", tempos, "tempo (ms)")
    grafico_svg(Path("resultados/memoria_lista_matriz.svg"),
                "Memoria estimada das representacoes", memorias, "bytes", True)
    print(f"Gerados: {CONSOLIDADO} e 2 graficos SVG")


if __name__ == "__main__":
    main()
