#include <Arduino.h>

#define BAUDRATE 115200

constexpr int8_t PIN_BTN = 4;
constexpr int8_t PIN_RAW_OUT = 16;
constexpr int8_t PIN_DEBOUNCE_OUT = 17;
constexpr int32_t DEBOUNCE_MS = 30;

volatile uint32_t rawCount = 0;
uint32_t debCount = 0;

uint32_t lastChangeMs = 0;

int lastReading = HIGH, lastStable = HIGH;

#define BUTTON_LEFT 15
#define BUTTON_RIGHT 3

int16_t counter_left = 0;
int16_t counter_right = 0;

void IRAM_ATTR onEdge()
{
  rawCount++;
  Serial.println("\nLEFT Button Pressed! Count: " + String(rawCount));
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(BAUDRATE);
  pinMode(PIN_BTN, INPUT_PULLDOWN);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_RAW_OUT, OUTPUT);
  pinMode(PIN_DEBOUNCE_OUT, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN), onEdge, CHANGE);
  Serial.printf("Logic analyzer demo: raw vs debounced \n");
}

void loop()
{
  // put your main code here, to run repeatedly:
  int reading = digitalRead(PIN_BTN);

  digitalWrite(PIN_RAW_OUT, reading);

  if (reading != lastReading)
  {
    lastChangeMs = millis();
    lastReading = reading;
  }

  if (millis() - lastChangeMs >= DEBOUNCE_MS)
  {
    if (reading != lastStable)
    {
      lastStable = reading;
      digitalWrite(PIN_DEBOUNCE_OUT, lastStable);
      digitalWrite(LED_BUILTIN, lastStable == LOW);

      if (lastStable == LOW)
      {
        debCount++;
        Serial.printf("RAW=%lu DEBOUNCED=%lu\n", rawCount, debCount);
      }
    }
  }
}