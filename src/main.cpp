#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

// Схема: 3V3 - потенціометр 10 kOhm - GND, середній вивід на GPIO4 (ADC1_CH3)

// Pins / ADC
constexpr adc1_channel_t   POT_CHANNEL = ADC1_CHANNEL_3;   // GPIO4 на ESP32-S3
constexpr adc_bits_width_t ADC_WIDTH   = ADC_WIDTH_BIT_12; // розрядність 12 біт
constexpr adc_atten_t      ADC_ATTEN   = ADC_ATTEN_DB_12;  // атенюація 12 dB (колишня 11 dB), діапазон приблизно 0..3100 mV

// Параметри для ручної формули U = RAW * VREF / RAW_MAX
constexpr uint16_t RAW_MAX      = 4095;    // 2^12 - 1
constexpr float    VREF_MV      = 3300.0f; // опорна напруга для ручного розрахунку (живлення 3V3)
constexpr uint32_t DEFAULT_VREF = 1100;    // використовується, лише якщо в eFuse немає калібрування

constexpr uint32_t SAMPLE_MS    = 100;
constexpr uint8_t  HEADER_EVERY = 20;      // повторювати заголовок таблиці кожні N рядків

esp_adc_cal_characteristics_t adcChars;
uint32_t lastSample = 0;
uint8_t  rowCount   = 0;

static const char* calTypeName(esp_adc_cal_value_t t) {
    switch (t) {
        case ESP_ADC_CAL_VAL_EFUSE_VREF:   return "eFuse Vref";
        case ESP_ADC_CAL_VAL_EFUSE_TP:     return "eFuse Two Point";
        case ESP_ADC_CAL_VAL_EFUSE_TP_FIT: return "eFuse Two Point + fitting";
        case ESP_ADC_CAL_VAL_DEFAULT_VREF: return "Default Vref";
        default:                           return "?";
    }
}

static void printHeader() {
    Serial.println();
    Serial.println(" RAW   U_manual(mV)   U_cali(mV)   Error(%)");
    Serial.println("------------------------------------------");
}

void setup() {
    Serial.begin(115200);
    delay(300);

    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(POT_CHANNEL, ADC_ATTEN);

    // Калібрування на основі даних, записаних в eFuse на заводі
    esp_adc_cal_value_t calType =
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, DEFAULT_VREF, &adcChars);

    Serial.println("=== ADC calibration ===");
    Serial.println("Channel     : ADC1_CH3 (GPIO4)");
    Serial.println("Resolution  : 12 bit (0..4095)");
    Serial.println("Attenuation : 12 dB (approx. 0..3100 mV)");
    Serial.printf ("Vref manual : %.0f mV\n", VREF_MV);
    Serial.printf ("Calibration : %s\n", calTypeName(calType));
    printHeader();
}

void loop() {
    const uint32_t now = millis();
    if (now - lastSample < SAMPLE_MS) return;
    lastSample = now;

    const int      raw     = adc1_get_raw(POT_CHANNEL);
    const float    uManual = raw * VREF_MV / RAW_MAX;
    const uint32_t uCali   = esp_adc_cal_raw_to_voltage(raw, &adcChars);

    // Похибка відносно каліброваного значення, при 0 mV ділити не можна
    const float errorPct = uCali > 0 ? fabsf(uManual - uCali) / uCali * 100.0f : 0.0f;

    if (rowCount == HEADER_EVERY) {
        printHeader();
        rowCount = 0;
    }
    rowCount++;

    Serial.printf("%4d   %12.1f   %10lu   %8.2f\n", raw, uManual, uCali, errorPct);
}
