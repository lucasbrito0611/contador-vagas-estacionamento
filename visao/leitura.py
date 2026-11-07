"""Pre-processamento e leitura OCR de placas brasileiras."""

from __future__ import annotations

import re
import os
from pathlib import Path
from typing import Optional

import cv2
import pytesseract


def _configurar_tesseract() -> None:
    candidatos = (
        Path(r"C:\Program Files\Tesseract-OCR\tesseract.exe"),
        Path(r"C:\Program Files (x86)\Tesseract-OCR\tesseract.exe"),
        Path(os.environ.get("LOCALAPPDATA", "")) / "Programs" / "Tesseract-OCR" / "tesseract.exe",
    )
    for caminho in candidatos:
        if caminho.is_file():
            pytesseract.pytesseract.tesseract_cmd = str(caminho)
            return


_configurar_tesseract()


PLATE_PATTERNS = (
    re.compile(r"^[A-Z]{3}[0-9][A-Z][0-9]{2}$"),
    re.compile(r"^[A-Z]{3}[0-9]{4}$"),
)
OCR_WHITELIST = "ABCDEFGHJKLMNPQRSTUVWXYZ0123456789"

LETTER_FIXES = str.maketrans({"0": "O", "1": "I", "8": "B", "5": "S"})
DIGIT_FIXES = str.maketrans({"O": "0", "Q": "0", "D": "0", "I": "1", "L": "1", "Z": "2", "S": "5", "B": "8", "G": "6"})


def preprocessar(recorte):
    """Remove bordas da placa, amplia, converte para cinza e aplica Otsu."""
    if recorte is None or getattr(recorte, "size", 0) == 0:
        raise ValueError("O recorte da placa esta vazio")

    altura, largura = recorte.shape[:2]
    inicio_y = max(1, int(altura * 0.2))
    inicio_x = max(1, int(largura * 0.05))
    recorte_sem_bordas = recorte[inicio_y:, inicio_x:]
    if recorte_sem_bordas.size == 0:
        raise ValueError("O recorte ficou vazio depois da remocao das bordas")

    ampliada = cv2.resize(recorte_sem_bordas, None, fx=5, fy=5, interpolation=cv2.INTER_LANCZOS4)
    cinza = cv2.cvtColor(ampliada, cv2.COLOR_BGR2GRAY) if len(ampliada.shape) == 3 else ampliada
    _, binaria = cv2.threshold(cinza, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    return binaria


def _corrigir_posicoes(texto: str, formato: str) -> str:
    """Corrige confusoes comuns somente nas posicoes esperadas da placa."""
    caracteres = list(texto)
    if formato == "mercosul":
        posicoes_letras = (0, 1, 2, 4)
        posicoes_numeros = (3, 5, 6)
    else:
        posicoes_letras = (0, 1, 2)
        posicoes_numeros = (3, 4, 5, 6)

    for posicao in posicoes_letras:
        if posicao < len(caracteres):
            caracteres[posicao] = caracteres[posicao].translate(LETTER_FIXES)
    for posicao in posicoes_numeros:
        if posicao < len(caracteres):
            caracteres[posicao] = caracteres[posicao].translate(DIGIT_FIXES)
    return "".join(caracteres)


def _validar_candidato(texto: str) -> Optional[str]:
    if len(texto) != 7:
        return None

    for formato, padrao in (("mercosul", PLATE_PATTERNS[0]), ("antigo", PLATE_PATTERNS[1])):
        candidato = _corrigir_posicoes(texto, formato)
        if padrao.fullmatch(candidato):
            return candidato
    return None


def ler_placa(imagem) -> Optional[str]:
    """Executa OCR e retorna uma placa valida ou None."""
    if imagem is None or getattr(imagem, "size", 0) == 0:
        return None

    config = f"--psm 7 -c tessedit_char_whitelist={OCR_WHITELIST}"
    bruto = pytesseract.image_to_string(imagem, config=config)
    texto = re.sub(r"[^A-Z0-9]", "", bruto.upper())
    return _validar_candidato(texto)


def ler_arquivo(caminho: Path) -> Optional[str]:
    recorte = cv2.imread(str(caminho))
    if recorte is None:
        raise ValueError(f"Nao foi possivel abrir a imagem: {caminho}")
    return ler_placa(preprocessar(recorte))


def executar_testes(pasta: Path) -> None:
    imagens = sorted(pasta.glob("*.png")) + sorted(pasta.glob("*.jpg")) + sorted(pasta.glob("*.jpeg"))
    if not imagens:
        print(f"Nenhuma imagem encontrada em {pasta}")
        return
    for imagem in imagens:
        print(f"{imagem.name}: {ler_arquivo(imagem) or 'nenhuma placa valida'}")


if __name__ == "__main__":
    executar_testes(Path(__file__).parent / "testes")
