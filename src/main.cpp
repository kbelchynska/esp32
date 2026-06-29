#include <Arduino.h>

#define LED_GPIO_RED 4

#define LED_GPIO_BLUE 16

void setup()
{
  // put your setup code here, to run once:

  pinMode(LED_GPIO_RED, OUTPUT);
  pinMode(LED_GPIO_BLUE, OUTPUT);
}

void loop()
{
  // put your main code here, to run repeatedly:

  digitalWrite(LED_GPIO_RED, HIGH);
  delay(90);
  digitalWrite(LED_GPIO_RED, LOW);
  delay(30);

  digitalWrite(LED_GPIO_BLUE, HIGH);
  delay(90);
  digitalWrite(LED_GPIO_BLUE, LOW);
  delay(30);
}
