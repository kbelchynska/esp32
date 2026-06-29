#include <Arduino.h>

#define LED1_PIN 2
#define LED2_PIN 4
#define EXT_BTN_PIN 5
#define BOOT_BTN_PIN 0

#define DEBOUNCE_MS 50

#define DELAY_SLOW 800
#define DELAY_FAST 150

int extLastRaw = LOW;
int bootLastRaw = HIGH;

unsigned long extChangeAt = 0;
unsigned long bootChangeAt = 0;

int extStable = LOW;
int bootStable = HIGH;

int blinkDelay = DELAY_SLOW;

void checkButtons()
{
  unsigned long now = millis();

  // BUTTON (INPUT_PULLDOWN: HIGH)
  int extRaw = digitalRead(EXT_BTN_PIN);
  if (extRaw != extLastRaw)
    extChangeAt = now;

  if (now - extChangeAt > DEBOUNCE_MS && extRaw != extStable)
  {
    extStable = extRaw;
    if (extStable == HIGH)
    {
      blinkDelay = DELAY_FAST;
      Serial.println("EXT BTN pressed -> FAST blink");
    }
  }
  extLastRaw = extRaw;

  //  BOOT (INPUT_PULLUP: LOW)
  int bootRaw = digitalRead(BOOT_BTN_PIN);
  if (bootRaw != bootLastRaw)
    bootChangeAt = now;

  if (now - bootChangeAt > DEBOUNCE_MS && bootRaw != bootStable)
  {
    bootStable = bootRaw;
    if (bootStable == LOW)
    {
      blinkDelay = DELAY_SLOW;
      Serial.println("BOOT BTN pressed -> SLOW blink");
    }
  }
  bootLastRaw = bootRaw;
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(EXT_BTN_PIN, INPUT_PULLDOWN);
  pinMode(BOOT_BTN_PIN, INPUT_PULLUP);

  Serial.println("Ready. EXT=fast blink, BOOT=slow blink");
}

void loop()
{
  // Turn on LEDs
  digitalWrite(LED1_PIN, HIGH);
  digitalWrite(LED2_PIN, HIGH);

  // During the delay, check buttons every 1 ms
  unsigned long start = millis();
  while (millis() - start < (unsigned long)blinkDelay)
    checkButtons();

  // Turn off LEDs
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  start = millis();
  while (millis() - start < (unsigned long)blinkDelay)
    checkButtons();
}
