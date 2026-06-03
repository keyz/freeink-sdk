#include "Uc8253MurphyDriver.h"

#include <BoardConfig.h>

namespace freeink {
namespace {
constexpr uint16_t M3_W = 240;
constexpr uint16_t M3_H = 416;
constexpr uint16_t M3_WB = M3_W / 8;
constexpr uint32_t M3_BUFFER = static_cast<uint32_t>(M3_WB) * M3_H;
}  // namespace

uint32_t Uc8253MurphyDriver::spiHz() const {
  return BoardConfig::ACTIVE.displaySpiHz != 0 ? BoardConfig::ACTIVE.displaySpiHz : 10000000;
}

PanelGeometry Uc8253MurphyDriver::geometry() const { return {M3_W, M3_H, M3_WB, M3_BUFFER}; }

void Uc8253MurphyDriver::begin(EpdBus& bus) {
  // TODO(murphy): longer reset, UC8253 init with OEM LUTs.
  bus.reset(200);
}

void Uc8253MurphyDriver::display(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) {
  // TODO(murphy): rotate logical 240x416 -> controller 416x240, load OEM LUT,
  // write RAM, refresh. Stub: no-op so the Murphy build links cleanly.
  (void)bus; (void)fb; (void)prev; (void)mode; (void)turnOff;
}

void Uc8253MurphyDriver::deepSleep(EpdBus& bus) {
  bus.cmd(0x07);
  bus.data(0xA5);
}

PanelDriver& uc8253MurphyDriver() {
  static Uc8253MurphyDriver instance;
  return instance;
}

}  // namespace freeink
