// =======================
// SLAVE (ID > 1) - 1s staggered TX
// =======================

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

// ==================================
// CHANGE THIS FOR EACH SLAVE NODE
// Slave 2 → NODE_ID 2
// Slave 3 → NODE_ID 3
// ==================================
#define NODE_ID 3

// LoRa settings
#define FREQUENCY        916.3
#define BANDWIDTH        250.0   
#define SPREADING_FACTOR 9
#define TXPOWER          14

// TDMA-ish frame settings
#define FRAME_PERIOD_MS     10000    // master sends FRAME every 10s

// timing inside frame:
// Node 2: frameStart + BASE_OFFSET_MS + 0 * PER_NODE_DELAY_MS
// Node 3: frameStart + BASE_OFFSET_MS + 1 * PER_NODE_DELAY_MS
// Node 4: frameStart + BASE_OFFSET_MS + 2 * PER_NODE_DELAY_MS ...
#define BASE_OFFSET_MS      200      // small delay after FRAME
#define PER_NODE_DELAY_MS   1000     // 1 second between nodes

String rxdata;
volatile bool rxFlag = false;

bool synced = false;
uint64_t frameStart = 0;

uint32_t counter = 1;

// Prevent multiple TX inside a frame
uint64_t lastFrameSent = 0;

void rxCallback() { rxFlag = true; }

void setup() {
  heltec_setup();
  Serial.begin(115200);
  both.println();
  both.println("=========== SLAVE BOOT ===========");
  both.printf("NODE_ID = %d\n", NODE_ID);

  RADIOLIB_OR_HALT(radio.begin());

  radio.setDio1Action(rxCallback);
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  radio.setOutputPower(TXPOWER);

  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  both.println("Radio ready, waiting for FRAME...");
}

void loop() {
  heltec_loop();

  // ===========================
  // PHASE 1: FIRST TIME SYNC
  // ===========================
  if (!synced) {
    if (rxFlag) {
      rxFlag = false;
      radio.readData(rxdata);

      if (rxdata.startsWith("FRAME|")) {
        frameStart = millis();
        synced = true;
        both.println("SYNCED to first FRAME!");
      } else {
        both.printf("Got non-FRAME while unsynced: %s\n", rxdata.c_str());
      }

      RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
    }
    return;
  }

  // ===========================
  // PHASE 2: RE-SYNC EACH FRAME
  // ===========================
  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);

    if (rxdata.startsWith("FRAME|")) {
      frameStart = millis();
      both.printf("FRAME received, resync.\n");
    } else {
      // optional: see if this node hears other nodes
      // both.printf("RX while synced: %s\n", rxdata.c_str());
    }

    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  // ===========================
  // PHASE 3: STAGGERED TRANSMIT
  // ===========================

  uint64_t now = millis();
  // ID 2 → offset 0, ID 3 → offset 1 second, etc.
  int32_t nodeIndex = NODE_ID - 2;
  if (nodeIndex < 0) nodeIndex = 0;  // safety

  uint64_t myTxTime = frameStart + BASE_OFFSET_MS + (uint64_t)nodeIndex * PER_NODE_DELAY_MS;

  // generous 200 ms fire window to tolerate jitter
  if (now >= myTxTime && now < myTxTime + 200) {

    // Do NOT transmit twice in the same frame
    if (lastFrameSent == frameStart) {
      return;
    }
    lastFrameSent = frameStart;

    // Build message: e.g., "20005", "30005" etc.
    char buf[6];
    sprintf(buf, "%1d%04u", NODE_ID, counter++);
    String msg = String(buf);

    both.printf("TX SLAVE[%d]: %s at t=%llu ms (myTxTime=%llu)\n",
                NODE_ID,
                msg.c_str(),
                (unsigned long long)now,
                (unsigned long long)myTxTime);

    radio.clearDio1Action();
    RADIOLIB(radio.transmit(msg.c_str()));
    radio.setDio1Action(rxCallback);

    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

    // extra guard against re-fire jitter
    delay(250);
  }
}
