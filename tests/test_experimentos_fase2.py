import csv
import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

RAIZ = Path(__file__).resolve().parents[1]
SCRIPT = RAIZ / "scripts" / "analisar_experimentos_fase2.py"
EXECUTAVEL = None
ESPEC = importlib.util.spec_from_file_location("fase2", SCRIPT)
ANALISADOR = importlib.util.module_from_spec(ESPEC)
ESPEC.loader.exec_module(ANALISADOR)


def linha(algoritmo, representacao, problema="AGM", solucao=12.0,
          referencia=12.0, razao=1.0, status="OK", repeticao=1):
    estrutural, auxiliar = 8192, 4096
    return {
        "cenario": "sintetico_conectado_ciclico_pesos_assumidos",
        "problema": problema, "algoritmo": algoritmo,
        "representacao": representacao, "N": "100", "arestas": "299",
        "repeticao": str(repeticao), "semente": "20261206",
        "parametros": "arvore_geradora+2N_arestas_extras",
        "tempo_ms": "0.125", "memoria_representacao_bytes": str(estrutural),
        "memoria_auxiliar_pico_bytes": str(auxiliar),
        "memoria_total_estimada_bytes": str(estrutural + auxiliar),
        "valor_solucao": str(solucao), "otimo_referencia": str(referencia),
        "razao_aproximacao": str(razao), "status": status,
    }


def conjunto_agm(repeticao=1):
    return [linha(a, r, repeticao=repeticao)
            for a in ("Prim", "Kruskal") for r in ("lista", "matriz")]


def gravar(caminho, linhas):
    with caminho.open("w", newline="", encoding="utf-8") as arquivo:
        escritor = csv.DictWriter(arquivo, fieldnames=ANALISADOR.CAMPOS_BRUTOS,
                                  lineterminator="\n")
        escritor.writeheader()
        escritor.writerows(linhas)


