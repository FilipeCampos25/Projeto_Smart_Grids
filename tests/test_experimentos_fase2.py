import csv
import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


RAIZ = Path(__file__).resolve().parents[1]
ARQUIVO_SCRIPT = RAIZ / "scripts" / "analisar_experimentos_fase2.py"
EXECUTAVEL_EXPERIMENTOS = None
ESPEC = importlib.util.spec_from_file_location(
    "analisar_experimentos_fase2", ARQUIVO_SCRIPT
)
ANALISADOR = importlib.util.module_from_spec(ESPEC)
ESPEC.loader.exec_module(ANALISADOR)


def linha(algoritmo, representacao, custo=12.0, status="OK", repeticao=1):
    return {
        "cenario": "sintetico_conectado_ciclico_pesos_assumidos",
        "algoritmo": algoritmo,
        "representacao": representacao,
        "N": "100",
        "arestas": "299",
        "repeticao": str(repeticao),
        "semente": "20261206",
        "parametros": "arvore_geradora+2N_arestas_extras",
        "tempo_ms": "0.125",
        "memoria_representacao_bytes": "8192",
        "memoria_auxiliar_pico_bytes": "4096",
        "valor_solucao": str(custo),
        "status": status,
    }


def gravar_csv(caminho, linhas):
    with caminho.open("w", newline="", encoding="utf-8") as arquivo:
        escritor = csv.DictWriter(
            arquivo, fieldnames=ANALISADOR.CAMPOS_BRUTOS, lineterminator="\n"
        )
        escritor.writeheader()
        escritor.writerows(linhas)


