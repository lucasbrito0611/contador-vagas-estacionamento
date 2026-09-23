#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
  
#include <WiFi.h>

#define WLAN_SSID       "Wokwi-GUEST"
#define WLAN_PASS       ""

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883  // use 8883 for SSL
#define AIO_USERNAME    "" //colocar aqui o nome de usuario do adafruit
#define AIO_KEY         "" //colocar aqui a chave do adafruit

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish vagasPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/estacionamento.vagas-status");

Adafruit_MQTT_Subscribe vagaIdSub = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/estacionamento.vagas-id");
Adafruit_MQTT_Subscribe vagaOnOffSub = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/estacionamento.vagas-onoff");

Adafruit_MQTT_Publish vaga1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/estacionamento.vaga-1");
Adafruit_MQTT_Publish vaga2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/estacionamento.vaga-2");
Adafruit_MQTT_Publish vaga3 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/estacionamento.vaga-3");
Adafruit_MQTT_Publish vaga4 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/estacionamento.vaga-4");

LiquidCrystal_I2C display(0x27, 16, 2);

#define numVagasComuns 2
#define numVagasIdoso 1
#define numVagasPcd 1
#define NUM_VAGAS 4

#define COR_VERMELHA 0
#define COR_VERDE 1
#define COR_AZUL 2
#define COR_AMARELA 3

int idVagaRecebido = -1;

int contadorPublicado = -1;
int statusPublicado[NUM_VAGAS] = {-1, -1, -1, -1};

const int ledsVagas[NUM_VAGAS][3] = {
  {13, 12, 14},
  {27, 26, 25},
  {23, 19, 18},
  {16, 4, 15}
};

int trig_vagas[] = {33, 32, 5, 17};
int echo_vagas[] = {35, 34, 36, 39};
bool vagaOcupada[NUM_VAGAS] = {false, false, false, false};
bool vagaBloqueada[NUM_VAGAS] = {false, false, false, false};

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

void MQTT_connect() {
  int8_t ret;

  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);
    retries--;
    if (retries == 0) {
      Serial.println("MQTT unavailable; trying again later");
      return;
    }
  }
  Serial.println("MQTT Connected!");
}

bool publicarVagaStatus(int vaga) {
  if (vaga < 0 || vaga >= NUM_VAGAS) {
    return false;
  }

  int statusAtual = vagaOcupada[vaga] ? 1 : 0;

  if (statusPublicado[vaga] == statusAtual) return true;

  bool sucesso;
  switch (vaga) {
    case 0:
      sucesso = vaga1.publish(statusAtual ? "1" : "0");
      break;
    case 1:
      sucesso = vaga2.publish(statusAtual ? "1" : "0");
      break;
    case 2:
      sucesso = vaga3.publish(statusAtual ? "1" : "0");
      break;
    default:
      sucesso = vaga4.publish(statusAtual ? "1" : "0");
      break;
  }

  if (sucesso) statusPublicado[vaga] = statusAtual;
  return sucesso;
}

void vagaIdCallback(char *data, uint16_t len) {
  data[len] = '\0';
  int idVaga = atoi(data) - 1;

  if (idVaga >= 0 && idVaga < NUM_VAGAS) {
    idVagaRecebido = idVaga;
  }
}

void vagaOnOffCallback(char *data, uint16_t len) {
  data[len] = '\0';
  int manual = atoi(data);

  if (idVagaRecebido >= 0 && idVagaRecebido < NUM_VAGAS) {
    if (vagaBloqueada[idVagaRecebido] != (manual == 1)) {
      vagaOcupada[idVagaRecebido] = (manual == 1);
      vagaBloqueada[idVagaRecebido] = vagaOcupada[idVagaRecebido];
    
      Serial.print("Vaga ");
      Serial.print(idVagaRecebido + 1);
      Serial.print(" -> ");
      Serial.println(vagaBloqueada[idVagaRecebido] ? "Manual" : "Automática");
    }
  }
}

void publicarAlteracoes(int vagasLivres) {
  for (int i = 0; i < NUM_VAGAS; i++) {
    publicarVagaStatus(i);
  }

  if (contadorPublicado != vagasLivres) {
    if (vagasPub.publish(vagasLivres)) {
      contadorPublicado = vagasLivres;
      Serial.print("Status atualizado, vaga(s) livre(s): ");
      Serial.println(vagasLivres);
    } else {
      Serial.println("Falha ao publicar status das vagas");
    }
  }
}

void conectarWifi() {
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected");
  } else {
    Serial.println();
    Serial.println("WiFi timeout. Continuing without network");
  }
}

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

  conectarWifi();

  vagaIdSub.setCallback(vagaIdCallback);
  vagaOnOffSub.setCallback(vagaOnOffCallback);
  mqtt.subscribe(&vagaIdSub);
  mqtt.subscribe(&vagaOnOffSub);

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

  for (int i = 0; i < NUM_VAGAS; i++) {
    pinMode(trig_vagas[i], OUTPUT);
    pinMode(echo_vagas[i], INPUT);

    for (int canal = 0; canal < 3; canal++) {
      pinMode(ledsVagas[i][canal], OUTPUT);
      digitalWrite(ledsVagas[i][canal], LOW);
    }
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    bool estavaConectado = mqtt.connected();
    MQTT_connect();
    if (!estavaConectado && mqtt.connected()) {
      for (int i = 0; i < NUM_VAGAS; i++) {
        statusPublicado[i] = -1;
      }
      contadorPublicado = -1;
    }

    if (mqtt.connected()) {
      mqtt.processPackets(1000);
      if (!mqtt.ping()) {
        mqtt.disconnect();
        for (int i = 0; i < NUM_VAGAS; i++) {
          statusPublicado[i] = -1;
        }
        contadorPublicado = -1;
      }
    }
  } else {
    for (int i = 0; i < NUM_VAGAS; i++) {
      statusPublicado[i] = -1;
    }
    contadorPublicado = -1;
    Serial.println("WiFi offline; waiting for connection...");
    delay(1000);
  }

  int vagasComunsOcupadas = 0;
  int vagasIdosoOcupadas = 0;
  int vagasPcdOcupadas = 0;

  for (int i = 0; i < NUM_VAGAS; i++) {
    digitalWrite(trig_vagas[i], LOW);
    delayMicroseconds(2);
    digitalWrite(trig_vagas[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_vagas[i], LOW);

    long duration = pulseIn(echo_vagas[i], HIGH, 30000);

    float distance = duration == 0 ? 999 : duration * 0.034 / 2;

    bool ocupada = distance < 336;
    if (!vagaBloqueada[i]) {
      vagaOcupada[i] = ocupada;
    }
    ocupada = vagaOcupada[i];

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

  int vagasComunsLivres = numVagasComuns - vagasComunsOcupadas;
  int vagasIdosoLivres = numVagasIdoso - vagasIdosoOcupadas;
  int vagasPcdLivres = numVagasPcd - vagasPcdOcupadas;
  int vagasOcupadas = vagasComunsOcupadas + vagasIdosoOcupadas + vagasPcdOcupadas;
  int vagasLivres = vagasComunsLivres + vagasIdosoLivres + vagasPcdLivres;

  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
    publicarAlteracoes(vagasLivres);
  }

  display.setCursor(4, 0);
  if (vagasComunsLivres < 10) {
    display.print(' ');
  }
  display.print(vagasComunsLivres);

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