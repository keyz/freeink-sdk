#pragma once

// UC8253 panel driver — Xteink X3 (792x528 B/W with 4-level grayscale). Selected
// at runtime via FreeInkDisplay::setDisplayX3() so one firmware binary drives
// both X3 and X4. Differential refresh keeps the previous frame in controller
// RAM (0x10) and diffs the new frame written to 0x13; a stateful policy promotes
// to a full sync on boot, on demand (requestResync), or when leaving grayscale.
//
// All waveforms are injected via Uc8253X3Config (full / grayscale / image / fast
// AA banks), so the per-mode waveform tuning lives in data, not code. Active-LOW
// two-phase BUSY; SPI clock board-overridable (default 10 MHz).

#include "PanelDriver.h"

namespace freeink {

// One UC8253 waveform bank: VCOM + the four transition LUTs, 42 bytes each.
struct Uc8253LutBank {
  const uint8_t* vcom;
  const uint8_t* ww;
  const uint8_t* bw;
  const uint8_t* wb;
  const uint8_t* bb;
};

struct Uc8253X3Config {
  Uc8253LutBank full;   // reverse-exact full refresh
  Uc8253LutBank gray;   // dedicated 4-level grayscale (anti-aliased text)
  Uc8253LutBank img;    // stock image-write full sync
  Uc8253LutBank fast;   // AA fast partial-style
  uint8_t lutLen;       // bytes per LUT (42)
};

const Uc8253X3Config& uc8253X3DefaultConfig();

class Uc8253X3Driver : public PanelDriver {
 public:
  explicit Uc8253X3Driver(const Uc8253X3Config& cfg = uc8253X3DefaultConfig());

  uint32_t spiHz() const override;
  BusyPolarity busyPolarity() const override { return BusyPolarity::X3TwoPhase; }
  PanelGeometry geometry() const override;

  void begin(EpdBus& bus) override;
  void deepSleep(EpdBus& bus) override;

  void display(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) override;

  // X3 streams grayscale planes straight to controller RAM; no strip path.
  bool supportsStripGrayscale() const override { return false; }
  void copyGrayscaleLsb(EpdBus& bus, const uint8_t* lsb) override;
  void copyGrayscaleMsb(EpdBus& bus, const uint8_t* msb) override;
  void displayGray(EpdBus& bus, const uint8_t* fb, bool turnOff) override;
  void cleanupGrayscaleBuffers(EpdBus& bus, const uint8_t* bw) override;

  void requestResync(uint8_t settlePasses) override;
  void skipInitialResync() override;

 private:
  void initController(EpdBus& bus);
  void loadBank(EpdBus& bus, const Uc8253LutBank& bank);

  const Uc8253X3Config& _cfg;

  uint16_t _w;
  uint16_t _h;
  uint16_t _wb;
  uint32_t _bufferSize;

  bool _isScreenOn = false;
  bool _redRamSynced = false;
  uint8_t _initialFullSyncsRemaining = 0;
  bool _forceFullSyncNext = false;
  uint8_t _forcedConditionPassesNext = 0;
  struct GrayState {
    bool lastBaseWasPartial = false;
    bool lsbValid = false;
  } _grayState;
};

PanelDriver& uc8253X3Driver();

}  // namespace freeink
