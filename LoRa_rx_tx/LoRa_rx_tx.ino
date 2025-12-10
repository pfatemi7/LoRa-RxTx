#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

// ===============================
// CONFIG
// ===============================
#define NODE_ID 1      // 1 = master, others = slaves
#define FREQUENCY 916.0
#define BANDWIDTH 125.0
#define SPREADING_FACTOR 9
#define TXPOWER 14

#define HEARTBEAT_GAP 200       // ms between master bursts
#define HEARTBEAT_COUNT 3       // number of burstse
#define LISTEN_WINDOW 4500      // slave listening window
#define FAIL_LIMIT 5            // missed cycles before promotion
#define SLEEP_MS 5000           // sleep durations

// ===============================
// STATE
// ===============================
volatile bool rxFlag = false;
String rxdata;

RTC_DATA_ATTR bool isMaster = false;
RTC_DATA_ATTR uint32_t counter = 1;
RTC_DATA_ATTR uint8_t missCount = 0;

// ===============================
// RX INTERRUPT
// ===============================
void rxCallback() {
  rxFlag = true;
}

// ===============================
// OLED OUTPUT
// ===============================
void show(const String &a, const String &b = "") {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, a);
  display.drawString(0, 16, b);
  display.display();
}

// ===============================
// RADIO SEND WRAPPER
// ===============================
void sendPacket(const String &msg) {
  radio.transmit(msg.c_str());
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
}

// ===============================
// MASTER LOGIC
// ===============================
void runMaster() {
  String payload = "MASTER|" + String(counter++);

  for (int i = 0; i < HEARTBEAT_COUNT; i++) {
    show("MASTER", payload);
    sendPacket(payload);
    delay(HEARTBEAT_GAP);
  }
}

// ===============================
// SLAVE LOGIC
// ===============================
void runSlave() {
  show("SLAVE", "Listening...");

  unsigned long start = millis();
  bool gotMaster = false;

  while (millis() - start < LISTEN_WINDOW) {

    if (rxFlag) {
      rxFlag = false;
      radio.readData(rxdata);

      if (rxdata.startsWith("MASTER|")) {
        show("RX MASTER", rxdata);

        missCount = 0;    // reset miss tracker
        gotMaster = true;
        break;
      }

      radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
    }
  }

  if (!gotMaster) {
    missCount++;
    show("NO MASTER", "Misses: " + String(missCount));
  }

  if (missCount >= FAIL_LIMIT) {
    show("PROMOTING", "BECOME MASTER");
    isMaster = true;
    missCount = 0;
  }
}

// ===============================
// SLEEP
// ===============================
void sleepNow() {
  digitalWrite(Vext, HIGH); // turn off OLED
  display.clear();
  display.display();

  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_MS * 1000ULL);
  esp_deep_sleep_start();
}

// ===============================
// SETUP
// ===============================
void setup() {
  heltec_setup();
  Serial.begin(115200);

  // OLED ON
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  display.init();

  // RADIO INIT
  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rxCallback);
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  radio.setOutputPower(TXPOWER);
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);

  if (NODE_ID == 1)
    isMaster = true;

  show("BOOT", isMaster ? "MASTER" : "SLAVE");
  delay(500);
}

// ===============================
// LOOP
// ===============================
void loop() {
  if (isMaster) runMaster();
  else runSlave();

  sleepNow();
}
