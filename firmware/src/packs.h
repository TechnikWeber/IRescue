#pragma once
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>

// One IR code we can send. For NEC we keep address/command and let the device
// encode it (same path as the sweep) so the byte order can never drift; other
// protocols carry the already-encoded value.
struct Candidate {
  decode_type_t type;       // e.g. NEC
  bool          hasAddrCmd; // true -> use addr/cmd, false -> use value
  uint8_t       addr, cmd;
  uint64_t      value;      // encoded value for non-NEC protocols
  uint16_t      bits;       // e.g. 32 for NEC
  String        name;       // optional label
};

// Loads a compact candidate list built by tools/import_db.py.
// NEC:   {"protocol":"NEC","address":0,"command":69,"name":"Power"}
// other: {"protocol":"SONY","value":"0xA90","bits":12,"name":"Power"}
inline bool loadCandidates(const char *path, std::vector<Candidate> &out) {
  out.clear();
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  for (JsonObject c : doc["codes"].as<JsonArray>()) {
    Candidate cand;
    cand.type  = strToDecodeType(c["protocol"] | "NEC");
    cand.name  = String((const char *)(c["name"] | ""));
    if (cand.type == NEC && c["address"].is<int>()) {
      cand.hasAddrCmd = true;
      cand.addr = c["address"] | 0;
      cand.cmd  = c["command"] | 0;
      cand.bits = 32;
      cand.value = 0;
    } else {
      cand.hasAddrCmd = false;
      cand.value = strtoull(c["value"] | "0", nullptr, 0);
      cand.bits  = c["bits"] | 32;
    }
    out.push_back(cand);
  }
  return true;
}

// Appends one code to a device-specific Flipper-compatible .ir pack.
// One pack per device on purpose (e.g. /packs/laufband-remote.ir).
inline bool appendToPack(const String &packFile, const String &btnName,
                         decode_type_t type, uint64_t value, uint16_t bits) {
  if (!LittleFS.exists("/packs")) LittleFS.mkdir("/packs");
  String path = "/packs/" + packFile;
  bool fresh = !LittleFS.exists(path);
  File f = LittleFS.open(path, fresh ? "w" : "a");
  if (!f) return false;
  if (fresh) f.print("Filetype: IR signals file\nVersion: 1\n");

  f.print("#\nname: "); f.print(btnName); f.print("\n");
  if (type == NEC) {
    // Standard NEC: low byte is the meaningful address/command; the encoded
    // 32-bit value is addr, ~addr, cmd, ~cmd (MSB..LSB in our encodeNEC order).
    uint8_t cmd  = (value >> 8) & 0xFF;   // command sits in bits 8..15
    uint8_t addr = (value >> 24) & 0xFF;  // address in bits 24..31
    f.print("type: parsed\nprotocol: NEC\n");
    f.printf("address: %02X 00 00 00\n", addr);
    f.printf("command: %02X 00 00 00\n", cmd);
    // NOTE: extended-NEC (16-bit address) byte layout is a TODO.
  } else {
    // Fallback: store the raw encoded value so nothing is lost.
    f.print("type: raw\n");
    f.printf("# protocol=%s value=0x%llX bits=%u (non-NEC export TODO)\n",
             typeToString(type).c_str(), (unsigned long long)value, bits);
  }
  f.close();
  return true;
}

inline void listPacks(JsonArray arr) {
  File dir = LittleFS.open("/packs");
  if (!dir || !dir.isDirectory()) return;
  for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
    JsonObject o = arr.add<JsonObject>();
    o["name"] = String(e.name());
    o["size"] = (uint32_t)e.size();
  }
}
