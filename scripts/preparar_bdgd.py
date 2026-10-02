#!/usr/bin/env python3
"""Baixa e normaliza o recorte BDGD usado na Fase I.

Dependencias da etapa de preparacao (nao usadas pela aplicacao C):
    python -m pip install geopandas pyogrio
"""

from __future__ import annotations

import argparse
import csv
import tempfile
import urllib.request
import zipfile
from collections import defaultdict, deque
from pathlib import Path

import pyogrio


ITEM_ARCGIS = "cd6c8ecb86c2427493cf4a0d129c2eb4"
URL_BDGD = (
    "https://www.arcgis.com/sharing/rest/content/items/"
    f"{ITEM_ARCGIS}/data"
)
CIRCUITO = "1_SFOR_1"
CAMADAS = {
    "SSDMT": "segmento_mt",
    "UNSEMT": "chave_mt",
    "UNREMT": "regulador_mt",
    "UNCRMT": "compensador_reativo_mt",
}


def localizar_gdb(origem: Path | None, temporario: Path) -> Path:
    if origem is None:
        arquivo_zip = temporario / "forcel_bdgd_2025.gdb.zip"
        print(f"Baixando BDGD oficial: {URL_BDGD}")
        urllib.request.urlretrieve(URL_BDGD, arquivo_zip)
    else:
        arquivo_zip = origem

    if arquivo_zip.is_dir() and arquivo_zip.suffix.lower() == ".gdb":
        return arquivo_zip
    if not zipfile.is_zipfile(arquivo_zip):
        raise ValueError("a origem deve ser um .gdb ou um arquivo .zip valido")
    with zipfile.ZipFile(arquivo_zip) as compactado:
        compactado.extractall(temporario)
    bancos = list(temporario.rglob("*.gdb"))
    if len(bancos) != 1:
        raise ValueError("nao foi encontrado exatamente um .gdb no arquivo")
    return bancos[0]


def ler_recorte(gdb: Path):
    arestas = []
    for camada, tipo in CAMADAS.items():
        informacoes = pyogrio.read_info(gdb, layer=camada)
        campos = ["COD_ID", "PAC_1", "PAC_2", "CTMT"]
        for opcional in ("P_N_OPE", "SIT_ATIV", "COMP"):
            if opcional in informacoes["fields"]:
                campos.append(opcional)
        tabela = pyogrio.read_dataframe(
            gdb, layer=camada, columns=campos, read_geometry=False
        )
        tabela = tabela[tabela["CTMT"] == CIRCUITO]
        if "SIT_ATIV" in tabela:
            tabela = tabela[tabela["SIT_ATIV"] == "AT"]
        if "P_N_OPE" in tabela:
            tabela = tabela[tabela["P_N_OPE"] != "A"]
        for _, registro in tabela.iterrows():
            origem, destino = registro["PAC_1"], registro["PAC_2"]
            if not isinstance(origem, str) or not isinstance(destino, str):
                continue
            comprimento = registro.get("COMP", "")
            if comprimento != comprimento:  # NaN
                comprimento = ""
            arestas.append(
                {
                    "id": str(registro["COD_ID"]),
                    "origem": origem.strip(),
                    "destino": destino.strip(),
                    "tipo": tipo,
                    "comprimento_m": comprimento,
                    "status": "ativo",
                    "circuito": CIRCUITO,
                }
            )

    circuitos = pyogrio.read_dataframe(
        gdb,
        layer="CTMT",
        columns=["COD_ID", "NOME", "SUB", "PAC_INI"],
        read_geometry=False,
    )
    linha = circuitos[circuitos["COD_ID"] == CIRCUITO]
    if len(linha) != 1:
        raise ValueError(f"circuito {CIRCUITO} nao encontrado de forma unica")
    registro = linha.iloc[0]
    return arestas, str(registro["PAC_INI"]), str(registro["SUB"])


def componente_da_subestacao(arestas, origem_subestacao: str):
    adjacencias = defaultdict(list)
    arestas_unicas = {}
    ignoradas = {"lacos": 0, "duplicatas": 0}
    for aresta in arestas:
        origem, destino = aresta["origem"], aresta["destino"]
        if not origem or not destino or origem == destino:
            ignoradas["lacos"] += 1
            continue
        chave = tuple(sorted((origem, destino)))
        if chave in arestas_unicas:
            ignoradas["duplicatas"] += 1
            continue
        arestas_unicas[chave] = aresta
        adjacencias[origem].append(destino)
        adjacencias[destino].append(origem)

    if origem_subestacao not in adjacencias:
        raise ValueError("PAC_INI da subestacao nao aparece na topologia ativa")
    visitados = {origem_subestacao}
    fila = deque([origem_subestacao])
    ordem = []
    while fila:
        atual = fila.popleft()
        ordem.append(atual)
        for vizinho in sorted(adjacencias[atual]):
            if vizinho not in visitados:
                visitados.add(vizinho)
                fila.append(vizinho)
    selecionadas = [
        aresta
        for chave, aresta in arestas_unicas.items()
        if chave[0] in visitados and chave[1] in visitados
    ]
    selecionadas.sort(key=lambda a: (a["origem"], a["destino"], a["id"]))
    return ordem, selecionadas, ignoradas


def gravar(destino: Path, vertices, arestas, subestacao: str):
    destino.mkdir(parents=True, exist_ok=True)
    with (destino / "vertices.csv").open("w", newline="", encoding="utf-8") as arq:
        escritor = csv.writer(arq, lineterminator="\n")
        escritor.writerow(["id", "tipo", "subestacao", "circuito"])
        for identificador in vertices:
            tipo = "subestacao" if identificador == CIRCUITO else "ponto_conexao"
            escritor.writerow([identificador, tipo, subestacao, CIRCUITO])
    with (destino / "arestas.csv").open("w", newline="", encoding="utf-8") as arq:
        campos = [
            "id", "origem", "destino", "tipo", "comprimento_m", "status", "circuito"
        ]
        escritor = csv.DictWriter(arq, fieldnames=campos, lineterminator="\n")
        escritor.writeheader()
        escritor.writerows(arestas)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--origem", type=Path, help=".zip oficial ou diretorio .gdb")
    parser.add_argument(
        "--destino", type=Path, default=Path("data/forcel_2025_circuito_1")
    )
    argumentos = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="bdgd_forcel_") as nome_temporario:
        gdb = localizar_gdb(argumentos.origem, Path(nome_temporario))
        arestas, origem_subestacao, subestacao = ler_recorte(gdb)
        vertices, selecionadas, ignoradas = componente_da_subestacao(
            arestas, origem_subestacao
        )
        gravar(argumentos.destino, vertices, selecionadas, subestacao)
    print(f"Vertices: {len(vertices)}")
    print(f"Arestas: {len(selecionadas)}")
    print(f"Subestacao: {subestacao} (indice externo {origem_subestacao})")
    print(f"Registros ignorados no recorte: {ignoradas}")


if __name__ == "__main__":
    main()