class TestarExperimentosFase2(unittest.TestCase):
    def test_consolida_e_gera_cinco_svgs(self):
        with tempfile.TemporaryDirectory() as temporario:
            diretorio = Path(temporario)
            entrada = diretorio / "bruto.csv"
            gravar(entrada, conjunto_agm())
            dados = ANALISADOR.consolidar(ANALISADOR.carregar(entrada), diretorio)
            ANALISADOR.gerar_graficos(dados, diretorio)
            self.assertEqual(len(dados), 4)
            for nome in ("fase2_tempo.svg", "fase2_memoria.svg",
                         "fase2_agm_custo.svg", "fase2_dijkstra_custo.svg",
                         "fase2_cobertura_qualidade.svg"):
                conteudo = (diretorio / nome).read_text(encoding="utf-8")
                self.assertIn("<svg ", conteudo)
                self.assertIn("</svg>", conteudo)

    def test_calcula_media_mediana_e_total(self):
        amostras = conjunto_agm(1) + conjunto_agm(2)
        for item in amostras[4:]:
            item["tempo_ms"] = "3.125"
        with tempfile.TemporaryDirectory() as temporario:
            entrada = Path(temporario) / "bruto.csv"
            gravar(entrada, amostras)
            dados = ANALISADOR.consolidar(ANALISADOR.carregar(entrada), Path(temporario))
        self.assertTrue(all(float(x["tempo_media_ms"]) == 1.625 for x in dados))
        self.assertTrue(all(int(x["memoria_total_estimada_mediana_bytes"]) == 12288 for x in dados))

    def test_rejeita_status_memoria_e_csv_incompletos(self):
        with tempfile.TemporaryDirectory() as temporario:
            entrada = Path(temporario) / "bruto.csv"
            dados = conjunto_agm()
            dados[0]["status"] = "ERRO_VALIDACAO"
            gravar(entrada, dados)
            with self.assertRaisesRegex(ValueError, "status experimental"):
                ANALISADOR.carregar(entrada)
            dados = conjunto_agm()
            dados[0]["memoria_total_estimada_bytes"] = "1"
            gravar(entrada, dados)
            with self.assertRaisesRegex(ValueError, "memoria total inconsistente"):
                ANALISADOR.carregar(entrada)
            gravar(entrada, conjunto_agm()[:-1])
            with self.assertRaisesRegex(ValueError, "AGM incompleta"):
                ANALISADOR.carregar(entrada)

    def test_rejeita_dijkstra_invalido(self):
        dados = [linha("Dijkstra", r, "CaminhoMinimo", solucao=10,
                       referencia=9) for r in ("lista", "matriz")]
        with tempfile.TemporaryDirectory() as temporario:
            entrada = Path(temporario) / "bruto.csv"
            gravar(entrada, dados)
            with self.assertRaisesRegex(ValueError, "Caminho nao minimo"):
                ANALISADOR.carregar(entrada)

    def test_rejeita_protocolo_sem_todas_as_escalas(self):
        with self.assertRaisesRegex(ValueError, "Protocolo incompleto"):
            ANALISADOR.verificar_escalas_protocolo([
                {**linha("Prim", "lista"), "N": 100, "repeticao": 1}
            ])

    def test_rejeita_aproximacao_acima_de_dois(self):
        dados = []
        for representacao in ("lista", "matriz"):
            dados.append(linha("BacktrackingExato", representacao,
                               "CoberturaVertices", 4, 4, 1))
            dados.append(linha("Aproximacao2", representacao,
                               "CoberturaVertices", 9, 4, 2.25))
        with tempfile.TemporaryDirectory() as temporario:
            entrada = Path(temporario) / "bruto.csv"
            gravar(entrada, dados)
            with self.assertRaisesRegex(ValueError, "Garantia"):
                ANALISADOR.carregar(entrada)

    def test_metadados_e_escalas_da_execucao(self):
        if EXECUTAVEL is None:
            self.skipTest("Makefile fornece o runner.")
        diretorio = EXECUTAVEL.parent
        ambiente = (diretorio / "fase2_teste_ambiente.txt").read_text(encoding="utf-8")
        for campo in ("compilador=", "relogio=", "repeticoes=1",
                      "tamanhos_polinomiais=100,500,1000",
                      "tamanhos_cobertura_exata=9,11,13"):
            self.assertIn(campo, ambiente)
        with (diretorio / "fase2_teste_bruto.csv").open(newline="", encoding="utf-8") as arquivo:
            dados = list(csv.DictReader(arquivo))
        self.assertEqual({int(x["N"]) for x in dados if x["problema"] == "CaminhoMinimo"}, {100, 500, 1000})
        self.assertEqual({int(x["N"]) for x in dados if x["algoritmo"] == "BacktrackingExato"}, {9, 11, 13})

    def test_runner_reproduz_instancias_e_solucoes(self):
        if EXECUTAVEL is None:
            self.skipTest("Makefile fornece o runner.")
        with tempfile.TemporaryDirectory() as temporario:
            diretorio = Path(temporario)
            arquivos = []
            for indice in (1, 2):
                csv_saida = diretorio / f"r{indice}.csv"
                processo = subprocess.run([
                    str(EXECUTAVEL), "--saida", str(csv_saida), "--ambiente",
                    str(diretorio / f"a{indice}.txt"), "--repeticoes", "1",
                    "--semente", "12345"], capture_output=True, text=True)
                self.assertEqual(processo.returncode, 0, processo.stderr)
                with csv_saida.open(newline="", encoding="utf-8") as arquivo:
                    arquivos.append(list(csv.DictReader(arquivo)))
            self.assertEqual(len(arquivos[0]), len(arquivos[1]))
            ignorar = {"tempo_ms"}
            self.assertEqual([{k: v for k, v in x.items() if k not in ignorar} for x in arquivos[0]],
                             [{k: v for k, v in x.items() if k not in ignorar} for x in arquivos[1]])

    def test_runner_rejeita_parametros_invalidos(self):
        if EXECUTAVEL is None:
            self.skipTest("Makefile fornece o runner.")
        for argumentos in (("--repeticoes", "0"), ("--repeticoes", "101"),
                           ("--semente", "0"), ("--desconhecida",)):
            processo = subprocess.run([str(EXECUTAVEL), *argumentos],
                                      capture_output=True, text=True)
            self.assertNotEqual(processo.returncode, 0)


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "--runner":
        EXECUTAVEL = Path(sys.argv[2]).resolve()
        del sys.argv[1:3]
    unittest.main()
