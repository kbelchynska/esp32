#include <Arduino.h>
#include <driver/adc.h>

// Схема: 3V3 - LDR - GPIO4 (ADC1_CH3) - 10 kOhm - GND
// Темно: опір LDR росте, напруга на GPIO4 падає, значення ADC менше
// Світло: опір LDR падає, напруга на GPIO4 росте, значення ADC більше
// LED: GPIO7 - 220 Ohm - LED - GND

// Pins / ADC
constexpr uint8_t PIN_LED = 7;
constexpr adc1_channel_t LDR_CHANNEL = ADC1_CHANNEL_3; // GPIO4 на ESP32-S3

// SMA
constexpr uint8_t SMA_WINDOW = 16;
constexpr uint32_t SAMPLE_MS = 20;

// Гістерезис у raw-одиницях ADC 0..4095
constexpr uint16_t TH_DARK = 1400;  // SMA нижче цього: темно, LED ON
constexpr uint16_t TH_LIGHT = 2200; // SMA вище цього: світло, LED OFF
static_assert(TH_DARK < TH_LIGHT, "TH_DARK must be below TH_LIGHT");

constexpr uint32_t PRINT_MS = 200;

// Simple Moving Average на кільцевому буфері з бігучою сумою
class SimpleMovingAverage
{
public:
    // Заповнюємо все вікно першим значенням, щоб SMA одразу був валідним
    void reset(uint16_t value)
    {
        for (auto &s : buf_)
            s = value;
        sum_ = static_cast<uint32_t>(value) * SMA_WINDOW;
        idx_ = 0;
    }

    // Віднімаємо найстаріший семпл, додаємо новий
    uint16_t update(uint16_t value)
    {
        sum_ -= buf_[idx_];
        buf_[idx_] = value;
        sum_ += value;
        idx_ = (idx_ + 1) % SMA_WINDOW;
        return sum_ / SMA_WINDOW;
    }

private:
    uint16_t buf_[SMA_WINDOW] = {};
    uint32_t sum_ = 0;
    uint8_t idx_ = 0;
};

// State
SimpleMovingAverage sma;
uint16_t rawValue = 0;
uint16_t smaValue = 0;
bool ledOn = false;
uint32_t lastSample = 0;
uint32_t lastPrint = 0;

// ADC oneshot
static void adcInit()
{
    adc1_config_width(ADC_WIDTH_BIT_12);                     // 0..4095
    adc1_config_channel_atten(LDR_CHANNEL, ADC_ATTEN_DB_11); // діапазон приблизно 0..3.1 V
}

static inline uint16_t adcReadOnce()
{
    return static_cast<uint16_t>(adc1_get_raw(LDR_CHANNEL));
}

static void updateLed(uint16_t avg)
{
    if (!ledOn && avg < TH_DARK)
    {
        ledOn = true;
        Serial.printf("[%6lu ms] DARK  (SMA=%u < %u) -> LED ON\n", millis(), avg, TH_DARK);
    }
    else if (ledOn && avg > TH_LIGHT)
    {
        ledOn = false;
        Serial.printf("[%6lu ms] LIGHT (SMA=%u > %u) -> LED OFF\n", millis(), avg, TH_LIGHT);
    }
    // Між TH_DARK і TH_LIGHT стан не змінюється, це прибирає мерехтіння
    digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
}

// Setup
void setup()
{
    Serial.begin(115200);
    delay(300);

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    adcInit();
    rawValue = adcReadOnce();
    sma.reset(rawValue);
    smaValue = rawValue;
    updateLed(smaValue);

    Serial.println("=== LDR + SMA + hysteresis ===");
    Serial.printf("Window=%u x %lu ms, ON below %u, OFF above %u\n",
                  SMA_WINDOW, SAMPLE_MS, TH_DARK, TH_LIGHT);
}

// Superloop
void loop()
{
    const uint32_t now = millis();

    if (now - lastSample >= SAMPLE_MS)
    {
        lastSample = now;
        rawValue = adcReadOnce();
        smaValue = sma.update(rawValue);
        updateLed(smaValue);
    }

    if (now - lastPrint >= PRINT_MS)
    {
        lastPrint = now;

        Serial.printf("RAW:%u\tSMA:%u\tTH_DARK:%u\tTH_LIGHT:%u\tLED:%u\n",
                      rawValue, smaValue, TH_DARK, TH_LIGHT, ledOn ? 1000 : 0);
    }
}
