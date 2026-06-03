#include "BatteryMonitor.h"

#include <Arduino.h>
#include <esp_idf_version.h>
#if ESP_IDF_VERSION_MAJOR < 5
#include <esp_adc_cal.h>
#endif

#include <algorithm>
#include <cmath>

BatteryMonitor::BatteryMonitor(uint8_t adcPin, float dividerMultiplier, int8_t chargeStatusPin)
    : _adcPin(adcPin), _dividerMultiplier(dividerMultiplier), _chargeStatusPin(chargeStatusPin) {
  if (_chargeStatusPin >= 0) {
    pinMode(_chargeStatusPin, INPUT_PULLUP);
  }
}

uint16_t BatteryMonitor::readPercentage() const {
  return percentageFromMillivolts(readMillivolts());
}

uint16_t BatteryMonitor::readMillivolts() const {
#if ESP_IDF_VERSION_MAJOR < 5
  // ESP-IDF 4.x doesn't have analogReadMilliVolts, so calibrate manually.
  const uint16_t raw = analogRead(_adcPin);
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  const uint16_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);
#else
  // ESP-IDF 5.x has analogReadMilliVolts
  const uint16_t mv = analogReadMilliVolts(_adcPin);
#endif

  return static_cast<uint16_t>(mv * _dividerMultiplier);
}

double BatteryMonitor::readVolts() const {
  return static_cast<double>(readMillivolts()) / 1000.0;
}

bool BatteryMonitor::isCharging() const {
  if (_chargeStatusPin < 0) {
    return false;
  }
  // MCP73832-style /STAT: LOW while charging.
  return digitalRead(_chargeStatusPin) == LOW;
}

uint16_t BatteryMonitor::percentageFromMillivolts(uint16_t millivolts) {
  double volts = millivolts / 1000.0;
  // Polynomial derived from LiPo samples
  double y = -144.9390 * volts * volts * volts +
             1655.8629 * volts * volts -
             6158.8520 * volts +
             7501.3202;

  // Clamp to [0,100] and round
  y = std::max(y, 0.0);
  y = std::min(y, 100.0);
  y = round(y);
  return static_cast<uint16_t>(y);
}
