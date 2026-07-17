#include <Arduino.h>

#define BUTTON_PIN 4
#define CHECK_DELAY_MS 30

volatile bool eventPending = false;

uint32_t validCounter = 0;

void IRAM_ATTR onButtonEdge()
{
  eventPending = true;
}

void setup()
{
  Serial.begin(115200);
  delay(300);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonEdge, FALLING);
  Serial.println("=== Task 3: State-based debounce ===");
}

void loop()
{
  if (eventPending)
  {
    eventPending = false;
    delay(CHECK_DELAY_MS);

    if (digitalRead(BUTTON_PIN) == LOW)
    {

      validCounter++;
      Serial.printf("PRESS accepted #%lu, t=%lu ms\n", validCounter, millis());

      while (digitalRead(BUTTON_PIN) == LOW)
      {
        delay(5);
      }
      Serial.println("Released (без реакції)");
    }
    else
    {
      Serial.println("Ignored: rebound/release, pin HIGH after delay");
    }
  }
}