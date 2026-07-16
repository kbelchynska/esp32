#include <Arduino.h>

const uint8_t LED1_PIN = 4;
const uint8_t LED2_PIN = 5;
const uint8_t LED3_PIN = 6;

struct BlinkTask
{
  uint8_t pin;
  unsigned long intervalMs;
  unsigned long lastToggle;
  bool state;
};

BlinkTask led1 = {LED1_PIN, 200, 0, false};
BlinkTask led2 = {LED2_PIN, 500, 0, false};
BlinkTask led3 = {LED3_PIN, 1000, 0, false};

void updateBlink(BlinkTask &task, unsigned long now)
{
  if (now - task.lastToggle >= task.intervalMs)
  {
    task.lastToggle = now;
    task.state = !task.state;
    digitalWrite(task.pin, task.state ? HIGH : LOW);
  }
}

void setup()
{
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);

  Serial.begin(115200);
  Serial.println("Superloop: 3 independent non-blocking LED blinkers started");
}

void loop()
{
  unsigned long now = millis();

  updateBlink(led1, now);
  updateBlink(led2, now);
  updateBlink(led3, now);
}