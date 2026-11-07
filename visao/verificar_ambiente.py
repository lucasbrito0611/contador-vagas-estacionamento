"""Confere se o ambiente da parte de visão computacional está pronto.

Uso (com o ambiente virtual ativado, de dentro da pasta visao/):
    python verificar_ambiente.py
"""

import importlib
import os
import platform
import shutil
from pathlib import Path

PASTA = Path(__file__).resolve().parent
FEEDS_ESPERADOS = ["estacionamento.ultima-placa", "estacionamento.cancela"]
TESSERACT_WINDOWS = Path(r"C:\Program Files\Tesseract-OCR\tesseract.exe")
TESSERACT_CANDIDATOS = (
    TESSERACT_WINDOWS,
    Path(r"C:\Program Files (x86)\Tesseract-OCR\tesseract.exe"),
    Path(os.environ.get("LOCALAPPDATA", "")) / "Programs" / "Tesseract-OCR" / "tesseract.exe",
)

erros = 0
avisos = 0


def ok(msg):
    print(f"  [OK]    {msg}")


def falha(msg, dica):
    global erros
    erros += 1
    print(f"  [ERRO]  {msg}\n          -> {dica}")


def aviso(msg, dica=""):
    global avisos
    avisos += 1
    print(f"  [AVISO] {msg}" + (f"\n          -> {dica}" if dica else ""))


def titulo(texto):
    print(f"\n== {texto} ==")


# 1. Python -------------------------------------------------------------------
titulo("Python")
versao = platform.python_version_tuple()
if (int(versao[0]), int(versao[1])) >= (3, 10):
    ok(f"Python {platform.python_version()}")
else:
    falha(f"Python {platform.python_version()} é antigo", "instale o Python 3.10 ou mais recente")

if os.environ.get("VIRTUAL_ENV"):
    ok("ambiente virtual ativado")
else:
    aviso("ambiente virtual não parece estar ativado", "veja o passo 2 do README.md")

# 2. Bibliotecas ----------------------------------------------------------------
titulo("Bibliotecas Python")
bibliotecas = {
    "cv2": "opencv-python",
    "ultralytics": "ultralytics",
    "pytesseract": "pytesseract",
    "Adafruit_IO": "adafruit-io",
    "dotenv": "python-dotenv",
}
importadas = {}
for modulo, pacote in bibliotecas.items():
    try:
        importadas[modulo] = importlib.import_module(modulo)
        ok(pacote)
    except ImportError:
        falha(f"{pacote} não instalado", "rode: pip install -r requirements.txt")

# 3. Programa Tesseract ---------------------------------------------------------
titulo("Programa Tesseract")
pytesseract = importadas.get("pytesseract")
if pytesseract is None:
    aviso("pulado (pytesseract não instalado)")
else:
    caminho_tesseract = shutil.which("tesseract") or next(
        (str(caminho) for caminho in TESSERACT_CANDIDATOS if caminho.is_file()),
        None,
    )
    if not shutil.which("tesseract") and caminho_tesseract:
        # Instalado no Windows, mas fora do PATH: dá para apontar direto no código.
        pytesseract.pytesseract.tesseract_cmd = caminho_tesseract
        aviso("Tesseract encontrado, mas fora do PATH",
              f"adicione ao PATH (README, passo 4) ou use o caminho encontrado: {caminho_tesseract}")
    try:
        ok(f"Tesseract {pytesseract.get_tesseract_version()}")
        cv2 = importadas.get("cv2")
        if cv2 is not None:
            import numpy as np

            img = np.full((80, 360), 255, dtype=np.uint8)
            cv2.putText(img, "ABC1D23", (15, 58), cv2.FONT_HERSHEY_SIMPLEX, 1.8, 0, 4)
            config = "--psm 7 -c tessedit_char_whitelist=ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
            lido = pytesseract.image_to_string(img, config=config).strip()
            if lido == "ABC1D23":
                ok("teste de leitura: leu 'ABC1D23' corretamente")
            else:
                aviso(f"teste de leitura: esperado 'ABC1D23', leu '{lido}'",
                      "o Tesseract funciona; só errou numa imagem sintética, não é problema")
    except Exception:
        falha("programa Tesseract não encontrado", "instale o Tesseract (README, passo 4)")

# 4. Modelo YOLO e vídeo -------------------------------------------------------
titulo("Arquivos")
modelos = sorted((PASTA / "modelos").glob("*.pt"))
if not modelos:
    falha("nenhum modelo .pt em visao/modelos/", "baixe um modelo de placas (README, passo 5)")
else:
    ok(f"modelo encontrado: {modelos[0].name}")
    if "ultralytics" in importadas:
        try:
            from ultralytics import YOLO

            modelo = YOLO(str(modelos[0]))
            ok(f"modelo carregou; classes: {list(modelo.names.values())}")
        except Exception as e:
            falha(f"o modelo não carregou ({e.__class__.__name__}: {e})",
                  "o arquivo pode estar corrompido ou não ser um modelo YOLO; baixe de novo")

videos = [p for ext in ("*.mp4", "*.avi", "*.mov", "*.mkv") for p in (PASTA / "videos").glob(ext)]
if not videos:
    falha("nenhum vídeo em visao/videos/", "coloque o vídeo de teste lá (README, passo 6)")
else:
    ok(f"vídeo encontrado: {videos[0].name}")
    cv2 = importadas.get("cv2")
    if cv2 is not None:
        cap = cv2.VideoCapture(str(videos[0]))
        lido, _ = cap.read()
        if lido:
            ok(f"vídeo abre no OpenCV ({cap.get(cv2.CAP_PROP_FPS):.0f} fps, "
               f"{int(cap.get(cv2.CAP_PROP_FRAME_COUNT))} frames)")
        else:
            falha("o OpenCV não conseguiu abrir o vídeo", "tente converter o vídeo para .mp4 (H.264)")
        cap.release()

# 5. Adafruit IO ----------------------------------------------------------------
titulo("Adafruit IO")
arquivo_env = PASTA / ".env"
if not arquivo_env.exists():
    falha("arquivo visao/.env não existe", "copie o .env.example para .env e preencha (README, passo 7)")
elif "dotenv" in importadas:
    importadas["dotenv"].load_dotenv(arquivo_env)
    usuario = os.getenv("ADAFRUIT_IO_USERNAME", "")
    chave = os.getenv("ADAFRUIT_IO_KEY", "")
    if not usuario or not chave or usuario == "seu_usuario_aqui" or chave == "sua_chave_aqui":
        falha("usuário ou chave não preenchidos no .env", "preencha com os dados de io.adafruit.com")
    elif "Adafruit_IO" in importadas:
        try:
            cliente = importadas["Adafruit_IO"].Client(usuario, chave)
            chaves = {f.key for f in cliente.feeds()}
            ok(f"conectou como '{usuario}' ({len(chaves)} feeds na conta)")
            for feed in FEEDS_ESPERADOS:
                if feed in chaves:
                    ok(f"feed {feed} existe")
                else:
                    aviso(f"feed {feed} ainda não existe", "a Pessoa A cria esse feed no site do Adafruit")
        except Exception as e:
            falha(f"não conectou ao Adafruit ({e.__class__.__name__})",
                  "confira usuário e chave no .env e a conexão com a internet")

# Resumo ------------------------------------------------------------------------
print("\n" + "=" * 50)
if erros == 0:
    print(f"Tudo pronto! ({avisos} aviso(s))")
else:
    print(f"{erros} erro(s) e {avisos} aviso(s). Corrija os erros e rode de novo.")
