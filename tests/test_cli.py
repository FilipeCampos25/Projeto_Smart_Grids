import argparse
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


PARSER = argparse.ArgumentParser()
PARSER.add_argument("--executavel", required=True)
ARGUMENTOS, _ = PARSER.parse_known_args()
EXECUTAVEL = Path(ARGUMENTOS.executavel).resolve()
RAIZ = Path(__file__).resolve().parents[1]


def executar(*argumentos, entrada=None, cwd=RAIZ):
    return subprocess.run(
        [str(EXECUTAVEL), *argumentos],
        cwd=cwd,
        input=entrada,
        text=True,
        capture_output=True,
        timeout=45,
        check=False,
    )


class TestarIntegracaoCLI(unittest.TestCase):
    def test_ajuda_e_opcoes_invalidas(self):
        ajuda = executar("--help")
        self.assertEqual(ajuda.returncode, 0)
        self.assertIn("Prim/Kruskal", ajuda.stdout)
        self.assertIn("make experimentos-fase2", ajuda.stdout)

        invalida = executar("--opcao-inexistente")
        self.assertNotEqual(invalida.returncode, 0)
        self.assertIn("Opcao desconhecida", invalida.stderr)

    def test_menu_trata_entrada_invalida_e_eof(self):
        invalida = executar(entrada="nao-numerico\n0\n")
        self.assertEqual(invalida.returncode, 0)
        self.assertIn("Opcao invalida", invalida.stdout)

        fim = executar(entrada="")
        self.assertEqual(fim.returncode, 0)
        self.assertIn("Entrada encerrada", fim.stdout)

    def test_menu_informa_dependencias_pendentes(self):
        processo = executar(entrada="9\n0\n")
        self.assertEqual(processo.returncode, 0)
        self.assertIn("Dijkstra (Issue #33): indisponivel", processo.stdout)
        self.assertIn("Cobertura de Vertices", processo.stdout)
        self.assertIn("make experimentos-fase2", processo.stdout)

    def test_agm_nao_aceita_peso_ausente_implicitamente(self):
        processo = executar(entrada="10\n1\n1\n1\n0\n")
        self.assertEqual(processo.returncode, 0)
        self.assertIn("arestas sem peso", processo.stdout)
        self.assertIn("existe aresta sem peso", processo.stderr)
        self.assertIn("Escolha explicitamente um custo assumido", processo.stdout)

    def test_agm_rejeita_opcoes_e_custo_nao_finito(self):
        opcao_fora_do_intervalo = executar(entrada="10\n9\n0\n")
        self.assertEqual(opcao_fora_do_intervalo.returncode, 0)
        self.assertIn("escolha Prim (1) ou Kruskal (2)", opcao_fora_do_intervalo.stdout)

        custo_nao_finito = executar(entrada="10\n1\n1\n2\nnan\n0\n")
        self.assertEqual(custo_nao_finito.returncode, 0)
        self.assertIn("Custo invalido", custo_nao_finito.stdout)

    def test_agm_aceita_custo_assumido_explicito_em_matriz(self):
        processo = executar(entrada="10\n2\n2\n2\n0.5\n0\n")
        self.assertEqual(processo.returncode, 0)
        self.assertIn("AVISO:", processo.stdout)
        self.assertIn("arvore geradora minima completa", processo.stdout)
        self.assertIn("Subtotais separados", processo.stdout)
        self.assertIn("soma total e numerica", processo.stdout.lower())

    def test_demo_cobre_fase_i_e_agm_com_ciclo_sintetico(self):
        with tempfile.TemporaryDirectory() as temporario:
            diretorio = Path(temporario)
            shutil.copytree(
                RAIZ / "data" / "forcel_2025_circuito_1",
                diretorio / "data" / "forcel_2025_circuito_1",
            )
            (diretorio / "resultados").mkdir()
            processo = executar("--demo", cwd=diretorio)
        self.assertEqual(processo.returncode, 0, msg=processo.stderr)
        self.assertIn("DEMONSTRACAO AUTOMATICA", processo.stdout)
        self.assertIn("AGM sintetica: ciclo de 4 vertices, 5 arestas", processo.stdout)
        self.assertIn("custo minimo numerico 4", processo.stdout)
        self.assertIn("Dijkstra (Issue #33): indisponivel", processo.stdout)


if __name__ == "__main__":
    unittest.main(argv=["test_cli.py"])
