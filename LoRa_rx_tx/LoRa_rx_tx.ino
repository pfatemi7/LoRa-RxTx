#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

// ===========================
// USER CONFIG
// ===========================
#define NODE_ID 3               // Node 1 = Master, others = Slaves
#define FREQUENCY 916.0
#define BANDWIDTH 125.0
#define SPREADING_FACTOR 9
#define TXPOWER 14

#define HEARTBEAT_GAP 150       // ms
#define HEARTBEAT_COUNT 3       // bursts each cycle
#define LISTEN_WINDOW 6500      // ms — slaves wait longer now
#define FAIL_LIMIT 10           // required misses before promotion
#define SLEEP_MS 5000           // ms deep sleep time

// ===========================
// STATE
// ===========================
volatile bool rxFlag = false;
String rxdata;

RTC_DATA_ATTR bool isMaster = false;
RTC_DATA_ATTR uint32_t counter = 1;
RTC_DATA_ATTR uint8_t missCount = 0;

// ===========================
// RX INTERRUPT
// ===========================
void rxCallback() {
  rxFlag = true;
}

// ===========================
// DISPLAY
// ===========================
void show(const String &a, const String &b = "") {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, a);
  display.drawString(0, 16, b);
  display.display();
}

// ===========================
// SEND PACKET
// ===========================
void sendPacket(const String &msg) {
  radio.transmit(msg.c_str());
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
}

// ===========================
// MASTER LOGIC
// ===========================
void runMaster() {
  String payload = "MASTER|" + String(counter++);

  for (int i = 0; i < HEARTBEAT_COUNT; i++) {
    show("MASTER", payload);
    sendPacket(payload);
    delay(HEARTBEAT_GAP);
  }
}

// ===========================
// SLAVE LOGIC
// ===========================
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
        missCount = 0;
        gotMaster = true;
        break;
      }

      radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
    }
  }

  if (!gotMaster) {
    missCount++;
    show("NO MASTER", "Miss=" + String(missCount));
  }

  if (missCount >= FAIL_LIMIT) {
    show("PROMOTING", "BECOME MASTER");
    isMaster = true;
    missCount = 0;
  }
}

// ===========================
// SAFE SLEEP
// ===========================
void sleepNow() {
  digitalWrite(Vext, HIGH); // OLED + peripherals OFF
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_MS * 1000ULL);
  esp_deep_sleep_start();
}

// ===========================
// SAFE BOOT SEQUENCE
// ===========================
void setup() {

  heltec_setup();              // REQUIRED first

  // FORCE Vext OFF during startup
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);    // OFF
  delay(80);

  Serial.begin(115200);

  // RADIO INIT while Vext is OFF
  RADIOLIB_OR_HALT(radio.begin());
  radio.reset();
  delay(10);

  radio.setDio1Action(rxCallback);
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  radio.setOutputPower(TXPOWER);
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);

  // NOW turn on OLED + Vext
  digitalWrite(Vext, LOW);     // ON
  delay(60);

  display.init();
  display.clear();
  display.display();

  // Node 1 is ALWAYS master
  if (NODE_ID == 1)
    isMaster = true;

  show("BOOT", isMaster ? "MASTER" : "SLAVE");
  delay(600);
}

// ===========================
// MAIN LOOP
// ===========================
void loop() {
  if (isMaster)
    runMaster();
  else
    runSlave();

  sleepNow();
}
