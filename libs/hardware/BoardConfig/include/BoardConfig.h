#pragma once

// FreeInk SDK — board hardware profiles.
//
// A BoardProfile is the single, compile-time-selected description of a device's
// pinout and capabilities. Device drivers (display / input / power) read from
// BoardConfig::ACTIVE so the same driver code adapts to any board.
//
// Selection is compile-time via -D board macros (see ACTIVE below). Boards that
// share an MCU and pinout (e.g. Xteink X3 and X4 — both ESP32-C3) intentionally
// share one profile: the X3 vs X4 panel difference is resolved at *runtime* by
// the display facade (EInkDisplay::setDisplayX3()), so a single firmware binary
// drives both. Boards on a different MCU (e.g. M5Stack PaperColor, ESP32-S3) get
// their own profile and their own binary.

#include <Arduino.h>

namespace BoardConfig {

// Physical device family. X3 is deliberately absent: it shares XTEINK_X4's
// profile and is selected at runtime, not via a distinct board macro.
enum class Board : uint8_t { XteinkX4, M5StackPaperColor, MurphyM3, DeLink };

// How the board reports button presses.
enum class InputStyle : uint8_t {
  XteinkAdcLadder,         // resistor ladder on two ADC pins (X3/X4)
  DigitalButtons,          // plain active-low GPIO buttons
  DigitalConfirmBackHold,  // confirm held > N ms synthesizes BACK (M5 PaperColor)
  DigitalFiveKey,          // 3 physical GPIO keys + synthesized events (Murphy M3)
};

// Panel controller silicon. Drivers are selected from this at begin().
enum class DisplayController : uint8_t { SSD1677, UC8253, ED2208 };

// Optional capacitive touch controller.
enum class TouchController : uint8_t { None, Chsc6x, Gt911 };

// Optional audio output path. No current board ships audio; this is scaffolding
// so a future device fills in pins and the AudioManager lights up.
enum class AudioOutput : uint8_t { None, I2sDac, PwmBuzzer };

constexpr int8_t PIN_UNASSIGNED = -1;

struct DisplayPins {
  int8_t sclk;
  int8_t mosi;
  int8_t cs;
  int8_t dc;
  int8_t rst;
  int8_t busy;
  int8_t powerEnable;
};

struct SdPins {
  int8_t sclk;
  int8_t miso;
  int8_t mosi;
  int8_t cs;
  int8_t powerEnable;
  bool separateSpi;
  uint32_t spiHz;  // 0 = use the SD manager default (40 MHz)
};

struct InputPins {
  int8_t back;
  int8_t confirm;
  int8_t left;
  int8_t right;
  int8_t up;
  int8_t down;
  int8_t power;
  bool powerActiveHigh;  // true = pressed reads HIGH (INPUT_PULLDOWN); false = active-LOW (INPUT_PULLUP)
};

// Capacitive touch panel description (TouchController::None disables it).
struct TouchConfig {
  TouchController controller;
  int8_t sda;
  int8_t scl;
  int8_t irq;
  int8_t reset;
  uint8_t i2cAddress;
  uint16_t rawMinX, rawMaxX;  // raw controller range, mapped to display coords
  uint16_t rawMinY, rawMaxY;
  bool synthesizeConfirm;     // emit a CONFIRM button event on tap
};

// PWM frontlight description (gpio == PIN_UNASSIGNED disables it).
struct FrontlightConfig {
  int8_t gpio;
  uint32_t pwmFrequency;
  uint8_t pwmResolutionBits;
  bool activeHigh;
};

// Audio output description (AudioOutput::None disables it). Scaffolding only —
// no current board populates real pins.
struct AudioConfig {
  AudioOutput output;
  int8_t bclk;    // I2S bit clock (unused for PWM buzzer)
  int8_t lrclk;   // I2S word select (unused for PWM buzzer)
  int8_t dout;    // I2S data out, or the PWM pin for a buzzer
  int8_t enable;  // amplifier enable / shutdown pin
  bool enableActiveHigh;
};

struct BoardProfile {
  Board board;
  const char* name;
  InputStyle inputStyle;
  DisplayController displayController;
  uint16_t displayWidth;
  uint16_t displayHeight;
  DisplayPins display;
  uint32_t displaySpiHz;  // 0 = use the panel driver's controller-appropriate default
  SdPins sd;
  InputPins input;
  int8_t batteryAdc;
  int8_t usbDetect;
  TouchConfig touch;
  FrontlightConfig frontlight;
  AudioConfig audio;
};

constexpr TouchConfig NO_TOUCH = {
    TouchController::None, PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED, 0, 0, 0, 0, 0, false};
constexpr FrontlightConfig NO_FRONTLIGHT = {PIN_UNASSIGNED, 0, 0, true};
constexpr AudioConfig NO_AUDIO = {AudioOutput::None, PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED, true};

// --- Xteink X4 (and X3, runtime-selected) — ESP32-C3, SSD1677 ----------------
constexpr BoardProfile XTEINK_X4 = {
    Board::XteinkX4,
    "xteink_x4",
    InputStyle::XteinkAdcLadder,
    DisplayController::SSD1677,
    800,
    480,
    {8, 10, 21, 4, 5, 6, PIN_UNASSIGNED},
    0,  // displaySpiHz: 0 -> SSD1677 driver default (40 MHz); X3 uses UC8253 default (10 MHz)
    {PIN_UNASSIGNED, 7, PIN_UNASSIGNED, 12, PIN_UNASSIGNED, false, 0},
    {0, 1, 2, 3, 4, 5, 3, false},
    0,
    20,
    NO_TOUCH,
    NO_FRONTLIGHT,
    NO_AUDIO};

// --- M5Stack PaperColor — ESP32-S3, ED2208 color panel, M5PM1 PMIC -----------
constexpr BoardProfile M5STACK_PAPER_COLOR = {
    Board::M5StackPaperColor,
    "m5stack_papercolor",
    InputStyle::DigitalConfirmBackHold,
    DisplayController::ED2208,
    400,
    600,
    {15, 13, 44, 43, 12, 11, PIN_UNASSIGNED},
    0,  // displaySpiHz: 0 -> ED2208 driver default (4 MHz)
    {15, 14, 13, 47, PIN_UNASSIGNED, false, 0},
    {1, 1, PIN_UNASSIGNED, PIN_UNASSIGNED, 10, 9, 1, false},
    PIN_UNASSIGNED,
    PIN_UNASSIGNED,
    NO_TOUCH,
    NO_FRONTLIGHT,
    NO_AUDIO};

// --- Murphy M3 (CrowPanel 3.7") — UC8253, CHSC6x touch, PWM frontlight --------
constexpr BoardProfile MURPHY_M3 = {
    Board::MurphyM3,
    "murphy_m3",
    InputStyle::DigitalFiveKey,
    DisplayController::UC8253,
    240,
    416,
    {4, 3, 5, 6, 7, 8, PIN_UNASSIGNED},
    0,  // displaySpiHz: 0 -> UC8253 driver default (10 MHz)
    {39, 13, 40, 10, PIN_UNASSIGNED, true, 0},
    {PIN_UNASSIGNED, 0, PIN_UNASSIGNED, PIN_UNASSIGNED, 1, 2, 0, false},
    PIN_UNASSIGNED,
    PIN_UNASSIGNED,
    {TouchController::Chsc6x, 13, 12, 44, 45, 0x2e, 24, 224, 24, 392, true},
    {48, 25000, 10, true},
    NO_AUDIO};

// --- de-link (X4-class GDEQ0426T82 panel on ESP32-S3) — SSD1677 + frontlight ---
// Reuses the SSD1677 driver (same controller/panel as X4); differs at the board
// level: S3 MCU, SDMMC SD, MCP73832 charge-sense, warm/cool PWM frontlight,
// optional vertical flip (-DFREEINK_DISPLAY_FLIPPED).
constexpr BoardProfile DE_LINK = {
    Board::DeLink,
    "de_link",
    InputStyle::XteinkAdcLadder,
    DisplayController::SSD1677,
    800,
    480,
    {8, 10, 21, 4, 5, 6, PIN_UNASSIGNED},
    0,  // displaySpiHz: SSD1677 default (40 MHz)
    // SDMMC 4-bit (CLK39/CMD40/D0=38/D1=48/D2=42/D3=41). The SdFat/SPI SD backend
    // can't drive SDMMC yet — an SDMMC FsFile backend is a documented follow-up.
    {39, 38, 40, 41, PIN_UNASSIGNED, true, 0},
    {0, 1, 2, 3, 4, 5, 3, true},  // power button active-HIGH (INPUT_PULLDOWN) on de-link
    4,  // batteryAdc GPIO4 (charge-status GPIO8 is passed to BatteryMonitor by firmware)
    PIN_UNASSIGNED,
    NO_TOUCH,
    // Primary brightness PWM (GPIO5). Warm/cool/rail/fault pins (GPIO6/7/17/18)
    // need the richer FrontlightManager path (follow-up).
    {5, 20000, 8, true},
    NO_AUDIO};

#if defined(CROSSPOINT_BOARD_M5STACK_PAPERCOLOR) || defined(BOARD_M5STACK_PAPERCOLOR)
constexpr BoardProfile ACTIVE = M5STACK_PAPER_COLOR;
#elif defined(CROSSPOINT_BOARD_MURPHY_M3) || defined(BOARD_MURPHY_M3)
constexpr BoardProfile ACTIVE = MURPHY_M3;
#elif defined(CROSSPOINT_BOARD_DELINK) || defined(BOARD_DELINK)
constexpr BoardProfile ACTIVE = DE_LINK;
#else
constexpr BoardProfile ACTIVE = XTEINK_X4;
#endif

constexpr bool isM5StackPaperColor() { return ACTIVE.board == Board::M5StackPaperColor; }
constexpr bool isMurphyM3() { return ACTIVE.board == Board::MurphyM3; }
constexpr bool isDeLink() { return ACTIVE.board == Board::DeLink; }
constexpr bool hasTouch() { return ACTIVE.touch.controller != TouchController::None; }
constexpr bool hasPwmFrontlight() { return ACTIVE.frontlight.gpio != PIN_UNASSIGNED; }
constexpr bool hasAudio() { return ACTIVE.audio.output != AudioOutput::None; }

}  // namespace BoardConfig
