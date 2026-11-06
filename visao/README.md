# Visão computacional: leitura de placas

Esta pasta tem a parte da Unidade 3: ler a placa do carro num vídeo e enviar ao Adafruit IO, para o ESP32 abrir a cancela.

Este README é o **roteiro de instalação**. Cada integrante segue no próprio PC. No fim, o script `verificar_ambiente.py` confere se ficou tudo certo.

```
visao/
├── README.md               ← este roteiro
├── requirements.txt        ← bibliotecas Python
├── .env.example            ← modelo do arquivo com usuário e chave do Adafruit
├── verificar_ambiente.py   ← confere se está tudo instalado
├── modelos/                ← coloque aqui o modelo YOLO (.pt)
└── videos/                 ← coloque aqui o vídeo de teste
```

> Os modelos, os vídeos e o `.env` **não vão para o GitHub** (estão no `.gitignore`), porque são arquivos grandes ou secretos. Compartilhem entre vocês por Drive ou WhatsApp.

---

## Passo 1: Python

Precisa do **Python 3.10 ou mais recente**. Para conferir:

```bash
python --version
```

No Linux e no macOS, pode ser que o comando seja `python3`. Se for o seu caso, troque `python` por `python3` em todos os comandos abaixo.

Se não tiver o Python, baixe em https://www.python.org/downloads/. **No Windows, marque a opção "Add Python to PATH" na primeira tela do instalador.**

## Passo 2: ambiente virtual

O ambiente virtual deixa as bibliotecas do projeto separadas do resto do PC. Abra o terminal **dentro da pasta `visao/`** e rode:

**Linux / macOS**
```bash
python3 -m venv venv
source venv/bin/activate
```

**Windows (PowerShell)**
```powershell
python -m venv venv
venv\Scripts\Activate.ps1
```

> Se o PowerShell bloquear o script, rode uma vez `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` e tente de novo.

Quando ativado, o terminal mostra `(venv)` no começo da linha. **Toda vez que abrir um terminal novo para trabalhar no projeto, ative o ambiente de novo** (só a segunda linha).

## Passo 3: bibliotecas Python

Com o `(venv)` ativo:

```bash
pip install -r requirements.txt
```

Isso instala o OpenCV, o YOLO (ultralytics), o pytesseract, a biblioteca do Adafruit e o python-dotenv. **A primeira instalação demora**, porque o ultralytics baixa o PyTorch (de 1 a 2 GB).

## Passo 4: programa Tesseract

O `pytesseract` é só uma "ponte": o **programa** Tesseract precisa ser instalado à parte.

**Linux (Ubuntu/Debian)**
```bash
sudo apt update && sudo apt install -y tesseract-ocr
```

**macOS**
```bash
brew install tesseract
```

**Windows**
1. Baixe o instalador do UB Mannheim: https://github.com/UB-Mannheim/tesseract/wiki
2. Instale no caminho padrão (`C:\Program Files\Tesseract-OCR`).
3. Coloque essa pasta no PATH: menu Iniciar → "Editar as variáveis de ambiente do sistema" → Variáveis de Ambiente → em "Path", clique em Editar → Novo → `C:\Program Files\Tesseract-OCR`.
4. **Feche e abra o terminal de novo** e confira:

```powershell
tesseract --version
```

## Passo 5: modelo YOLO de placas

O professor treinou um modelo próprio com placas brasileiras, mas ele só sai na Aula 17 (Práticas 12 e 13). Até lá, usamos um modelo público treinado com **placas Mercosul**:

