// =======================
// MASTER (ID = 1) + JSON LOG + HTTP UPLOAD
// =======================

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>

// ---------- WiFi CONFIG ----------
const char* WIFI_SSID     = "iPhone";
const char* WIFI_PASSWORD = "12345678p";

// ---------- SERVER CONFIG ----------
const char* SERVER_URL = "https://webhook.site/1936c21a-b572-4ea3-932b-78b3d35e7f07";  // <-- change this
const char* JSON_PATH  = "/data.json";

// ---------- LoRa CONFIG ----------
#define FREQUENCY        916.3
#define BANDWIDTH        250.0   
#define SPREADING_FACTOR 9
#define TXPOWER          14

#define FRAME_PERIOD_MS  10000   // 10 seconds

String rxdata;
volatile bool rxFlag = false;
uint32_t frameCounter = 1;

unsigned long lastUpload = 0;
const unsigned long UPLOAD_INTERVAL_MS = 60000;   // upload every 60s

void rxCallback() { rxFlag = true; }

// =======================
// WiFi helpers
// =======================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  both.printf("WiFi: connecting to %s\n", WIFI_SSID);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
    delay(250);
    both.print(".");
  }
  both.println();

  if (WiFi.status() == WL_CONNECTED) {
    both.printf("WiFi: connected, IP=%s\n", WiFi.localIP().toString().c_str());
  } else {
    both.println("WiFi: FAILED, continue offline.");
  }
}

// =======================
// SPIFFS helpers
// =======================
void initFS() {
  if (!SPIFFS.begin(true)) {
    both.println("SPIFFS mount FAILED");
  } else {
    both.println("SPIFFS mounted.");
  }
}

// Append one JSON object as a line in the file
void appendJsonRecord(uint8_t node, uint32_t counterVal, const String& raw, uint32_t frame) {
  File f = SPIFFS.open(JSON_PATH, FILE_APPEND);
  if (!f) {
    both.println("FS: open for append FAILED");
    return;
  }

  // newline-delimited JSON (one object per line)
  String line;
  line.reserve(96);
  line += "{\"node\":";
  line += node;
  line += ",\"counter\":";
  line += counterVal;
  line += ",\"raw\":\"";
  line += raw;
  line += "\",\"frame\":";
  line += frame;
  line += "}\n";

  f.print(line);
  f.close();

  both.printf("FS: appended JSON: %s", line.c_str());
}

// Build array from NDJSON and POST it
void uploadJsonIfAny() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      both.println("Upload: no WiFi, skip.");
      return;
    }
  }

  File f = SPIFFS.open(JSON_PATH, FILE_READ);
  if (!f) {
    both.println("Upload: no JSON file, skip.");
    return;
  }
  if (f.size() == 0) {
    both.println("Upload: empty JSON file, skip.");
    f.close();
    return;
  }

  // Build JSON array from NDJSON
  String body = "[\n";
  bool first = true;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    if (!first) {
      body += ",\n";
    }
    first = false;
    body += line;
  }
  body += "\n]\n";
  f.close();

  both.printf("Upload: body length = %d bytes\n", body.length());

  HTTPClient http;
  http.setConnectTimeout(7000);           // 7s connect timeout
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST((uint8_t*)body.c_str(), body.length());
  both.printf("Upload: HTTP %d (%s)\n", code, http.errorToString(code).c_str());

  if (code > 0 && code < 300) {
    both.println("Upload: success, deleting local JSON file.");
    SPIFFS.remove(JSON_PATH);
  } else {
    both.println("Upload: FAILED, keeping file for retry.");

    // If connection-level error, force WiFi reconnect next time
    if (code < 0) {
      both.println("Upload: connection error, dropping WiFi and retrying later.");
      WiFi.disconnect(true);
    }
  }

  http.end();
}


// =======================
// LoRa + FRAME
// =======================
void setup() {
  heltec_setup();
  Serial.begin(115200);
  both.println("MASTER INIT");

  // FS + WiFi
  initFS();
  connectWiFi();

  // Radio
  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rxCallback);
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  radio.setOutputPower(TXPOWER);

  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  both.println("Master ready.");
}

void loop() {
  heltec_loop();

  static uint64_t lastFrame = 0;
  uint64_t now = millis();

  // -------- FRAME broadcast every 10s --------
  if (now - lastFrame >= FRAME_PERIOD_MS) {
    lastFrame = now;

    String frameMsg = "FRAME|" + String(frameCounter++);
    both.printf("TX FRAME: %s\n", frameMsg.c_str());

    radio.clearDio1Action();
    RADIOLIB(radio.transmit(frameMsg.c_str()));
    radio.setDio1Action(rxCallback);
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  // -------- Handle RX from slaves --------
  if (rxFlag) {
    rxFlag = false;

    radio.readData(rxdata);

    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("RX SLAVE: %s\n", rxdata.c_str());

      // Expect format like "20007", "30012", etc.
      if (rxdata.length() >= 2) {
        char c0 = rxdata.charAt(0);
        if (c0 >= '0' && c0 <= '9') {
          uint8_t node = (uint8_t)(c0 - '0');
          uint32_t cnt = rxdata.substring(1).toInt();

          appendJsonRecord(node, cnt, rxdata, frameCounter - 1);
        } else {
          both.println("RX parse: first char is not digit, skipping JSON log.");
        }
      } else {
        both.println("RX parse: too short, skipping JSON log.");
      }
    }

    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  // -------- Periodic upload --------
  if (now - lastUpload >= UPLOAD_INTERVAL_MS) {
    lastUpload = now;
    uploadJsonIfAny();
  }
}
