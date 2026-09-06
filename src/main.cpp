#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C display(0x27, 16, 2);

#define numVagasComuns 2
#define numVagasIdoso 1
#define numVagasPcd 1

#define COR_VERMELHA 0
#define COR_VERDE 1
#define COR_AZUL 2
#define COR_AMARELA 3

const int ledsVagas[4][3] = {
  {13, 25, 33},
  {32, 26, 14},
  {23, 19, 18},
  {17, 4, 2}
};

int trig_vagas[] = {12, 27, 5, 16};
int echo_vagas[] = {39, 36, 34, 35};

byte simboloLivre[8] = {
  B00000, B00001, B00010, B10100, B01000, B00000, B00000, B00000
};

byte simboloOcupada[8] = {
  B10001, B01010, B00100, B01010, B10001, B00000, B00000, B00000
};

byte simboloPcd[8] = {
  B01000, B00000, B01100, B01000, B01110, B10011, B10010, B01100
};

byte simboloIdoso[8] = {
  B00100, B00000, B01100, B01011, B01001, B01101, B10101, B10101
};

void acenderLed(int vaga, int cor) {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledsVagas[vaga][i], LOW);
  }

  if (cor == COR_AMARELA) {
    digitalWrite(ledsVagas[vaga][0], HIGH);
    digitalWrite(ledsVagas[vaga][1], HIGH);
  } else {
    for (int i = 0; i < 3; i++) {
      if (i == cor) {
        digitalWrite(ledsVagas[vaga][i], HIGH);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  display.init();
  display.backlight();
  display.createChar(0, simboloLivre);
  display.createChar(1, simboloOcupada);
  display.createChar(2, simboloIdoso);
  display.createChar(3, simboloPcd);

  display.setCursor(0, 0);
  display.print("  ");
  display.write(byte(0));
  display.print("    || ");
  display.write(byte(1));
  display.print("   ");
  
  display.setCursor(0, 1);
  display.print("  ");
  display.write(byte(3));
  display.print("    || ");
  display.write(byte(2));
  display.print("   ");

  for (int i = 0; i < 4; i++) {
    pinMode(trig_vagas[i], OUTPUT);
    pinMode(echo_vagas[i], INPUT);

    for (int canal = 0; canal < 3; canal++) {
      pinMode(ledsVagas[i][canal], OUTPUT);
      digitalWrite(ledsVagas[i][canal], LOW);
    }
  }
}

void loop() {
  int vagasComunsOcupadas = 0;
  int vagasIdosoOcupadas = 0;
  int vagasPcdOcupadas = 0;

  for (int i = 0; i < 4; i++) {
    digitalWrite(trig_vagas[i], LOW);
    delayMicroseconds(2);
    digitalWrite(trig_vagas[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_vagas[i], LOW);

    long duration = pulseIn(echo_vagas[i], HIGH, 30000);

    float distance = duration == 0 ? 999 : duration * 0.034 / 2;

    bool ocupada = distance < 336;

    if (ocupada) {
      acenderLed(i, COR_VERMELHA);

      if (i == 0) {
        vagasPcdOcupadas++;
      } else if (i == 3) {
        vagasIdosoOcupadas++;
      } else {
        vagasComunsOcupadas++;
      }
    } else {
      if (i == 0) {
        acenderLed(i, COR_AZUL);
      } else if (i == 3) {
        acenderLed(i, COR_AMARELA);
      } else {
        acenderLed(i, COR_VERDE);
      }
    }

    delay(50);
  }

  int vagasLivres = numVagasComuns - vagasComunsOcupadas;
  int vagasIdosoLivres = numVagasIdoso - vagasIdosoOcupadas;
  int vagasPcdLivres = numVagasPcd - vagasPcdOcupadas;
  int vagasOcupadas = vagasComunsOcupadas + vagasIdosoOcupadas + vagasPcdOcupadas;

  display.setCursor(4, 0);
  if (vagasLivres < 10) {
    display.print(' ');
  }
  display.print(vagasLivres);

  display.setCursor(12, 0);
  if (vagasOcupadas < 10) {
    display.print(' ');
  }
  display.print(vagasOcupadas);

  display.setCursor(4, 1);
  if (vagasPcdLivres < 10) {
    display.print(' ');
  }
  display.print(vagasPcdLivres);

  display.setCursor(12, 1);
  if (vagasIdosoLivres < 10) {
    display.print(' ');
  }
  display.print(vagasIdosoLivres);
}