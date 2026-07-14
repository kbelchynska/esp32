#include <Arduino.h>

// Configuration (constexpr — compile-time constants)
namespace Cfg
{
  constexpr uint8_t LED_PIN = 2;
  constexpr uint8_t BTN_PIN = 0;          // BOOT button, INPUT_PULLUP
  constexpr uint32_t BLINK_HALF_MS = 500; // half-period: on=500ms, off=500ms
  constexpr uint32_t BAUD_RATE = 115200;
}

// Strongly-typed enums
enum class LedState
{
  Off = LOW,
  On = HIGH
};
enum class LedMode
{
  Blinking,
  AlwaysOn,
  AlwaysOff
};

// Led encapsulates pin + state, no dynamic memory
class Led
{
public:
  // static const: additional setting, not a compile-time expression
  static const uint8_t BLINKS_ON_PRESS;

  void init()
  {
    pinMode(Cfg::LED_PIN, OUTPUT);
    set(LedState::Off);
  }

  void set(LedState s)
  {
    _state = s;
    digitalWrite(Cfg::LED_PIN, static_cast<uint8_t>(s));
  }

  void setMode(LedMode m)
  {
    _mode = m;
    _lastToggle = millis();
    if (m == LedMode::AlwaysOn)
      set(LedState::On);
    if (m == LedMode::AlwaysOff)
      set(LedState::Off);
  }

  LedMode getMode() const { return _mode; }

  // called every loop() iteration — non-blocking
  void update()
  {
    if (_mode != LedMode::Blinking)
      return;
    const uint32_t now = millis();
    if (now - _lastToggle >= Cfg::BLINK_HALF_MS)
    {
      _lastToggle = now;
      set(_state == LedState::On ? LedState::Off : LedState::On);
    }
  }

private:
  LedState _state = LedState::Off;
  LedMode _mode = LedMode::Blinking;
  uint32_t _lastToggle = 0;
};

// out-of-class definition required for static const (non-constexpr)
const uint8_t Led::BLINKS_ON_PRESS = 3;

volatile bool buttonPressed = false;

void IRAM_ATTR onBtn()
{
  buttonPressed = true;
}

// Superloop timing
namespace Perf
{
  static uint32_t count = 0;
  static uint32_t totalUs = 0;
  constexpr uint32_t WINDOW = 1000;
}

//  Single global instance
Led led;

//  Setup
void setup()
{
  Serial.begin(Cfg::BAUD_RATE);
  led.init();
  pinMode(Cfg::BTN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Cfg::BTN_PIN), onBtn, FALLING);
  Serial.println("Ready | BOOT cycles: Blinking -> AlwaysOn -> AlwaysOff");
}

//  Superloop
void loop()
{
  const uint32_t t0 = micros();

  // handle button: логіка перемикання — поза ISR
  if (buttonPressed)
  {
    buttonPressed = false;
    switch (led.getMode())
    {
    case LedMode::Blinking:
      led.setMode(LedMode::AlwaysOn);
      break;
    case LedMode::AlwaysOn:
      led.setMode(LedMode::AlwaysOff);
      break;
    case LedMode::AlwaysOff:
      led.setMode(LedMode::Blinking);
      break;
    }
  }

  led.update();

  Perf::totalUs += micros() - t0;
  if (++Perf::count >= Perf::WINDOW)
  {
    Serial.printf("[perf] avg %.2f us/iter over %lu iters\n",
                  static_cast<float>(Perf::totalUs) / Perf::WINDOW,
                  Perf::WINDOW);
    Perf::count = 0;
    Perf::totalUs = 0;
  }
}
