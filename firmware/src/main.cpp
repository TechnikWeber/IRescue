#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
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
DNSServer      dns;

bool learning = false;

// ---- live log (broadcast + kept for late joiners) ------------------------
#include <deque>
std::deque<String> logbuf;
void logLine(const String &s) {
  String line = "[" + String(millis() / 1000) + "s] " + s;
  logbuf.push_back(line);
  while (logbuf.size() > 60) logbuf.pop_front();
  JsonDocument d; d["event"] = "log"; d["line"] = line;
  String out; serializeJson(d, out); ws.textAll(out);
  Serial.println(line);
}

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
  d["feedback"] = engine.waitForFeedback();
  d["learning"] = learning;

  // Live "now sending" info.
  const LastSent &ls = engine.last();
  if (ls.valid) {
    JsonObject cur = d["cur"].to<JsonObject>();
    cur["proto"] = typeToString(ls.type);
    cur["addr"]  = ls.addr;
    cur["cmd"]   = ls.cmd;
    cur["value"] = "0x" + String((uint32_t)ls.value, HEX);
    cur["bits"]  = ls.bits;
    cur["name"]  = ls.name;
  }

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
    logLine("kalibriert, Grundlinie=" + String((int)oracle.baseline()));
    sendJson(r, 200, statusJson());
  });

  server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *r) {
    if (r->hasParam("gap", true))  engine.setPostGap(argInt(r, "gap", DEFAULT_POST_GAP_MS));
    if (r->hasParam("sens", true)) oracle.setSensitivity(r->getParam("sens", true)->value().toFloat());
    sendJson(r, 200, statusJson());
  });

  server.on("/api/start", HTTP_POST, [](AsyncWebServerRequest *r) {
    // feedback=1 -> use the light sensor and stop on a blink (needs calibration)
    // feedback=0 -> "blind" mode: just send everything, no sensor needed
    bool feedback = argInt(r, "feedback", 1) != 0;
    engine.setWaitForFeedback(feedback);
    if (feedback && !oracle.calibrated()) {
      sendJson(r, 400, "{\"error\":\"calibrate first (or turn feedback off)\"}");
      return;
    }
    String m = r->hasParam("mode", true) ? r->getParam("mode", true)->value() : "full";
    if (m == "pack") {
      loadCandidates("/candidates.json", engine.candidates_);
      engine.start(Mode::PACK);
    } else if (m == "addr") {
      engine.start(Mode::ADDR_SWEEP, (uint16_t)argInt(r, "address", 0));
    } else {
      engine.start(Mode::FULL_NEC);
    }
    logLine("Suche gestartet: " + m + (feedback ? " (mit Sensor)" : " (blind)")
            + ", " + String(engine.total()) + " Codes");
    pushStatus();
    sendJson(r, 200, statusJson());
  });

  server.on("/api/stop",   HTTP_POST, [](AsyncWebServerRequest *r){ engine.stop();   logLine("gestoppt"); pushStatus(); sendJson(r,200,statusJson()); });
  server.on("/api/pause",  HTTP_POST, [](AsyncWebServerRequest *r){ engine.pause();  logLine("pausiert"); pushStatus(); sendJson(r,200,statusJson()); });
  server.on("/api/resume", HTTP_POST, [](AsyncWebServerRequest *r){ engine.resume(); logLine("weiter"); pushStatus(); sendJson(r,200,statusJson()); });

  // Manual send. NEC: address+command. Any protocol: protocol+value(+bits).
  auto handleSend = [](AsyncWebServerRequest *r) {
    if (r->hasParam("protocol", true) && r->hasParam("value", true)) {
      decode_type_t t = strToDecodeType(r->getParam("protocol", true)->value().c_str());
      uint64_t v = strtoull(r->getParam("value", true)->value().c_str(), nullptr, 0);
      uint16_t bits = argInt(r, "bits", 32);
      engine.sendOne(t, v, bits);
      logLine("gesendet " + typeToString(t) + " 0x" + String((uint32_t)v, HEX)
              + "/" + String(bits) + "b");
    } else {
      uint8_t addr = argInt(r, "address", 0);
      uint8_t cmd  = argInt(r, "command", 0);
      engine.sendNecAddrCmd(addr, cmd);
      logLine("gesendet NEC addr=" + String(addr) + " cmd=" + String(cmd));
    }
    pushStatus();
    sendJson(r, 200, "{\"ok\":true}");
  };
  server.on("/api/send", HTTP_POST, handleSend);
  server.on("/api/test", HTTP_POST, handleSend);  // backwards-compatible alias

  // Candidate list for the manual "click through the list" view.
  server.on("/api/candidates", HTTP_GET, [](AsyncWebServerRequest *r) {
    if (engine.candidates_.empty()) loadCandidates("/candidates.json", engine.candidates_);
    int off = r->hasParam("offset") ? r->getParam("offset")->value().toInt() : 0;
    int lim = r->hasParam("limit")  ? r->getParam("limit")->value().toInt()  : 200;
    JsonDocument d;
    d["total"] = (uint32_t)engine.candidates_.size();
    JsonArray a = d["items"].to<JsonArray>();
    for (int i = off; i < off + lim && i < (int)engine.candidates_.size(); i++) {
      const Candidate &c = engine.candidates_[i];
      JsonObject o = a.add<JsonObject>();
      o["i"] = i;
      o["proto"] = typeToString(c.type);
      o["name"] = c.name;
      if (c.hasAddrCmd) { o["address"] = c.addr; o["command"] = c.cmd; }
      else { o["value"] = "0x" + String((uint32_t)c.value, HEX); o["bits"] = c.bits; }
    }
    String out; serializeJson(d, out); sendJson(r, 200, out);
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

  // Captive portal: OS probe URLs -> redirect to the UI so it pops up.
  auto toPortal = [](AsyncWebServerRequest *r) {
    r->redirect("http://" + WiFi.softAPIP().toString() + "/");
  };
  server.on("/generate_204", HTTP_ANY, toPortal);       // Android
  server.on("/gen_204", HTTP_ANY, toPortal);            // Android
  server.on("/hotspot-detect.html", HTTP_ANY, toPortal);// Apple
  server.on("/canonical.html", HTTP_ANY, toPortal);     // Firefox
  server.on("/ncsi.txt", HTTP_ANY, toPortal);           // Windows
  server.on("/connecttest.txt", HTTP_ANY, toPortal);    // Windows
  server.onNotFound(toPortal);                          // anything else -> UI
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
    logLine("empfangen " + typeToString(res.decode_type) + " 0x"
            + String((uint32_t)res.value, HEX));
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

  // Always-on OPEN AP so the UI is reachable without any network setup.
  uint64_t chip = ESP.getEfuseMac();
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "%s%04X", AP_SSID_PREFIX, (uint16_t)(chip & 0xFFFF));
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);                       // open network, no password
  IPAddress ip = WiFi.softAPIP();
  dns.start(53, "*", ip);                  // resolve every domain to us -> captive portal
  Serial.printf("Open AP: %s  ->  http://%s/\n", ssid, ip.toString().c_str());

  ws.onEvent([](AsyncWebSocket *, AsyncWebSocketClient *c, AwsEventType type,
                void *, uint8_t *, size_t) {
    if (type == WS_EVT_CONNECT) {
      c->text(statusJson());
      for (const String &l : logbuf) {          // replay recent log to late joiners
        JsonDocument d; d["event"] = "log"; d["line"] = l;
        String out; serializeJson(d, out); c->text(out);
      }
    }
  });
  server.addHandler(&ws);
  setupRoutes();
  server.begin();
}

uint32_t lastPush = 0;
size_t   lastHitCount = 0;
void loop() {
  dns.processNextRequest();
  engine.tick();          // one code per iteration (no-op when idle/paused)
  handleLearn();

  // Log any new blink hit the engine just found.
  if (engine.hits().size() > lastHitCount) {
    for (size_t i = lastHitCount; i < engine.hits().size(); i++) {
      const Hit &h = engine.hits()[i];
      logLine("★ TREFFER: Lampe blinkte bei addr=" + String(h.address)
              + " cmd=" + String(h.command) + " (dip " + String(h.dip) + ")");
    }
    lastHitCount = engine.hits().size();
  }
  if (!engine.running() && lastHitCount && engine.index() == 0) lastHitCount = 0;

  uint32_t now = millis();
  if (now - lastPush > 250) {      // stream progress ~4x/s
    if (engine.running() || learning) oracle.read();
    pushStatus();
    ws.cleanupClients();
    lastPush = now;
  }
}
