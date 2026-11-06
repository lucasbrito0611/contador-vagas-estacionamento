import os

from Adafruit_IO import Client
from dotenv import load_dotenv

def publicar_placa(placa: str) -> None:
    load_dotenv()

    usuario = os.getenv("ADAFRUIT_IO_USERNAME")
    chave = os.getenv("ADAFRUIT_IO_KEY")
    if not usuario or not chave:
        raise RuntimeError("ADAFRUIT_IO_USERNAME e ADAFRUIT_IO_KEY precisam estar preenchidos no visao/.env")
    
    placa = placa.strip()
    if not placa:
        raise ValueError("A placa não pode estar vazia")

    cliente = Client(usuario, chave)
    cliente.send_data("estacionamento.ultima-placa", placa)