class TestarExperimentosFase2(unittest.TestCase):
    def conjunto_valido(self):
        return [
            linha(algoritmo, representacao)
            for algoritmo in ("Prim", "Kruskal")
            for representacao in ("lista", "matriz")
        ]

    def test_consolida_csv_e_gera_graficos_svg(self):
        with tempfile.TemporaryDirectory() as temporario:
            diretorio = Path(temporario)
            entrada = diretorio / "bruto.csv"
            gravar_csv(entrada, self.conjunto_valido())
            consolidadas = ANALISADOR.consolidar(
                ANALISADOR.carregar(entrada), diretorio
            )
            ANALISADOR.gerar_graficos(consolidadas, diretorio)

            with (diretorio / "fase2_consolidado.csv").open(
                newline="", encoding="utf-8"
            ) as arquivo:
                leitor = csv.DictReader(arquivo)
                self.assertEqual(leitor.fieldnames, ANALISADOR.CAMPOS_CONSOLIDADOS)
                dados = list(leitor)
            self.assertEqual(len(dados), 4)
            self.assertTrue(
                all(registro["equivalencia_validada"] == "sim" for registro in dados)
            )
            self.assertTrue(
                all(
                    int(registro["memoria_auxiliar_pico_mediana_bytes"]) > 0
                    for registro in dados
                )
            )
            for nome in (
                "fase2_tempo.svg",
                "fase2_memoria.svg",
                "fase2_qualidade.svg",
            ):
                conteudo = (diretorio / nome).read_text(encoding="utf-8")
                self.assertIn("<svg ", conteudo)
                self.assertIn("</svg>", conteudo)

    def test_mantem_media_e_mediana_de_repeticoes(self):
        amostras = []
        for repeticao, tempo in ((1, "1.0"), (2, "3.0")):
            for medicao in self.conjunto_valido():
                medicao["repeticao"] = str(repeticao)
                medicao["tempo_ms"] = tempo
                amostras.append(medicao)
        with tempfile.TemporaryDirectory() as temporario:
            diretorio = Path(temporario)
            entrada = diretorio / "bruto.csv"
            gravar_csv(entrada, amostras)
            dados = ANALISADOR.consolidar(ANALISADOR.carregar(entrada), diretorio)
        self.assertEqual(len(dados), 4)
        self.assertTrue(
            all(float(registro["tempo_mediana_ms"]) == 2.0 for registro in dados)
        )
        self.assertTrue(
            all(float(registro["tempo_media_ms"]) == 2.0 for registro in dados)
        )
        self.assertTrue(all(int(registro["repeticoes"]) == 2 for registro in dados))

    def test_rejeita_cabecalho_incorreto(self):
        with tempfile.TemporaryDirectory() as temporario:
            entrada = Path(temporario) / "invalido.csv"
            entrada.write_text("algoritmo,vertices\nPrim,100\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Cabecalho"):
                ANALISADOR.carregar(entrada)

    def test_rejeita_status_falho(self):
        with tempfile.TemporaryDirectory() as temporario:
            entrada = Path(temporario) / "falha.csv"
            gravar_csv(
                entrada,
                [linha("Prim", "lista", status="ERRO_VALIDACAO")],
            )
            with self.assertRaisesRegex(ValueError, "status experimental"):
                ANALISADOR.carregar(entrada)

    def test_rejeita_custos_divergentes_ou_comparacao_incompleta(self):
        divergentes = self.conjunto_valido()
        divergentes[0]["valor_solucao"] = "15"
        with self.assertRaisesRegex(ValueError, "divergentes"):
            ANALISADOR.verificar_equivalencia(
                [
                    {
                        **item,
                        "N": int(item["N"]),
                        "repeticao": int(item["repeticao"]),
                        "semente": int(item["semente"]),
                        "valor_solucao": float(item["valor_solucao"]),
                    }
                    for item in divergentes
                ]
            )
        incompletas = self.conjunto_valido()[:-1]
        with self.assertRaisesRegex(ValueError, "incompleta"):
            ANALISADOR.verificar_equivalencia(
                [
                    {
                        **item,
                        "N": int(item["N"]),
                        "repeticao": int(item["repeticao"]),
                        "semente": int(item["semente"]),
                        "valor_solucao": float(item["valor_solucao"]),
                    }
                    for item in incompletas
                ]
            )

    def test_rejeita_metricas_nao_finitas(self):
        with tempfile.TemporaryDirectory() as temporario:
            entrada = Path(temporario) / "nan.csv"
            registro = linha("Prim", "lista")
            registro["tempo_ms"] = "nan"
            gravar_csv(entrada, [registro])
            with self.assertRaisesRegex(ValueError, "tempo nao finito"):
                ANALISADOR.carregar(entrada)

    def test_rejeita_instancia_sintetica_sem_ciclo_ou_com_arestas_inesperadas(self):
        with tempfile.TemporaryDirectory() as temporario:
            entrada = Path(temporario) / "instancia.csv"
            registro = linha("Prim", "lista")
            registro["arestas"] = "99"
            gravar_csv(entrada, [registro])
            with self.assertRaisesRegex(ValueError, "conectadas e ciclicas"):
                ANALISADOR.carregar(entrada)

            registro["arestas"] = "300"
            gravar_csv(entrada, [registro])
            with self.assertRaisesRegex(ValueError, "E esperado"):
                ANALISADOR.carregar(entrada)

    def test_execucao_cli_rejeita_csv_incompleto(self):
        with tempfile.TemporaryDirectory() as temporario:
            diretorio = Path(temporario)
            entrada = diretorio / "bruto.csv"
            gravar_csv(entrada, [linha("Prim", "lista")])
            processo = subprocess.run(
                [
                    sys.executable,
                    str(ARQUIVO_SCRIPT),
                    "--entrada",
                    str(entrada),
                    "--diretorio",
                    str(diretorio),
                ],
                check=False,
                capture_output=True,
                text=True,
            )
        self.assertEqual(processo.returncode, 1)
        self.assertIn("incompleta", processo.stderr)

    def test_metadata_ambiente_da_execucao_c(self):
        diretorio_build = (
            EXECUTAVEL_EXPERIMENTOS.parent
            if EXECUTAVEL_EXPERIMENTOS is not None
            else RAIZ / "build"
        )
        metadados = diretorio_build / "fase2_teste_ambiente.txt"
        if not metadados.exists():
            self.skipTest("Execute test-experimentos-fase2 para gerar os metadados.")
        conteudo = metadados.read_text(encoding="utf-8")
        for campo in (
            "compilador=",
            "padrao_C=",
            "sistema=",
            "arquitetura=",
            "repeticoes=1",
            "semente_base=20261106",
            "tamanhos=100,500,1000",
        ):
            with self.subTest(campo=campo):
                self.assertIn(campo, conteudo)

    def test_executavel_rejeita_parametros_invalidos(self):
        if EXECUTAVEL_EXPERIMENTOS is None:
            self.skipTest("Makefile fornece o caminho do executavel experimental.")
        for argumentos in (
            ("--repeticoes", "0"),
            ("--repeticoes", "101"),
            ("--semente", "0"),
            ("--opcao-inexistente",),
        ):
            with self.subTest(argumentos=argumentos):
                processo = subprocess.run(
                    [str(EXECUTAVEL_EXPERIMENTOS), *argumentos],
                    check=False,
                    capture_output=True,
                    text=True,
                )
                self.assertNotEqual(processo.returncode, 0)


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "--runner":
        EXECUTAVEL_EXPERIMENTOS = Path(sys.argv[2]).resolve()
        del sys.argv[1:3]
    unittest.main()
