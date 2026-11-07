import unittest
from unittest.mock import patch

import numpy as np

try:
    from . import leitura
except ImportError:
    import leitura

ler_placa = leitura.ler_placa
preprocessar = leitura.preprocessar


class LeituraPlacaTests(unittest.TestCase):
    def test_preprocessar_amplia_e_binariza(self):
        recorte = np.full((10, 20, 3), 255, dtype=np.uint8)
        resultado = preprocessar(recorte)
        self.assertEqual(resultado.shape, (40, 95))
        self.assertEqual(set(np.unique(resultado)), {255})

    @patch.object(leitura.pytesseract, "image_to_string", return_value="qgu6j73\n")
    def test_ler_placa_mercosul(self, _ocr):
        resultado = ler_placa(np.zeros((20, 60), dtype=np.uint8))
        self.assertEqual(resultado, "QGU6J73")

    @patch.object(leitura.pytesseract, "image_to_string", return_value="abc1234\n")
    def test_ler_placa_antiga(self, _ocr):
        resultado = ler_placa(np.zeros((20, 60), dtype=np.uint8))
        self.assertEqual(resultado, "ABC1234")

    @patch.object(leitura.pytesseract, "image_to_string", return_value="invalida\n")
    def test_ler_placa_rejeita_formato_invalido(self, _ocr):
        resultado = ler_placa(np.zeros((20, 60), dtype=np.uint8))
        self.assertIsNone(resultado)


if __name__ == "__main__":
    unittest.main()
