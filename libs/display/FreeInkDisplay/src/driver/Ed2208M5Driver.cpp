#include "Ed2208M5Driver.h"

#include <BoardConfig.h>

namespace freeink {
namespace {
// M5 PaperColor keeps the CrossPoint landscape memory layout: the panel is
// physically 600x400; the app treats it as 400x600.
constexpr uint16_t M5_W = 600;
constexpr uint16_t M5_H = 400;
constexpr uint16_t M5_WB = M5_W / 8;
constexpr uint32_t M5_BUFFER = static_cast<uint32_t>(M5_WB) * M5_H;
}  // namespace

uint32_t Ed2208M5Driver::spiHz() const {
  return BoardConfig::ACTIVE.displaySpiHz != 0 ? BoardConfig::ACTIVE.displaySpiHz : 4000000;
}

PanelGeometry Ed2208M5Driver::geometry() const { return {M5_W, M5_H, M5_WB, M5_BUFFER}; }

int8_t Ed2208M5Driver::spiMiso() const { return BoardConfig::ACTIVE.sd.miso; }
int8_t Ed2208M5Driver::coCs() const { return BoardConfig::ACTIVE.sd.cs; }

void Ed2208M5Driver::begin(EpdBus& bus) {
  // TODO(m5): enable M5PM1 EPD power over I2C, then run the ED2208 init sequence.
  bus.reset();
}

void Ed2208M5Driver::display(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) {
  // TODO(m5): pack the framebuffer into ED2208 color RAM, run the dirty-window
  // interrupted refresh (honoring _completeNextRefresh), then power down if
  // turnOff. Stub: no-op so the M5 build links cleanly.
  (void)bus; (void)fb; (void)prev; (void)mode; (void)turnOff;
  _completeNextRefresh = false;
}

void Ed2208M5Driver::deepSleep(EpdBus& bus) {
  // TODO(m5): power off the panel via M5PM1.
  (void)bus;
}

PanelDriver& ed2208M5Driver() {
  static Ed2208M5Driver instance;
  return instance;
}

}  // namespace freeink
