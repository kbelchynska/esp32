#include <Arduino.h>

#define BUTTON_PIN 4

volatile uint32_t counter = 0;
volatile bool triggered = false;

void IRAM_ATTR onButtonPress()
{
  counter++;
  triggered = true;
}

void setup()
{
  Serial.begin(115200);
  delay(300);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);
  Serial.println("=== Task 1: No debounce ===");
}

void loop()
{
  if (triggered)
  {
    triggered = false;
    Serial.printf("Interrupt #%lu at t=%lu ms\n", counter, millis());
  }
}