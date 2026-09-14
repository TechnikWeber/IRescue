#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "config.h"
#include "oracle.h"
#include "engine.h"
#include "packs.h"

IRsend  irsend(PIN_IR_SEND);
IRrecv  irrecv(PIN_IR_RECV, 1024, 15, true);
Oracle  oracle;
Engine  engine(irsend, oracle);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool learning = false;

// ---- status broadcast ----------------------------------------------------
String statusJson() {
  JsonDocument d;
  d["running"]  = engine.running();
  d["paused"]   = engine.paused();
  d["index"]    = engine.index();
  d["total"]    = engine.total();
  d["mode"]     = (int)engine.mode();
  d["sensor"]   = oracle.last();
  d["baseline"] = oracle.baseline();
  d["calibrated"] = oracle.calibrated();
  d["learning"] = learning;
  JsonArray hits = d["hits"].to<JsonArray>();
  for (const Hit &h : engine.hits()) {
    JsonObject o = hits.add<JsonObject>();
    o["index"]   = h.index;
    o["address"] = h.address;
    o["command"] = h.command;
    o["dip"]     = h.dip;
  }
  String out; serializeJson(d, out); return out;
}

void pushStatus() { ws.textAll(statusJson()); }

// ---- helpers -------------------------------------------------------------
void sendJson(AsyncWebServerRequest *req, int code, const String &body) {
  req->send(code, "application/json", body);
}
long argInt(AsyncWebServerRequest *r, const char *k, long def) {
  return r->hasParam(k, true) ? r->getParam(k, true)->value().toInt() : def;
}

// ---- routes --------------------------------------------------------------
void setupRoutes() {
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *r) {
    sendJson(r, 200, statusJson());
  });

  server.on("/api/calibrate", HTTP_POST, [](AsyncWebServerRequest *r) {
    oracle.calibrate();
    sendJson(r, 200, statusJson());
  });

  server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *r) {
    if (r->hasParam("gap", true))  engine.setPostGap(argInt(r, "gap", DEFAULT_POST_GAP_MS));
    if (r->hasParam("sens", true)) oracle.setSensitivity(r->getParam("sens", true)->value().toFloat());
    sendJson(r, 200, statusJson());
  });

  server.on("/api/start", HTTP_POST, [](AsyncWebServerRequest *r) {
    if (!oracle.calibrated()) { sendJson(r, 400, "{\"error\":\"calibrate first\"}"); return; }
    String m = r->hasParam("mode", true) ? r->getParam("mode", true)->value() : "full";
    if (m == "pack") {
      loadCandidates("/candidates.json", engine.candidates_);
      engine.start(Mode::PACK);
    } else if (m == "addr") {
      engine.start(Mode::ADDR_SWEEP, (uint16_t)argInt(r, "address", 0));
    } else {
      engine.start(Mode::FULL_NEC);
    }
    pushStatus();
    sendJson(r, 200, statusJson());
  });

  server.on("/api/stop",   HTTP_POST, [](AsyncWebServerRequest *r){ engine.stop();   pushStatus(); sendJson(r,200,statusJson()); });
  server.on("/api/pause",  HTTP_POST, [](AsyncWebServerRequest *r){ engine.pause();  pushStatus(); sendJson(r,200,statusJson()); });
  server.on("/api/resume", HTTP_POST, [](AsyncWebServerRequest *r){ engine.resume(); pushStatus(); sendJson(r,200,statusJson()); });

  // Send one code (test a candidate / confirm a hit).
  server.on("/api/test", HTTP_POST, [](AsyncWebServerRequest *r) {
    uint8_t addr = argInt(r, "address", 0);
    uint8_t cmd  = argInt(r, "command", 0);
    engine.sendNecAddrCmd(addr, cmd);
    sendJson(r, 200, "{\"ok\":true}");
  });

  // Packs
  server.on("/api/packs", HTTP_GET, [](AsyncWebServerRequest *r) {
    JsonDocument d; listPacks(d["packs"].to<JsonArray>());
    String out; serializeJson(d, out); sendJson(r, 200, out);
  });

  // Save a found code into a device pack, e.g. name="Laufband-Remote", btn="Power".
  server.on("/api/pack/save", HTTP_POST, [](AsyncWebServerRequest *r) {
    String pack = r->hasParam("pack", true) ? r->getParam("pack", true)->value() : "unbenannt";
    String btn  = r->hasParam("button", true) ? r->getParam("button", true)->value() : "Taste";
    uint8_t addr = argInt(r, "address", 0);
    uint8_t cmd  = argInt(r, "command", 0);
    if (!pack.endsWith(".ir")) pack += ".ir";
    bool ok = appendToPack(pack, btn, NEC, irsend.encodeNEC(addr, cmd), 32);
    sendJson(r, ok ? 200 : 500, ok ? "{\"ok\":true}" : "{\"error\":\"write failed\"}");
  });

  // Learning
  server.on("/api/learn/start", HTTP_POST, [](AsyncWebServerRequest *r){ learning = true;  irrecv.resume(); pushStatus(); sendJson(r,200,statusJson()); });
  server.on("/api/learn/stop",  HTTP_POST, [](AsyncWebServerRequest *r){ learning = false; pushStatus(); sendJson(r,200,statusJson()); });

  server.onNotFound([](AsyncWebServerRequest *r){ r->send(404, "text/plain", "not found"); });
}

// ---- learning loop -------------------------------------------------------
void handleLearn() {
  if (!learning) return;
  decode_results res;
  if (irrecv.decode(&res)) {
    JsonDocument d;
    d["event"]    = "learned";
    d["protocol"] = typeToString(res.decode_type);
    d["value"]    = "0x" + String((uint32_t)res.value, HEX);
    d["bits"]     = res.bits;
    if (res.decode_type == NEC) {
      d["address"] = (res.value >> 24) & 0xFF;   // see packs.h for layout
      d["command"] = (res.value >> 8) & 0xFF;
    }
    String out; serializeJson(d, out); ws.textAll(out);
    irrecv.resume();
  }
}

// ---- setup / loop --------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  if (!LittleFS.begin(true)) Serial.println("LittleFS mount failed");

  irsend.begin();
  irrecv.enableIRIn();
  oracle.begin(PIN_LIGHT);

  // Always-on AP so the UI is reachable without any network setup.
  uint64_t chip = ESP.getEfuseMac();
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "%s%04X", AP_SSID_PREFIX, (uint16_t)(chip & 0xFFFF));
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, AP_PASSWORD);
  Serial.printf("AP: %s  pw: %s  ->  http://%s/\n",
                ssid, AP_PASSWORD, WiFi.softAPIP().toString().c_str());

  ws.onEvent([](AsyncWebSocket *, AsyncWebSocketClient *c, AwsEventType type,
                void *, uint8_t *, size_t) {
    if (type == WS_EVT_CONNECT) c->text(statusJson());
  });
  server.addHandler(&ws);
  setupRoutes();
  server.begin();
}

uint32_t lastPush = 0;
void loop() {
  engine.tick();          // one code per iteration (no-op when idle/paused)
  handleLearn();

  uint32_t now = millis();
  if (now - lastPush > 250) {      // stream progress ~4x/s
    if (engine.running() || learning) oracle.read();
    pushStatus();
    ws.cleanupClients();
    lastPush = now;
  }
}
