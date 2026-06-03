#pragma once

// ED2208 panel driver — M5Stack PaperColor (Spectra 6 color e-paper, ESP32-S3).
//
// STATUS: STUB. The full ED2208 port (color RAM packing, interrupted-refresh
// cutoff, dirty-window partial refresh, and M5PM1 PMIC power sequencing over
// I2C) lands in a follow-up. This stub provides the geometry, bus parameters,
// and singleton so the M5 build links and the abstraction is in place. It does
// not yet drive the panel.
//
// Selection: only linked when -DFREEINK_DRIVER_ED2208 (set by the M5 board env).

#include "PanelDriver.h"

namespace freeink {

class Ed2208M5Driver : public PanelDriver {
 public:
  uint32_t spiHz() const override;
  BusyPolarity busyPolarity() const override { return BusyPolarity::ActiveLow; }
  PanelGeometry geometry() const override;
  int8_t spiMiso() const override;  // M5 shares the SD MISO
  int8_t coCs() const override;     // SD CS held high during panel transactions

  void begin(EpdBus& bus) override;
  void deepSleep(EpdBus& bus) override;
  void display(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) override;

  void requestCompleteWaveformNextRefresh() override { _completeNextRefresh = true; }

 private:
  bool _completeNextRefresh = false;
};

PanelDriver& ed2208M5Driver();

}  // namespace freeink
