/*
 * LoRa TDMA Slave Node
 * 
 * Copyright (c) 2024 LoRa RxTx Contributors
 * 
 * This software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and noninfringement.
 * 
 * See LICENSE file for full license terms.
 */

// =======================
// SLAVE (ID > 1) - 1s staggered TX + DEEP SLEEP
// =======================

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>
#include <esp_sleep.h>

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

// FRAME & timing
#define FRAME_PERIOD_MS     10000    // master sends FRAME every 10s

// timing inside frame:
// Node 2: frameStart + BASE_OFFSET_MS + 0 * PER_NODE_DELAY_MS
// Node 3: frameStart + BASE_OFFSET_MS + 1 * PER_NODE_DELAY_MS
// Node 4: frameStart + BASE_OFFSET_MS + 2 * PER_NODE_DELAY_MS ...
#define BASE_OFFSET_MS      200      // small delay after FRAME
#define PER_NODE_DELAY_MS   1000     // 1 second between nodes

// Deep sleep duration (seconds)
#define SLEEP_SECONDS       10       // nominally 1 frame

String rxdata;
volatile bool rxFlag = false;

bool synced = false;
uint64_t frameStart = 0;

// Counter must survive deep sleep
RTC_DATA_ATTR uint32_t counter = 1;

// Prevent multiple TX inside a frame (no need to persist across sleep)
uint64_t lastFrameSent = 0;

void rxCallback() { rxFlag = true; }

void goToSleep() {
  both.printf("NODE %d going to deep sleep for %d s\n", NODE_ID, SLEEP_SECONDS);

  // small flush so you see logs
  Serial.flush();

  // Enable timer wakeup
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);

  // Deep sleep (CPU stops here)
  esp_deep_sleep_start();
}

void setup() {
  heltec_setup();
  Serial.begin(115200);
  both.println();
  both.println("=========== SLAVE BOOT ===========");
  both.printf("NODE_ID = %d\n", NODE_ID);
  both.printf("Counter starts at: %u\n", counter);

  RADIOLIB_OR_HALT(radio.begin());

  radio.setDio1Action(rxCallback);
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  radio.setOutputPower(TXPOWER);

  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  both.println("Radio ready, waiting for FRAME...");
  synced = false;
  lastFrameSent = 0;
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
      // optional debug if nodes hear each other
      // both.printf("RX while synced: %s\n", rxdata.c_str());
    }

    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  // ===========================
  // PHASE 3: STAGGERED TRANSMIT
  // ===========================

  uint64_t now = millis();
  // nodeIndex: ID 2 → 0, ID 3 → 1, ID 4 → 2, ...
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

    // Build message: e.g., "20005" / "30012"
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

    // No need to stay awake after this → go to sleep
    goToSleep();
  }

  // If we're between frames, just idle & listen.
}