- **Repositório:** [samuelpdsv/Computer-Vision-MERCOSUL-ALPR](https://github.com/samuelpdsv/Computer-Vision-MERCOSUL-ALPR) (código MIT; imagens de treino [CC BY 4.0](https://universe.roboflow.com/samuels-workspace-8un26/placas-dec-3-classes-mercosul-a440m/dataset/1); citem a fonte no trabalho).
- **Arquivo:** `best.pt` (YOLOv8n, 6 MB, 1 classe: `Placa_mercosul_carro`). Download direto:
  https://github.com/samuelpdsv/Computer-Vision-MERCOSUL-ALPR/raw/main/runs/detect/Treinamento/Placas_Mercosul4/weights/best.pt

Baixe o arquivo e coloque na pasta `visao/modelos/`.

**Limitações:**
- Foi treinado **só com placas Mercosul de carro** (442 imagens). Placas antigas (cinza) ou de moto talvez não sejam detectadas, então **escolham um vídeo com placa Mercosul**.
- O autor mediu a precisão nas mesmas imagens do treino, então o desempenho real é desconhecido. **Testem no vídeo de vocês** (comando abaixo).

**Testar o modelo no vídeo** (dentro de `visao/`, com o `(venv)` ativo; troque o nome do vídeo):

```bash
yolo predict model=modelos/best.pt source=videos/SEU_VIDEO.mp4 conf=0.25 save=True
```

O vídeo com as caixas desenhadas é salvo numa pasta dentro de `runs/detect/`. Se a placa ganhar uma caixa na maior parte dos frames em que aparece de perto, o modelo serve. Se não, tentem `conf=0.1` ou outro vídeo.

Depois da Aula 17, é só colocar o modelo do professor nessa mesma pasta.

## Passo 6: vídeo de teste

Escolham um vídeo de um carro vindo em direção à câmera, com:
- **placa brasileira** (antiga ou Mercosul);
- placa **nítida e grande** em algum momento do vídeo;
- carro de frente e boa iluminação.

Salve na pasta `visao/videos/`, de preferência em `.mp4`.

## Passo 7: usuário e chave do Adafruit

Copie o arquivo de exemplo:

**Linux / macOS**
```bash
cp .env.example .env
```

**Windows (PowerShell)**
```powershell
copy .env.example .env
```

Abra o `.env` e preencha com o usuário e a chave do Adafruit IO (no site, ícone da chave amarela, "My Key"). Usem a mesma conta que o ESP já usa no `main.cpp`.

> **Nunca** coloque a chave real no `.env.example` nem em nenhum arquivo que vá para o GitHub.

## Passo 8: conferir tudo

Ainda dentro de `visao/`, com o `(venv)` ativo:

```bash
python verificar_ambiente.py
```

O script mostra `[OK]`, `[AVISO]` ou `[ERRO]` para cada item, com a dica de correção. Ele confere:
- a versão do Python e se o ambiente virtual está ativo;
- se as bibliotecas foram instaladas;
- se o programa Tesseract foi encontrado (e faz um teste rápido de leitura);
- se existe um modelo em `modelos/` e se ele carrega;
- se existe um vídeo em `videos/` e se o OpenCV consegue abri-lo;
- se o `.env` está preenchido, se a conexão com o Adafruit funciona e se os feeds `estacionamento.ultima-placa` e `estacionamento.cancela` já existem.

Os feeds só aparecem depois que a Pessoa A os criar no site do Adafruit. Até lá, eles aparecem como `[AVISO]`, o que é normal.

---

## Problemas comuns

| Mensagem | O que fazer |
|---|---|
| `python: command not found` | Use `python3`, ou reinstale o Python marcando "Add Python to PATH" (Windows). |
| `No module named 'cv2'` (ou outra biblioteca) | O ambiente virtual não está ativado. Ative (passo 2) e rode o `pip install` de novo. |
| `tesseract is not installed or it's not in your PATH` | Instale o programa Tesseract (passo 4). No Windows, confira o PATH e **reabra o terminal**. |
| `pip install` muito lento ou parado | É o download do PyTorch; espere. Se der erro de espaço, libere uns 3 GB no disco. |
| O vídeo não abre no OpenCV | Converta para `.mp4` (H.264), por exemplo com um conversor online. |
| `RequestError` ou erro 401 no Adafruit | Usuário ou chave errados no `.env`. Copie de novo do site. |
| Erro 429 no Adafruit | Limite do plano gratuito (30 envios por minuto). Espere um minuto. |
