#include <Arduino.h>
#include <driver/adc.h>

// Pins / ADC
constexpr adc1_channel_t POT_LED_CHANNEL = ADC1_CHANNEL_3;   // GPIO4
constexpr adc1_channel_t POT_MOTOR_CHANNEL = ADC1_CHANNEL_4; // GPIO5
constexpr uint8_t PIN_LED = 7;
constexpr uint8_t PIN_MOTOR = 6;

constexpr uint16_t ADC_MAX = 4095;
constexpr uint8_t OVERSAMPLE = 8; // усереднення кількох зчитувань проти шуму

// PWM
// Канал n використовує таймер n / 2, тому канали 0 і 2 мають окремі таймери і частоту одного каналу можна змінювати, не зачіпаючи інший
constexpr uint8_t LED_CH = 0;
constexpr uint32_t LED_FREQ = 5000; // 5 kHz, мерехтіння оку не видно
constexpr uint8_t MOTOR_CH = 2;
constexpr uint32_t MOTOR_FREQ = 20000; // 20 kHz, вище чутного діапазону, мотор не пищить
constexpr uint8_t PWM_BITS = 10;
constexpr uint32_t PWM_MAX = (1u << PWM_BITS) - 1; // 1023

// Мотор не рушає з місця при малому заповненні, тому нижче DEADZONE мотор вимкнений, вище duty починається з MOTOR_MIN_DUTY
constexpr uint16_t MOTOR_DEADZONE = 100; // у raw-одиницях ADC
constexpr uint32_t MOTOR_MIN_DUTY = 350; // підібрати під свій мотор

constexpr uint32_t UPDATE_MS = 20;
constexpr uint32_t PRINT_MS = 200;

uint32_t ledDuty = 0;
uint32_t motorDuty = 0;
uint16_t ledRaw = 0;
uint16_t motorRaw = 0;
uint32_t lastUpdate = 0;
uint32_t lastPrint = 0;

static uint16_t readPot(adc1_channel_t ch)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < OVERSAMPLE; i++)
        sum += adc1_get_raw(ch);
    return sum / OVERSAMPLE;
}

static uint32_t ledDutyFromRaw(uint16_t raw)
{

    const float x = static_cast<float>(raw) / ADC_MAX;
    return static_cast<uint32_t>(x * x * PWM_MAX + 0.5f);
}

static uint32_t motorDutyFromRaw(uint16_t raw)
{
    if (raw < MOTOR_DEADZONE)
        return 0;
    return map(raw, MOTOR_DEADZONE, ADC_MAX, MOTOR_MIN_DUTY, PWM_MAX);
}

void setup()
{
    Serial.begin(115200);
    delay(300);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(POT_LED_CHANNEL, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(POT_MOTOR_CHANNEL, ADC_ATTEN_DB_12);

    ledcSetup(LED_CH, LED_FREQ, PWM_BITS);
    ledcAttachPin(PIN_LED, LED_CH);
    ledcWrite(LED_CH, 0);

    ledcSetup(MOTOR_CH, MOTOR_FREQ, PWM_BITS);
    ledcAttachPin(PIN_MOTOR, MOTOR_CH);
    ledcWrite(MOTOR_CH, 0);

    Serial.println("=== PWM: LED + motor ===");
    Serial.printf("LED   : GPIO%u, ch %u, %lu Hz, %u bit\n", PIN_LED, LED_CH, LED_FREQ, PWM_BITS);
    Serial.printf("Motor : GPIO%u, ch %u, %lu Hz, %u bit\n", PIN_MOTOR, MOTOR_CH, MOTOR_FREQ, PWM_BITS);
}

void loop()
{
    const uint32_t now = millis();

    // Кожен потенціометр керує лише своїм каналом
    if (now - lastUpdate >= UPDATE_MS)
    {
        lastUpdate = now;

        ledRaw = readPot(POT_LED_CHANNEL);
        ledDuty = ledDutyFromRaw(ledRaw);
        ledcWrite(LED_CH, ledDuty);

        motorRaw = readPot(POT_MOTOR_CHANNEL);
        motorDuty = motorDutyFromRaw(motorRaw);
        ledcWrite(MOTOR_CH, motorDuty);
    }

    if (now - lastPrint >= PRINT_MS)
    {
        lastPrint = now;
        Serial.printf("LED raw:%4u duty:%4lu (%3lu%%)   MOTOR raw:%4u duty:%4lu (%3lu%%)\n",
                      ledRaw, ledDuty, ledDuty * 100 / PWM_MAX,
                      motorRaw, motorDuty, motorDuty * 100 / PWM_MAX);
    }
}
