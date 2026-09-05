#include <Arduino.h>

// Pinos TRIG (disparo do pulso sonoro)
int trig_vagas[] = {12, 27, 5, 16};

// Pinos ECHO (recepção do pulso de retorno)
int echo_vagas[] = {14, 26, 17, 4};

void setup() {
  Serial.begin(115200);

  // - TRIG como SAÍDA (o ESP32 envia o sinal)
  // - ECHO como ENTRADA (o ESP32 lê o retorno do sinal)
  for (int i = 0; i < 4; i++) {
    pinMode(trig_vagas[i], OUTPUT);
    pinMode(echo_vagas[i], INPUT);
  }
}

void loop() {
  // Percorre cada uma das 4 vagas sequencialmente
  for (int i = 0; i < 4; i++) {
    // Gera um pulso ultrassônico limpo de 10 microssegundos no pino TRIG
    digitalWrite(trig_vagas[i], LOW);
    delayMicroseconds(2);
    digitalWrite(trig_vagas[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_vagas[i], LOW);

    // Mede quanto tempo (em microssegundos) o pino ECHO levou para receber o eco
    long duration = pulseIn(echo_vagas[i], HIGH);

    // A fórmula da velocidade do som no ar divide o tempo por 58 para obter cm (ou 148 para polegadas)
    Serial.print("Distância em centímetros da vaga ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(duration / 58);

    Serial.print("Distância em polegadas da vaga ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(duration / 148);

    // Intervalo de segurança entre a leitura de uma vaga e outra para evitar interferência acústica
    delay(500);
  }

  // Pausa antes de iniciar a próxima rodada completa de medições
  delay(1000);
}