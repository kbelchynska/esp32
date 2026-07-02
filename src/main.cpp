#include <Arduino.h>

#define LDR_PIN 4
#define LED_PIN 7
#define BAUDRATE 115200
#define WINDOW_SIZE 8

int readings[WINDOW_SIZE];
int idx = 0, sum = 0;
int threshold = 1800;
int hysteresis = 400;
bool ledOn = false;
const int CH = 0, FREQ = 5000, RES = 8;

void setup()
{
  Serial.begin(BAUDRATE);
  // pinMode(LED_PIN, OUTPUT);
  ledcSetup(CH, FREQ, RES);
  ledcAttachPin(LED_PIN, CH);
  // delay(300);
  Serial.println("ADC start ...");

  for (int i = 0; i < WINDOW_SIZE; i++)
  {
    readings[i] = analogRead(LDR_PIN);
    sum += readings[i];
  }
  // int vMin = 4095, vMax = 0;
  // Serial.println("The calibration for 5s: will close / will open the sensor!");

  // unsigned long t0 = millis();
  // while (millis() - t0 < 5000) {
  //   int v = analogRead(LDR_PIN);
  //   if (v < vMin) vMin = v;
  //   if (v > vMax) vMax = v;
  //   delay(10);
  // }

  // threshold = (vMin + vMax) / 2;
  // Serial.print("vMin = "); Serial.print(vMin);
  // Serial.print("\tvMax = "); Serial.print(vMax);
  // Serial.print("\tThreshold = "); Serial.println(threshold);
}

void loop()
{

  sum = sum - readings[idx];
  // 12 bits 0 - 4095 (4096)
  // 8 bits 0-255 (256)
  int adc = analogRead(LDR_PIN);
  readings[idx] = adc;
  sum += adc;
  idx = (idx + 1) % WINDOW_SIZE;

  int avg = sum / WINDOW_SIZE;
  // div on 4095, max = 4095 (from 0)
  float volts = adc / 4095.0 * 3.3;

  if (!ledOn && avg < threshold - hysteresis)
    ledOn = true;
  if (ledOn && avg > threshold + hysteresis)
    ledOn = false;

  // 2. Яскравість рахуємо тільки коли увімкнено
  int bright = 0;
  if (ledOn)
  {
    // верхня межа мапінгу = поріг вимкнення,
    // бо вище цього avg ledOn вже стане false
    bright = map(avg, 0, threshold + hysteresis, 255, 0);
    bright = constrain(bright, 0, 255);
  }

  ledcWrite(CH, bright);

  // if (adc < threshold) {
  //   digitalWrite(LED_PIN, HIGH);
  // } else {
  //   digitalWrite(LED_PIN, LOW);
  // }
  // digitalWrite(LED_PIN, ledOn ? HIGH : LOW);

  Serial.print("ADC = ");
  Serial.print(adc);
  Serial.print("\tV:");
  Serial.print(volts, 2);
  Serial.print("\tAVG:");
  Serial.print(avg);
  Serial.print("\tPWM:");
  Serial.println(bright);
  // Serial.print("\tLED:");
  // Serial.println(ledOn);
  delay(50);
}