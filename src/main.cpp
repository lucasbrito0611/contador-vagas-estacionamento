#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C display(0x27, 16, 2);

#define numVagasComuns 2
#define numVagasIdoso 1
#define numVagasPcd 1

// Pinos TRIG (disparo do pulso sonoro)
int trig_vagas[] = {12, 27, 5, 16};

// Pinos ECHO (recepção do pulso de retorno)
int echo_vagas[] = {14, 26, 17, 4};

byte simboloLivre[8] = {
    B00000,
    B00001,
    B00010,
    B10100,
    B01000,
    B00000,
    B00000,
    B00000};

byte simboloOcupada[8] = {
    B10001,
    B01010,
    B00100,
    B01010,
    B10001,
    B00000,
    B00000,
    B00000};

byte simboloPcd[8] = {
    B01000,
    B00000,
    B01100,
    B01000,
    B01110,
    B10011,
    B10010,
    B01100};

byte simboloIdoso[8] = {
    B00100,
    B00000,
    B01100,
    B01011,
    B01001,
    B01101,
    B10101,
    B10101};

void setup()
{
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

  // - TRIG como SAÍDA (o ESP32 envia o sinal)
  // - ECHO como ENTRADA (o ESP32 lê o retorno do sinal)
  for (int i = 0; i < 4; i++)
  {
    pinMode(trig_vagas[i], OUTPUT);
    pinMode(echo_vagas[i], INPUT);
  }
}

void loop()
{
  int vagasComunsOcupadas = 0;
  int vagasIdosoOcupadas = 0;
  int vagasPcdOcupadas = 0;

  // Percorre cada uma das 4 vagas sequencialmente
  for (int i = 0; i < 4; i++)
  {
    // Gera um pulso ultrassônico limpo de 10 microssegundos no pino TRIG
    digitalWrite(trig_vagas[i], LOW);
    delayMicroseconds(2);
    digitalWrite(trig_vagas[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_vagas[i], LOW);

    // Mede quanto tempo (em microssegundos) o pino ECHO levou para receber o eco
    long duration = pulseIn(echo_vagas[i], HIGH);

    float distance = duration * 0.034 / 2; // Converte o tempo em distância (cm)

    // Verifica se a vaga está ocupada ou livre
    if (distance < 336)
    {
      if (i == 2)
      {
        vagasIdosoOcupadas++;
      }
      else if (i == 3)
      {
        vagasPcdOcupadas++;
      }
      else
      {
        vagasComunsOcupadas++;
      }
    }

    // Intervalo de segurança entre a leitura de uma vaga e outra para evitar interferência acústica
    delay(500);
  }

  int vagasLivres = numVagasComuns - vagasComunsOcupadas;
  int vagasIdosoLivres = numVagasIdoso - vagasIdosoOcupadas;
  int vagasPcdLivres = numVagasPcd - vagasPcdOcupadas;
  int vagasOcupadas = vagasComunsOcupadas + vagasIdosoOcupadas + vagasPcdOcupadas;

  display.setCursor(4, 0);
  if (vagasLivres < 10)
    display.print(' ');
  display.print(vagasLivres);

  display.setCursor(12, 0);
  if (vagasOcupadas < 10)
    display.print(' ');
  display.print(vagasOcupadas);

  display.setCursor(4, 1);
  if (vagasPcdLivres < 10)
    display.print(' ');
  display.print(vagasPcdLivres);

  display.setCursor(12, 1);
  if (vagasIdosoLivres < 10)
    display.print(' ');
  display.print(vagasIdosoLivres);
}