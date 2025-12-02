#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// WIFI
const char* ssid = "TELUS1927";
const char* password = "platform1770";

// MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_out_topic = "parhamMesh/out";

// CLIENTS
WiFiClient espClient;
PubSubClient mqtt(espClient);

// DHT
#define DHTPIN 47
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LoRa
#define FREQUENCY        916.0
#define BANDWIDTH        125.0
#define SPREADING_FACTOR 11
#define TXPOWER          22

// NODE ID
uint32_t NODE_ID = 2;

void sendPacket(const char *payload) {
  radio.transmit(payload);
  radio.startReceive();
  mqtt.publish(mqtt_out_topic, payload);
}

void goToSleep() {
  esp_sleep_enable_timer_wakeup(3600ULL * 1000000ULL); // 1 hour
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  delay(300);

  dht.begin();
  delay(300);

  heltec_setup();

  // OLED OFF — KILL POWER
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);   // OLED fully powered off

  // LoRa init
  RADIOLIB_OR_HALT(radio.begin());
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  radio.setOutputPower(TXPOWER);
  radio.setCodingRate(5);
  radio.setPreambleLength(12);
  radio.setCRC(true);
  radio.startReceive();

  // WiFi
  WiFi.begin(ssid, password);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) {
    delay(200);
  }

  // MQTT
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.connect(("Node_" + String(NODE_ID)).c_str());

  // ================================
  // 3 SECOND MESH WINDOW
  // ================================
  uint32_t start = millis();
  while (millis() - start < 3000) {
    if (radio.available()) {
      String pkt;
      radio.readData(pkt);
      pkt.trim();

      if (pkt.length() == 5) {
        radio.transmit(pkt);
        radio.startReceive();
        mqtt.publish(mqtt_out_topic, pkt.c_str());
      }
    }
  }

  // SENSOR READ
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  int temp = isnan(t) ? 0 : constrain((int)t, 0, 99);
  int hum  = isnan(h) ? 0 : constrain((int)h, 0, 99);

  char payload[6];
  snprintf(payload, 6, "%1d%02d%02d", NODE_ID, temp, hum);

  // SEND SENSOR PACKET
  sendPacket(payload);

  // SHUTDOWN WI-FI + LORA
  WiFi.disconnect(true);
  radio.sleep();

  // DEEP SLEEP
  goToSleep();
}

void loop() {}

