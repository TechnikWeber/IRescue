#pragma once
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <vector>
#include "config.h"
#include "oracle.h"
#include "packs.h"

enum class Mode { PACK, ADDR_SWEEP, FULL_NEC };

struct Hit {
  uint32_t index;
  uint16_t address;
  uint16_t command;
  int      dip;      // how deep the brightness dropped
};

// Cooperative brute-force engine: call tick() repeatedly from loop().
// Each tick sends one code and watches the lamp during the gap after it.
class Engine {
 public:
  Engine(IRsend &tx, Oracle &oracle) : tx_(tx), oracle_(oracle) {}

  void start(Mode mode, uint16_t fixedAddress = 0) {
    mode_ = mode;
    fixedAddress_ = fixedAddress;
    index_ = 0;
    hits_.clear();
    switch (mode_) {
      case Mode::PACK:       total_ = candidates_.size(); break;
      case Mode::ADDR_SWEEP: total_ = 256; break;         // all commands, one address
      case Mode::FULL_NEC:   total_ = 65536; break;       // 256 addr * 256 cmd
    }
    running_ = total_ > 0;
    paused_  = false;
  }

  void stop()   { running_ = false; paused_ = false; }
  void pause()  { paused_ = true; }
  void resume() { paused_ = false; }

  // Sends one code immediately (used by the "test" button and slow re-tests).
  void sendOne(decode_type_t type, uint64_t value, uint16_t bits) {
    tx_.send(type, value, bits);
  }
  void sendNecAddrCmd(uint8_t addr, uint8_t cmd) {
    tx_.sendNEC(tx_.encodeNEC(addr, cmd), 32);
  }

  void tick() {
    if (!running_ || paused_) return;
    if (index_ >= total_) { running_ = false; return; }

    decode_type_t type = NEC;
    uint64_t value = 0; uint16_t bits = 32;
    uint16_t addr = 0, cmd = 0;

    switch (mode_) {
      case Mode::PACK: {
        const Candidate &c = candidates_[index_];
        type = c.type; value = c.value; bits = c.bits;
        if (c.type == NEC && c.hasAddrCmd) { addr = c.addr; cmd = c.cmd; value = tx_.encodeNEC(addr, cmd); }
        break;
      }
      case Mode::ADDR_SWEEP:
        addr = fixedAddress_; cmd = index_ & 0xFF;
        value = tx_.encodeNEC(addr, cmd);
        break;
      case Mode::FULL_NEC:
        addr = (index_ >> 8) & 0xFF; cmd = index_ & 0xFF;
        value = tx_.encodeNEC(addr, cmd);
        break;
    }

    tx_.send(type, value, bits);

    int dip = 0;
    if (oracle_.watch(postGapMs_, dip)) {
      hits_.push_back({index_, addr, cmd, dip});
      if (pauseOnHit_) paused_ = true;   // stop so the user can confirm
    }
    index_++;
  }

  // --- status / config ---
  bool     running() const { return running_; }
  bool     paused()  const { return paused_; }
  Mode     mode()    const { return mode_; }
  uint32_t index()   const { return index_; }
  uint32_t total()   const { return total_; }
  const std::vector<Hit> &hits() const { return hits_; }

  void setPostGap(uint32_t ms) { postGapMs_ = constrain(ms, 40u, 1000u); }
  uint32_t postGap() const { return postGapMs_; }
  void setPauseOnHit(bool b) { pauseOnHit_ = b; }

  std::vector<Candidate> candidates_;  // filled by loadCandidates() for PACK mode

 private:
  IRsend  &tx_;
  Oracle  &oracle_;
  Mode     mode_ = Mode::FULL_NEC;
  uint16_t fixedAddress_ = 0;
  uint32_t index_ = 0, total_ = 0;
  uint32_t postGapMs_ = DEFAULT_POST_GAP_MS;
  bool     running_ = false, paused_ = false, pauseOnHit_ = true;
  std::vector<Hit> hits_;
};
