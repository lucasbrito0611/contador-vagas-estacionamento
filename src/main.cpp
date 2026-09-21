#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

LiquidCrystal_I2C display(0x27, 16, 2);

#define numVagasComuns 2
#define numVagasIdoso 1
#define numVagasPcd 1

#define COR_VERMELHA 0
#define COR_VERDE 1
#define COR_AZUL 2
#define COR_AMARELA 3

/************************* WiFi Access Point ***************************/
#define WLAN_SSID "Wokwi-GUEST"
#define WLAN_PASS ""
/************************* Adafruit.io Setup ***************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "lucasbrito06"
#define AIO_KEY         "aio_kxtE20zVYDHkvSGJoMGaJlJMckoU"

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup feeds
Adafruit_MQTT_Publish vagasComuns = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/vagas-comuns-livres");
Adafruit_MQTT_Publish vagasIdoso = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/vagas-idoso-livres");
Adafruit_MQTT_Publish vagasPcd = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/vagas-pcd-livres");
Adafruit_MQTT_Publish vagasOcupadas  = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/total-vagas-ocupadas");

const int ledsVagas[4][3] = {
  {13, 12, 14},
  {27, 26, 25},
  {23, 19, 18},
  {16, 4, 15}
};

int trig_vagas[] = {33, 32, 5, 17};
int echo_vagas[] = {35, 34, 36, 39};

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
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
  // Avoid trying MQTT if WiFi is down
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected — skipping MQTT connect");
    return;
  }

  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 10 seconds...");
    mqtt.disconnect();
    delay(10000);  // wait 10 seconds
    retries--;
    if (retries == 0) {
      Serial.println("MQTT connection failed after retries — will try again later");
      return;
    }
  }
  Serial.println("MQTT Connected!");
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

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  Serial.print("Connecting to WiFi");
  unsigned long wifiStart = millis();
  const unsigned long wifiTimeout = 15000; // 15s
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart) < wifiTimeout) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(); Serial.println("WiFi connected");
    Serial.print("IP address: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println(); Serial.println("WiFi connection timed out");
  }
}

unsigned long lastTime = 0;

void loop() {
  // Only attempt MQTT connect if WiFi is up
  if (WiFi.status() == WL_CONNECTED) {
    MQTT_connect();
    mqtt.processPackets(10);
  } else {
    static unsigned long lastWifiMsg = 0;
    if (millis() - lastWifiMsg > 5000) {
      lastWifiMsg = millis();
      Serial.println("WiFi not connected — skipping MQTT");
    }
  }

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
  
  int totalOcupadas = vagasComunsOcupadas + vagasIdosoOcupadas + vagasPcdOcupadas;

  // Atualização do Display LCD
  display.setCursor(4, 0);
  if (vagasLivres < 10) {
    display.print(' ');
  }
  display.print(vagasLivres);

  display.setCursor(12, 0);
  if (totalOcupadas < 10) {
    display.print(' ');
  }
  display.print(totalOcupadas);

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

  // Trava de tempo de 10 segundos e publicação dos dados
  unsigned long now = millis();
  if (now - lastTime > 10000) {
    lastTime = now;
    
    // Publish only if MQTT connected
    if (mqtt.connected()) {
      bool p1 = vagasComuns.publish(vagasLivres);
      bool p2 = vagasIdoso.publish(vagasIdosoLivres);
      bool p3 = vagasPcd.publish(vagasPcdLivres);
      bool p4 = vagasOcupadas.publish(totalOcupadas);
      Serial.print("MQTT publish results: ");
      Serial.print(p1); Serial.print(","); Serial.print(p2); Serial.print(","); Serial.print(p3); Serial.print(","); Serial.println(p4);
      Serial.println("Dados atualizados enviados via MQTT!");
    } else {
      Serial.println("MQTT not connected — skipping publish");
    }
  }
}