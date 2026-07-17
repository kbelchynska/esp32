#include <Arduino.h>

#define BUTTON_PIN 4
#define POLL_INTERVAL_MS 10
#define STABLE_COUNT_NEEDED 3

enum ButtonState
{
  IDLE,
  DEBOUNCE_LOW,
  PRESSED,
  DEBOUNCE_HIGH
};

ButtonState state = IDLE;
uint32_t lastPollTime = 0;
uint8_t stableCounter = 0;
uint32_t validCounter = 0;

void setup()
{
  Serial.begin(115200);
  delay(300);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("=== Task 4: Polling state-machine debounce ===");
}

void loop()
{
  uint32_t now = millis();
  if (now - lastPollTime < POLL_INTERVAL_MS)
    return;
  lastPollTime = now;

  bool pinLow = (digitalRead(BUTTON_PIN) == LOW);

  switch (state)
  {
  case IDLE:
    if (pinLow)
    {
      state = DEBOUNCE_LOW;
      stableCounter = 1;
    }
    break;

  case DEBOUNCE_LOW:
    if (pinLow)
    {
      stableCounter++;
      if (stableCounter >= STABLE_COUNT_NEEDED)
      {
        validCounter++;
        Serial.printf("PRESS #%lu confirmed, t=%lu ms\n", validCounter, now);
        state = PRESSED;
      }
    }
    else
    {
      state = IDLE;
      stableCounter = 0;
    }
    break;

  case PRESSED:
    if (!pinLow)
    {
      state = DEBOUNCE_HIGH;
      stableCounter = 1;
    }
    break;

  case DEBOUNCE_HIGH:
    if (!pinLow)
    {
      stableCounter++;
      if (stableCounter >= STABLE_COUNT_NEEDED)
      {
        Serial.printf("Release confirmed, t=%lu ms\n", now);
        state = IDLE;
      }
    }
    else
    {
      state = PRESSED;
      stableCounter = 0;
    }
    break;
  }
}