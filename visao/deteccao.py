# Detecção de placas com YOLO.
from pathlib import Path

import cv2
from ultralytics import YOLO

PASTA = Path(__file__).resolve().parent
CAMINHO_MODELO = PASTA / "modelos" / "best.pt"
CONFIANCA_MINIMA = 0.5   # descarta detecções fracas (ex.: a de 0,44 com o carro longe)

if not CAMINHO_MODELO.exists():
    raise FileNotFoundError(f"Modelo não encontrado em {CAMINHO_MODELO}. Veja o passo 5 do README.")

modelo = YOLO(str(CAMINHO_MODELO))


def detectar_placas(frame):
    """Procura placas num frame do vídeo.

    Recebe: um frame do OpenCV (imagem BGR).
    Devolve: lista de tuplas (x1, y1, x2, y2, confianca), uma por placa.
             (x1, y1) é o canto superior esquerdo e (x2, y2) o inferior direito.
             Lista vazia se não achar nenhuma placa.
    """
    resultado = modelo(frame, conf=CONFIANCA_MINIMA, verbose=False)[0]

    placas = []
    for caixa in resultado.boxes:
        x1, y1, x2, y2 = map(int, caixa.xyxy[0])
        confianca = float(caixa.conf[0])
        placas.append((x1, y1, x2, y2, confianca))
    return placas


# --- Teste: só roda com "python deteccao.py", não quando o main.py importa ----
if __name__ == "__main__":
    video = cv2.VideoCapture(str(PASTA / "videos" / "carro.mp4"))
    numero_frame = 0

    while True:
        lido, frame = video.read()
        if not lido:
            break  # fim do vídeo
        numero_frame += 1

        for x1, y1, x2, y2, confianca in detectar_placas(frame):
            largura = x2 - x1
            print(f"frame {numero_frame}: placa em ({x1},{y1})-({x2},{y2}), "
                  f"largura {largura}px, confiança {confianca:.2f}")
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            cv2.putText(frame, f"{confianca:.2f}", (x1, y1 - 8),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

        cv2.imshow("Deteccao de placas (q para sair)", frame)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    video.release()
    cv2.destroyAllWindows()