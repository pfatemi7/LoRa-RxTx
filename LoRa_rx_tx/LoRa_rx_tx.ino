#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

#define RADIO_CS_PIN 8
#define RADIO_RST_PIN 12
#define RADIO_BUSY_PIN 13
#define RADIO_DIO1_PIN 14

#define FREQ 916.0
#define PERIOD_MS 30000
#define LISTEN_MS 2000
#define NORMAL_LISTEN_MS 1800

RTC_DATA_ATTR bool paired = false;
RTC_DATA_ATTR bool isMaster = false;
RTC_DATA_ATTR uint32_t counter = 0;
RTC_DATA_ATTR uint8_t missCount = 0;
RTC_DATA_ATTR uint32_t NODE_ID = 1;

void hardResetSX1262() {
  pinMode(RADIO_RST_PIN, OUTPUT);
  digitalWrite(RADIO_RST_PIN, LOW);
  delay(20);
  digitalWrite(RADIO_RST_PIN, HIGH);
  delay(20);
}

void safeSleep() {
  digitalWrite(Vext, HIGH);
  esp_sleep_enable_timer_wakeup((uint64_t)PERIOD_MS * 1000ULL);
  esp_deep_sleep_start();
}

void show(const String &a, const String &b = "") {
  display.clear();
  display.drawString(0, 0, a);
  if (b.length()) display.drawString(0, 20, b);
  display.display();
}

void enterDiscovery() {
  paired = false;
  missCount = 0;

  show("DISCOVERY", "LISTENING...");
  radio.startReceive();

  String pkt;
  uint32_t start = millis();

  while (millis() - start < LISTEN_MS) {
    if (radio.receive(pkt) == RADIOLIB_ERR_NONE) {
      pkt.trim();
      if (pkt.length() == 5) {
        uint8_t peerId = pkt.charAt(0) - '0';
        paired = true;

        // Priority: lower numeric ID has precedence (ID1 preferred; ID2 takes over if ID1 is gone)
        if (peerId > 0 && peerId > NODE_ID) {
          isMaster = true;
          show("MASTER MODE", "PRIORITY");
          delay(1000);
          return;
        }

        isMaster = false;
        show("SLAVE MODE", "PAIRED");
        delay(1500);
        safeSleep();
      }
    }
    radio.startReceive();
  }

  // No one spoke → this node becomes master
  isMaster = true;
  paired = true;
  show("MASTER MODE", "ACTIVE");
  delay(1000);
}

void setup() {
  Serial.begin(115200);

  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  delay(150);

  heltec_setup();
  display.init();

  hardResetSX1262();

  radio.begin();
  radio.setFrequency(FREQ);
  radio.setBandwidth(125.0);
  radio.setSpreadingFactor(11);
  radio.setCodingRate(5);
  radio.setCRC(true);

  delay(50);

  // ======================================
  // FIRST BOOT → DISCOVERY MODE
  // ======================================
  if (!paired) {
    enterDiscovery();
  }

  // ======================================
  // MASTER MODE
  // ======================================
  if (isMaster) {
    counter++;
    if (counter > 9999) counter = 1;

    char payload[10];
    snprintf(payload, sizeof(payload), "%1u%04u", NODE_ID, counter);

    show("MASTER TX", payload);

    // Triple transmission for reliability
    for (int i = 0; i < 3; i++) {
      radio.transmit(payload);
      delay(120);
    }

    Serial.println(String("TX: ") + payload);

    delay(1500);
    safeSleep();
    return;
  }

  // ======================================
  // SLAVE MODE
  // ======================================
  show("SLAVE MODE", "LISTENING");

  radio.startReceive();
  String pkt;
  uint32_t start = millis();
  bool gotData = false;

  while (millis() - start < NORMAL_LISTEN_MS) {
    if (radio.receive(pkt) == RADIOLIB_ERR_NONE) {
      pkt.trim();
      if (pkt.length() == 5) {
        gotData = true;
        break;
      }
    }
    radio.startReceive();
  }

  if (!gotData) {
    missCount++;

    if (missCount >= 1) {
      show("MASTER LOST", "RE-DISCOVERING");
      delay(1500);
      esp_restart();  // clean reset → clean discovery
    }

    // One missed packet → tolerate
    show("NO DATA", "ONE MISS");
    delay(1500);
    safeSleep();
    return;
  }

  // Got valid data
  missCount = 0;

  show("SLAVE RX", pkt);
  delay(1500);
  safeSleep();
}

void loop() {}
