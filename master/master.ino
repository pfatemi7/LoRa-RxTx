// =======================
// MASTER (ID = 1)
// =======================

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

#define FREQUENCY        916.3
#define BANDWIDTH        250.0   
#define SPREADING_FACTOR 9
#define TXPOWER          14

#define FRAME_PERIOD_MS 10000   // 10 seconds

String rxdata;
volatile bool rxFlag = false;
uint32_t frameCounter = 1;

void rxCallback() { rxFlag = true; }

void setup() {
  heltec_setup();
  Serial.begin(115200);
  both.println("MASTER INIT");

  RADIOLIB_OR_HALT(radio.begin());

  radio.setDio1Action(rxCallback);
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  radio.setOutputPower(TXPOWER);

  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void loop() {
  heltec_loop();

  static uint64_t lastFrame = 0;
  uint64_t now = millis();

  // FRAME sync broadcast
  if (now - lastFrame >= FRAME_PERIOD_MS) {
    lastFrame = now;

    String frameMsg = "FRAME|" + String(frameCounter++);
    both.printf("TX FRAME: %s\n", frameMsg.c_str());

    radio.clearDio1Action();
    RADIOLIB(radio.transmit(frameMsg.c_str()));
    radio.setDio1Action(rxCallback);
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  // Receive slave packets
  if (rxFlag) {
    rxFlag = false;

    radio.readData(rxdata);

    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("RX SLAVE: %s\n", rxdata.c_str());
    }

    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}
