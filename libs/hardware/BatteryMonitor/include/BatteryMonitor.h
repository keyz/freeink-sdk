#pragma once
#include <cstdint>

// FreeInk SDK — battery monitor.
//
// Reads a divided LiPo voltage off an ADC pin and maps it to a percentage.
// An optional charge-status pin (e.g. MCP73832 /STAT, active-LOW) enables
// isCharging() on boards that wire one up; it defaults to unused so existing
// two-argument construction is unchanged.
class BatteryMonitor {
public:
    static constexpr int8_t PIN_NONE = -1;

    // Optional divider multiplier defaults to 2.0; optional charge-status pin
    // defaults to unused.
    explicit BatteryMonitor(uint8_t adcPin, float dividerMultiplier = 2.0f, int8_t chargeStatusPin = PIN_NONE);

    // Read voltage and return percentage (0-100)
    uint16_t readPercentage() const;

    // Read the battery voltage in millivolts (accounts for divider)
    uint16_t readMillivolts() const;

    // Read the battery voltage in volts (accounts for divider)
    double readVolts() const;

    // True when the charger reports an active charge (charge-status pin LOW).
    // Always false when no charge-status pin is configured.
    bool isCharging() const;

    // Percentage (0-100) from a millivolt value
    static uint16_t percentageFromMillivolts(uint16_t millivolts);

private:
    uint8_t _adcPin;
    float _dividerMultiplier;
    int8_t _chargeStatusPin;
};
