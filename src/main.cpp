#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

// Схема
// Потенціометр: 3V3 - 10 kOhm - GND, середній вивід на GPIO4 (ADC1_CH3)
// Сервопривод SG90: сигнал (жовтий/оранжевий) на GPIO9, живлення 5V, GND спільна з ESP32

// Pins / ADC
constexpr adc1_channel_t POT_CHANNEL = ADC1_CHANNEL_3; // GPIO4
constexpr adc_atten_t ADC_ATTEN = ADC_ATTEN_DB_12;
constexpr uint8_t PIN_SERVO = 9;

// Потенціометр: повний оберт 270 градусів між напругами 0 і VCC
constexpr float POT_RANGE_DEG = 270.0f;
constexpr float VCC_MV = 3300.0f;

// Сервопривод: 180 градусів між імпульсами SERVO_MIN_US і SERVO_MAX_US
constexpr float SERVO_RANGE_DEG = 180.0f;
constexpr uint32_t SERVO_MIN_US = 500;  // 0 градусів
constexpr uint32_t SERVO_MAX_US = 2500; // 180 градусів, підібрати під свій сервопривод

// Пропорція 1:1: поворот потенціометра на 1 градус повертає вал на 1 градус. Діапазони різні, тому використовується лише спільна частина, вирівняна по центру
constexpr float POT_START_DEG = (POT_RANGE_DEG - SERVO_RANGE_DEG) / 2.0f;

constexpr uint8_t SERVO_CH = 0;
constexpr uint32_t SERVO_FREQ = 50;
constexpr uint8_t SERVO_BITS = 14;
constexpr uint32_t SERVO_PERIOD_US = 1000000 / SERVO_FREQ;
constexpr uint32_t SERVO_DUTY_MAX = (1u << SERVO_BITS) - 1;

// SMA проти шуму АЦП, щоб вал не тремтів
constexpr uint8_t SMA_WINDOW = 8;
constexpr uint32_t UPDATE_MS = 20;

esp_adc_cal_characteristics_t adcChars;

uint32_t smaBuf[SMA_WINDOW] = {};
uint32_t smaSum = 0;
uint8_t smaIdx = 0;

uint32_t lastUpdate = 0;
int loggedAngle = -1;

static uint32_t readPotMv()
{
    return esp_adc_cal_raw_to_voltage(adc1_get_raw(POT_CHANNEL), &adcChars);
}

static uint32_t smaUpdate(uint32_t value)
{
    smaSum -= smaBuf[smaIdx];
    smaBuf[smaIdx] = value;
    smaSum += value;
    smaIdx = (smaIdx + 1) % SMA_WINDOW;
    return smaSum / SMA_WINDOW;
}

// Кут потенціометра від його крайнього лівого положення
static float potAngleFromMv(uint32_t mv)
{
    return mv / VCC_MV * POT_RANGE_DEG;
}

// Кут серво від крайнього лівого положення, з обрізанням до спільного діапазону
static float servoAngleFromPot(float potDeg)
{
    return constrain(potDeg - POT_START_DEG, 0.0f, SERVO_RANGE_DEG);
}

static void servoWrite(float angleDeg)
{
    const float us = SERVO_MIN_US + angleDeg / SERVO_RANGE_DEG * (SERVO_MAX_US - SERVO_MIN_US);
    const uint32_t duty = static_cast<uint32_t>(us / SERVO_PERIOD_US * SERVO_DUTY_MAX + 0.5f);
    ledcWrite(SERVO_CH, duty);
}

void setup()
{
    Serial.begin(115200);
    delay(300);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(POT_CHANNEL, ADC_ATTEN);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_BIT_12, 1100, &adcChars);

    // Заповнюємо вікно SMA першим значенням, щоб серво не смикнувся на старті
    const uint32_t mv = readPotMv();
    for (auto &s : smaBuf)
        s = mv;
    smaSum = mv * SMA_WINDOW;

    ledcSetup(SERVO_CH, SERVO_FREQ, SERVO_BITS);
    ledcAttachPin(PIN_SERVO, SERVO_CH);
    servoWrite(servoAngleFromPot(potAngleFromMv(mv)));

    Serial.println("=== Servo 1:1 ===");
    Serial.printf("Pot %.0f deg, servo %.0f deg, used pot range %.0f..%.0f deg\n",
                  POT_RANGE_DEG, SERVO_RANGE_DEG, POT_START_DEG, POT_START_DEG + SERVO_RANGE_DEG);
}

void loop()
{
    const uint32_t now = millis();
    if (now - lastUpdate < UPDATE_MS)
        return;
    lastUpdate = now;

    const uint32_t mv = smaUpdate(readPotMv());
    const float potDeg = potAngleFromMv(mv);
    const float servoDeg = servoAngleFromPot(potDeg);
    servoWrite(servoDeg);

    // Логуємо лише зміну кута хоча б на 1 градус, щоб не засмічувати консоль
    const int angle = static_cast<int>(servoDeg + 0.5f);
    if (angle != loggedAngle)
    {
        loggedAngle = angle;
        const char *clip = potDeg < POT_START_DEG                     ? "  (clipped: left)"
                           : potDeg > POT_START_DEG + SERVO_RANGE_DEG ? "  (clipped: right)"
                                                                      : "";
        Serial.printf("U=%4lu mV  pot=%5.1f deg  servo=%3d deg from left%s\n",
                      mv, potDeg, angle, clip);
    }
}
