# Programa principal da leitura de placas

import time
from collections import Counter
from pathlib import Path

import cv2

from deteccao import detectar_placas
from mqtt import publicar_placa

# Leitura da Pessoa C. Enquanto o leitura.py não existir, usa uma leitura falsa.
try:
    from leitura import ler_placa, preprocessar
    LEITURA_FALSA = False
except ImportError:
    LEITURA_FALSA = True

    def preprocessar(recorte):
        return recorte

    def ler_placa(imagem):
        return "ABC1D23"

# --- Configurações -----------------------------------------------------------
PASTA = Path(__file__).resolve().parent
VIDEO = PASTA / "videos" / "carro.mp4"

PULAR_FRAMES = 3          # processa 1 a cada 3 frames
LARGURA_MINIMA = 80       # px; placa menor que isso é pequena demais para o OCR
VOTOS_NECESSARIOS = 5     # leituras iguais para confirmar a placa
TEMPO_REPUBLICAR = 30     # s sem publicar a mesma placa de novo
FRAMES_SEM_PLACA = 60     # frames sem detecção = carro foi embora, zera os votos

SALVAR_RECORTES = False   # True salva os recortes em runs/recortes/ (para a Pessoa C)
PASTA_RECORTES = PASTA / "runs" / "recortes"


def maior_placa(placas):
    """Se o YOLO achar mais de uma placa, fica com a maior (o carro mais perto)."""
    return max(placas, key=lambda p: p[2] - p[0])


def recortar(frame, x1, y1, x2, y2):
    altura, largura = frame.shape[:2]
    return frame[max(0, y1):min(altura, y2), max(0, x1):min(largura, x2)]


def main():
    video = cv2.VideoCapture(str(VIDEO))
    if not video.isOpened():
        raise FileNotFoundError(f"Não consegui abrir o vídeo {VIDEO}")

    if LEITURA_FALSA:
        print("AVISO: leitura.py não encontrado; usando leitura falsa (sempre 'ABC1D23').")
    if SALVAR_RECORTES:
        PASTA_RECORTES.mkdir(parents=True, exist_ok=True)

    votos = Counter()            # placa lida -> quantas vezes
    ultima_publicacao = {}       # placa -> horário (time.time()) da última publicação
    ultimo_frame_com_placa = 0
    numero_frame = 0
    status = "Procurando placa..."

    while True:
        lido, frame = video.read()
        if not lido:
            break
        numero_frame += 1

        if numero_frame % PULAR_FRAMES == 0:
            placas = detectar_placas(frame)

            if placas:
                ultimo_frame_com_placa = numero_frame
                x1, y1, x2, y2, confianca = maior_placa(placas)
                grande = (x2 - x1) >= LARGURA_MINIMA
                cor = (0, 255, 0) if grande else (0, 200, 255)  # verde = vai pro OCR; laranja = pequena
                cv2.rectangle(frame, (x1, y1), (x2, y2), cor, 2)

                if grande:
                    recorte = recortar(frame, x1, y1, x2, y2)
                    if SALVAR_RECORTES:
                        cv2.imwrite(str(PASTA_RECORTES / f"frame_{numero_frame}.png"), recorte)

                    texto = ler_placa(preprocessar(recorte))
                    if texto:
                        votos[texto] += 1
                        cv2.putText(frame, texto, (x1, y1 - 8),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, cor, 2)

                        placa, quantidade = votos.most_common(1)[0]
                        if time.time() - ultima_publicacao.get(placa, 0) < TEMPO_REPUBLICAR:
                            status = f"Placa {placa} enviada!"  # já enviada há pouco; não repete
                            votos.clear()
                        elif quantidade >= VOTOS_NECESSARIOS:
                            try:
                                publicar_placa(placa)
                                ultima_publicacao[placa] = time.time()
                                status = f"Placa {placa} enviada!"
                                print(f"frame {numero_frame}: placa {placa} confirmada e enviada "
                                      f"(votos: {dict(votos)})")
                            except Exception as erro:
                                status = f"Erro ao enviar {placa}"
                                print(f"frame {numero_frame}: erro ao publicar {placa}: {erro}")
                            votos.clear()
                        else:
                            status = f"Lendo: {placa} ({quantidade}/{VOTOS_NECESSARIOS} votos)"

            elif numero_frame - ultimo_frame_com_placa > FRAMES_SEM_PLACA and votos:
                votos.clear()  # carro saiu da imagem; o próximo carro começa do zero
                status = "Procurando placa..."

        cv2.putText(frame, status, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        cv2.imshow("Leitura de placas (q para sair)", frame)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    video.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()