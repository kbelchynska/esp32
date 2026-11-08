#include <Arduino.h>

// Модуль 6.1. RTOS introduction
// ДЗ з уроку 2.3 (3 незалежні світлодіоди), але замість суперлупа
// кожен світлодіод блимає у власній задачі FreeRTOS.
//
// Схема: LED1 -> GPIO4, LED2 -> GPIO5, LED3 -> GPIO6 (через резистор 220-330 Ohm на GND)

constexpr uint8_t LED1_PIN = 4;
constexpr uint8_t LED2_PIN = 5;
constexpr uint8_t LED3_PIN = 6;

constexpr uint32_t TASK_STACK = 2048; // в Arduino-ESP32 розмір стеку задається в байтах
constexpr UBaseType_t TASK_PRIO = 1;

// Параметри задачі: кожна задача отримує вказівник на свою структуру
struct BlinkParams
{
    const char *name;
    uint8_t pin;
    uint32_t intervalMs;
    BaseType_t core;
};

// static: структури мають жити весь час роботи задач, бо задачі тримають вказівник на них
static BlinkParams led1 = {"LED1", LED1_PIN, 200, 0};  // PRO_CPU
static BlinkParams led2 = {"LED2", LED2_PIN, 500, 1};  // APP_CPU
static BlinkParams led3 = {"LED3", LED3_PIN, 1000, 1}; // APP_CPU

// Одна функція задачі для всіх світлодіодів
static void blinkTask(void *arg)
{
    const BlinkParams *p = static_cast<const BlinkParams *>(arg);

    pinMode(p->pin, OUTPUT);
    digitalWrite(p->pin, LOW);
    bool state = false;

    Serial.printf("[%s] started on core %d, GPIO%u, period %lu ms\n",
                  p->name, xPortGetCoreID(), p->pin, (unsigned long)p->intervalMs);

    const TickType_t period = pdMS_TO_TICKS(p->intervalMs);
    TickType_t lastWake = xTaskGetTickCount();

    for (;;)
    {
        state = !state;
        digitalWrite(p->pin, state ? HIGH : LOW);

        // vTaskDelayUntil тримає рівний період незалежно від часу виконання тіла циклу.
        // Під час очікування задача заблокована і не займає процесор (на відміну від суперлупа,
        // який постійно опитує millis()).
        vTaskDelayUntil(&lastWake, period);
    }
}

static void startBlinkTask(BlinkParams &p)
{
    const BaseType_t ok = xTaskCreatePinnedToCore(
        blinkTask,  // функція задачі
        p.name,     // ім'я (для дебагу)
        TASK_STACK, // розмір стеку
        &p,         // параметр задачі
        TASK_PRIO,  // пріоритет
        nullptr,    // handle не потрібен
        p.core);    // ядро

    if (ok != pdPASS)
        Serial.printf("Failed to create task %s\n", p.name);
}

void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.println("=== RTOS: 3 independent LED blink tasks ===");

    startBlinkTask(led1);
    startBlinkTask(led2);
    startBlinkTask(led3);
}

void loop()
{
    // Уся логіка в задачах. Видаляємо loopTask, щоб він не займав процесор.
    vTaskDelete(nullptr);
}
