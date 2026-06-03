#include "Uc8253X3Driver.h"

#include <BoardConfig.h>

#include "../lut/Uc8253X3Luts.h"

namespace freeink {
namespace {
constexpr uint16_t X3_WIDTH = 792;
constexpr uint16_t X3_HEIGHT = 528;
constexpr uint16_t X3_WIDTH_BYTES = X3_WIDTH / 8;  // 99
constexpr uint32_t X3_BUFFER_SIZE = static_cast<uint32_t>(X3_WIDTH_BYTES) * X3_HEIGHT;
}  // namespace

const Uc8253X3Config& uc8253X3DefaultConfig() {
  static const Uc8253X3Config cfg = {
      {lut_x3_vcom_full, lut_x3_ww_full, lut_x3_bw_full, lut_x3_wb_full, lut_x3_bb_full},
      {lut_x3_vcom_gray, lut_x3_ww_gray, lut_x3_bw_gray, lut_x3_wb_gray, lut_x3_bb_gray},
      {lut_x3_vcom_img, lut_x3_ww_img, lut_x3_bw_img, lut_x3_wb_img, lut_x3_bb_img},
      {lut_x3_vcom_fast, lut_x3_ww_fast, lut_x3_bw_fast, lut_x3_wb_fast, lut_x3_bb_fast},
      42,
  };
  return cfg;
}

Uc8253X3Driver::Uc8253X3Driver(const Uc8253X3Config& cfg)
    : _cfg(cfg), _w(X3_WIDTH), _h(X3_HEIGHT), _wb(X3_WIDTH_BYTES), _bufferSize(X3_BUFFER_SIZE) {}

uint32_t Uc8253X3Driver::spiHz() const {
  return BoardConfig::ACTIVE.displaySpiHz != 0 ? BoardConfig::ACTIVE.displaySpiHz : 10000000;
}

PanelGeometry Uc8253X3Driver::geometry() const { return {_w, _h, _wb, _bufferSize}; }

void Uc8253X3Driver::loadBank(EpdBus& bus, const Uc8253LutBank& bank) {
  bus.cmdData(0x20, bank.vcom, _cfg.lutLen);
  bus.cmdData(0x21, bank.ww, _cfg.lutLen);
  bus.cmdData(0x22, bank.bw, _cfg.lutLen);
  bus.cmdData(0x23, bank.wb, _cfg.lutLen);
  bus.cmdData(0x24, bank.bb, _cfg.lutLen);
}

void Uc8253X3Driver::initController(EpdBus& bus) {
  bus.cmd(0x00);
  bus.data(0x3F);
  bus.data(0x08);
  bus.cmd(0x61);
  bus.data(0x03);
  bus.data(0x18);
  bus.data(0x02);
  bus.data(0x58);
  bus.cmd(0x65);
  bus.data(0x00);
  bus.data(0x00);
  bus.data(0x00);
  bus.data(0x00);
  bus.cmd(0x03);
  bus.data(0x1D);
  bus.cmd(0x01);
  bus.data(0x07);
  bus.data(0x17);
  bus.data(0x3F);
  bus.data(0x3F);
  bus.data(0x17);
  bus.cmd(0x82);
  bus.data(0x1D);
  bus.cmd(0x06);
  bus.data(0x25);
  bus.data(0x25);
  bus.data(0x3C);
  bus.data(0x37);
  bus.cmd(0x30);
  bus.data(0x09);
  bus.cmd(0xE1);
  bus.data(0x02);
  loadBank(bus, _cfg.full);
  _isScreenOn = false;
}

void Uc8253X3Driver::begin(EpdBus& bus) {
  bus.reset(50);  // X3 needs an extra settle after reset
  _redRamSynced = false;
  _initialFullSyncsRemaining = 2;
  _forceFullSyncNext = false;
  _forcedConditionPassesNext = 0;
  _grayState = {};
  initController(bus);
}

void Uc8253X3Driver::display(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) {
  (void)prev;
  // On X3, treat HALF as fast differential; only FULL forces the full sync.
  const bool fastMode = (mode != RefreshMode::Full);

  const bool forcedFullSync = _forceFullSyncNext;
  const bool doFullSync =
      !fastMode || !_redRamSynced || _initialFullSyncsRemaining > 0 || forcedFullSync;
  _grayState.lastBaseWasPartial = !doFullSync;

  if (doFullSync) {
    // Full sync: image LUTs, inverted data to both RAMs.
    loadBank(bus, _cfg.img);
    bus.cmd(0x13);
    bus.writeMirroredPlane(fb, _h, _wb, true);
    bus.cmd(0x10);
    bus.writeMirroredPlane(fb, _h, _wb, true);
    bus.cmdData2(0x50, 0xA9, 0x07);
  } else {
    // Fast differential: full LUTs, RED RAM (0x10) retains the previous frame.
    loadBank(bus, _cfg.full);
    bus.cmd(0x13);
    bus.writeMirroredPlane(fb, _h, _wb, false);
    bus.cmdData2(0x50, 0x29, 0x07);
  }

  if (!_isScreenOn || doFullSync) {
    bus.cmd(0x04);
    bus.waitBusy(" X3_CMD04");
    _isScreenOn = true;
  }

  bus.cmd(0x12);
  bus.waitBusy(" X3_CMD12");

  if (turnOff) {
    bus.cmd(0x02);
    bus.waitBusy(" X3_CMD02_POWEROFF");
    _isScreenOn = false;
  }

  if (!fastMode) delay(200);

  // One light settle pass after the first major full sync improves early
  // page-turn quality without the old multi-pass cost.
  uint8_t postConditionPasses = 0;
  if (doFullSync) {
    if (forcedFullSync) postConditionPasses = _forcedConditionPassesNext;
    else if (_initialFullSyncsRemaining == 1) postConditionPasses = 1;
  }

  if (postConditionPasses > 0) {
    const uint16_t xEnd = static_cast<uint16_t>(_w - 1);
    const uint16_t yEnd = static_cast<uint16_t>(_h - 1);
    const uint8_t w[9] = {0x00, 0x00, static_cast<uint8_t>(xEnd >> 8), static_cast<uint8_t>(xEnd & 0xFF),
                          0x00, 0x00, static_cast<uint8_t>(yEnd >> 8), static_cast<uint8_t>(yEnd & 0xFF),
                          0x01};
    loadBank(bus, _cfg.full);
    bus.cmdData2(0x50, 0x29, 0x07);
    for (uint8_t i = 0; i < postConditionPasses; i++) {
      bus.cmd(0x91);
      bus.cmdData(0x90, w, 9);
      bus.cmd(0x13);
      bus.writeMirroredPlane(fb, _h, _wb, false);
      bus.cmd(0x92);
      if (!_isScreenOn) {
        bus.cmd(0x04);
        bus.waitBusy(" X3_CMD04");
        _isScreenOn = true;
      }
      bus.cmd(0x12);
      bus.waitBusy(" X3_CMD12(cond)");
    }
  }

  // Sync RED RAM (0x10) with the non-inverted current frame for the next diff.
  bus.cmd(0x10);
  bus.writeMirroredPlane(fb, _h, _wb, false);
  _redRamSynced = true;

  if (doFullSync && _initialFullSyncsRemaining > 0) {
    _initialFullSyncsRemaining--;
  }
  _forceFullSyncNext = false;
  _forcedConditionPassesNext = 0;
}

void Uc8253X3Driver::copyGrayscaleLsb(EpdBus& bus, const uint8_t* lsb) {
  if (!lsb) {
    _grayState.lsbValid = false;
    return;
  }
  bus.cmd(0x10);
  bus.writeMirroredPlane(lsb, _h, _wb, false);
  _grayState.lsbValid = true;
}

void Uc8253X3Driver::copyGrayscaleMsb(EpdBus& bus, const uint8_t* msb) {
  if (!msb || !_grayState.lsbValid) return;
  bus.cmd(0x13);
  bus.writeMirroredPlane(msb, _h, _wb, false);
}

void Uc8253X3Driver::displayGray(EpdBus& bus, const uint8_t* fb, bool turnOff) {
  (void)fb;
  if (!_grayState.lsbValid) return;

  loadBank(bus, _cfg.gray);
  bus.cmdData2(0x50, 0x29, 0x07);

  if (!_isScreenOn) {
    bus.cmd(0x04);
    bus.waitBusy(" X3_CMD04(gray)");
    _isScreenOn = true;
  }

  bus.cmd(0x12);
  bus.waitBusy(" X3_CMD12(gray)");

  if (turnOff) {
    bus.cmd(0x02);
    bus.waitBusy(" X3_CMD02_POWEROFF(gray)");
    _isScreenOn = false;
  }

  // RAM baseline is re-established by cleanupGrayscaleBuffers() after this.
  _redRamSynced = false;
  _forceFullSyncNext = false;
  _forcedConditionPassesNext = 0;
  _grayState.lsbValid = false;
}

void Uc8253X3Driver::cleanupGrayscaleBuffers(EpdBus& bus, const uint8_t* bw) {
  if (!bw) return;
  // Rebase both planes from the restored BW buffer so the next differential
  // update compares from a coherent known state.
  bus.cmd(0x13);
  bus.writeMirroredPlane(bw, _h, _wb, false);
  bus.cmd(0x10);
  bus.writeMirroredPlane(bw, _h, _wb, false);
  _redRamSynced = true;
  _forceFullSyncNext = false;
  _forcedConditionPassesNext = 0;
}

void Uc8253X3Driver::requestResync(uint8_t settlePasses) {
  _forceFullSyncNext = true;
  _forcedConditionPassesNext = settlePasses;
}

void Uc8253X3Driver::skipInitialResync() {
  _initialFullSyncsRemaining = 0;
  _redRamSynced = true;
}

void Uc8253X3Driver::deepSleep(EpdBus& bus) {
  if (_isScreenOn) {
    bus.cmd(0x02);  // power off analog rails
    bus.waitBusy(" X3 power-down");
    _isScreenOn = false;
  }
  bus.cmd(0x07);  // UC8253 deep sleep
  bus.data(0xA5);
}

PanelDriver& uc8253X3Driver() {
  static Uc8253X3Driver instance;
  return instance;
}

}  // namespace freeink
