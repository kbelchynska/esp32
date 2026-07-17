#include <Arduino.h>

#define BUTTON_PIN 4
#define DEBOUNCE_MS 50

volatile uint32_t rawIsrCount = 0;
volatile bool isrEvent = false;

uint32_t validCounter = 0;
uint32_t lastValidTime = 0;

void IRAM_ATTR onButtonPress()
{
  rawIsrCount++;
  isrEvent = true;
}

void setup()
{
  Serial.begin(115200);
  delay(300);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);
  Serial.println("=== Task 2: Time-based debounce (50 ms) ===");
}

void loop()
{
  if (isrEvent)
  {
    isrEvent = false;
    uint32_t now = millis();

    if (now - lastValidTime >= DEBOUNCE_MS)
    {
      validCounter++;
      lastValidTime = now;
      Serial.printf("VALID press #%lu (raw ISR count=%lu), t=%lu ms\n",
                    validCounter, rawIsrCount, now);
    }
    else
    {
      Serial.printf("Ignored bounce, dt=%lu ms (raw ISR count=%lu)\n",
                    now - lastValidTime, rawIsrCount);
    }
  }
}