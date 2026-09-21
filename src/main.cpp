#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
  
#include <WiFi.h>

#define WLAN_SSID       "Wokwi-GUEST"
#define WLAN_PASS       ""

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883  // use 8883 for SSL
#define AIO_USERNAME    "ViniciusPi"
#define AIO_KEY         "aio_DmPw48ShZ0kHR0OsY6XIGv52GaYW"

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish vagasPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/estacionamento.vagas-status");
Adafruit_MQTT_Subscribe vagasSub = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/estacionamento.vagas-comandos");

LiquidCrystal_I2C display(0x27, 16, 2);

#define numVagasComuns 2
#define numVagasIdoso 1
#define numVagasPcd 1
#define NUM_VAGAS 4

#define COR_VERMELHA 0
#define COR_VERDE 1
#define COR_AZUL 2
#define COR_AMARELA 3

unsigned long lastMsg = 0;
int vagasStatus[NUM_VAGAS][2] = {{0, 0}, {1, 0}, {2, 0}, {3, 0}};

const int ledsVagas[NUM_VAGAS][3] = {
  {13, 12, 14},
  {27, 26, 25},
  {23, 19, 18},
  {16, 4, 15}
};

int trig_vagas[] = {33, 32, 5, 17};
int echo_vagas[] = {35, 34, 36, 39};
bool vagaOcupada[NUM_VAGAS] = {false, false, false, false};

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
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

void vagas_callback(char *data, uint16_t len) {
  data[len] = '\0';

  int idVaga;
  int ocupado;

  sscanf(data, "%d,%d", &idVaga, &ocupado);

  if (idVaga >= 0 && idVaga < NUM_VAGAS) {
    vagaOcupada[idVaga] = (ocupado == 1);
    Serial.print("Vaga ");
    Serial.print(idVaga);
    Serial.print(" -> ");
    Serial.println(vagaOcupada[idVaga] ? "Ocupada" : "Livre");
  }
}

void publicarVagasStatus() {
  int vagasLivres = 0;

  for (int i = 0; i < NUM_VAGAS; i++) {
    if (!vagaOcupada[i]) {
      vagasLivres++;  
    }
  }

  String mqtt_msg = String(vagasLivres);
  if (!vagasPub.publish(mqtt_msg.c_str())) {
    Serial.println("Falha ao publicar");
  } else {
    Serial.println("Publicado com sucesso");
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

  vagasSub.setCallback(vagas_callback);
  mqtt.subscribe(&vagasSub);

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
    MQTT_connect();
    mqtt.processPackets(10000);
    if (!mqtt.ping()) {
      mqtt.disconnect();
    }

    unsigned long now = millis();
    if (now - lastMsg > 10000 || now == 0) {
      lastMsg = now;
      publicarVagasStatus();
    }
  } else {
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
    vagaOcupada[i] = ocupada;
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