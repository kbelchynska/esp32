#include <Arduino.h>

// ESP32: керування реле + зчитування стану контакту реле
// Використано: millis(), апаратне переривання, 10 вимірювань часу спрацювання + середнє значення

const int RELAY_CTRL_PIN = 10; // вихід — командує реле (IN на модулі)
const int CONTACT_PIN = 6;     // вхід — "слухає" реле (COM на модулі)
// Обидва піни безпечні для ESP32-S3 (не зайняті flash/PSRAM)

const int NUM_MEASUREMENTS = 14;             // кількість вимірювань
const unsigned long RELAY_ON_TIMEOUT = 2000; // макс. час очікування спрацювання, мс
const unsigned long COOLDOWN_TIME = 500;     // пауза між циклами, мс
const unsigned long DEBOUNCE_TIME = 20;      // програмний антидребезг контакту, мс

// --- Змінні, що використовуються в перериванні, ОБОВ'ЯЗКОВО volatile ---
volatile bool contactClosed = false;
volatile unsigned long lastInterruptTime = 0;
volatile unsigned long triggerMillis = 0;

// Обробник переривання: викликається при спаданні сигналу на CONTACT_PIN (HIGH -> LOW)
void IRAM_ATTR onContactChange()
{
  unsigned long now = millis();
  if (now - lastInterruptTime > DEBOUNCE_TIME)
  { // антидребезг
    contactClosed = true;
    triggerMillis = now;
    lastInterruptTime = now;
  }
}

// Стани нашого неблокуючого автомата
enum State
{
  RELAY_ON,
  WAIT_TRIGGER,
  RELAY_OFF,
  COOLDOWN,
  DONE
};
State state = RELAY_ON;

unsigned long stateStartTime = 0;
float measurements[NUM_MEASUREMENTS];
int measurementCount = 0;

void setup()
{
  Serial.begin(115200);
  delay(300);

  pinMode(RELAY_CTRL_PIN, OUTPUT);
  digitalWrite(RELAY_CTRL_PIN, HIGH); // реле вимкнене на старті
  // Якщо ваш модуль з інверсною логікою (Low Level Trigger),
  // тут має бути HIGH замість LOW, і навпаки нижче в коді.

  pinMode(CONTACT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CONTACT_PIN), onContactChange, FALLING);

  Serial.println("=== Старт вимірювань часу спрацювання реле ===");
  stateStartTime = millis();
}

void loop()
{
  unsigned long now = millis();

  switch (state)
  {

  case RELAY_ON:
    // Скидаємо флаг перед новим циклом
    noInterrupts();
    contactClosed = false;
    interrupts();

    digitalWrite(RELAY_CTRL_PIN, LOW); // команда: увімкнути реле
    stateStartTime = now;              // фіксуємо момент подачі команди (millis())
    state = WAIT_TRIGGER;
    break;

  case WAIT_TRIGGER:
    if (contactClosed)
    {
      // Переривання зафіксувало реальне замикання контакту
      unsigned long elapsed = triggerMillis - stateStartTime;
      measurements[measurementCount] = elapsed;

      Serial.print("Вимірювання #");
      Serial.print(measurementCount + 1);
      Serial.print(": ");
      Serial.print(elapsed);
      Serial.println(" мс");

      measurementCount++;
      state = RELAY_OFF;
    }
    else if (now - stateStartTime > RELAY_ON_TIMEOUT)
    {
      Serial.println("Таймаут — контакт не спрацював, повторюємо цикл");
      state = RELAY_OFF; // не рахуємо цей цикл як вимірювання, просто йдемо далі
    }
    break;

  case RELAY_OFF:
    digitalWrite(RELAY_CTRL_PIN, HIGH); // вимикаємо реле
    stateStartTime = now;
    state = COOLDOWN;
    break;

  case COOLDOWN:
    // Неблокуюча пауза між циклами (без delay())
    if (now - stateStartTime > COOLDOWN_TIME)
    {
      if (measurementCount >= NUM_MEASUREMENTS)
      {
        state = DONE;
      }
      else
      {
        state = RELAY_ON;
      }
    }
    break;

  case DONE:
  {
    float sum = 0;
    for (int i = 0; i < NUM_MEASUREMENTS; i++)
    {
      sum += measurements[i];
    }
    float average = sum / NUM_MEASUREMENTS;

    Serial.println("--------------------------------------");
    Serial.print("Середній час спрацювання реле (");
    Serial.print(NUM_MEASUREMENTS);
    Serial.print(" вимірювань): ");
    Serial.print(average);
    Serial.println(" мс");
    Serial.println("=== Завершено ===");

    while (true)
    {
      delay(1000); // зупиняємось тут, більше нічого не робимо
    }
  }
  break;
  }
}