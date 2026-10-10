#include <Arduino.h>

// Pins
constexpr uint8_t PIN_BUZZER = 8;
constexpr uint8_t PIN_LED = 7;
constexpr uint8_t PIN_BUTTON = 0;

// PWM
constexpr uint8_t BUZZER_CH = 0;
constexpr uint8_t BUZZER_BITS = 10;

// Часовий тік плеєра
constexpr uint32_t TICK_MS = 50;

constexpr uint32_t BLINK_MS = 300;
constexpr uint32_t DEBOUNCE_MS = 50;

// Частоти нот 4-ї та 5-ї октави, Hz
constexpr uint16_t REST = 0;
constexpr uint16_t NOTE_C4 = 262;
constexpr uint16_t NOTE_D4 = 294;
constexpr uint16_t NOTE_E4 = 330;
constexpr uint16_t NOTE_F4 = 349;
constexpr uint16_t NOTE_G4 = 392;
constexpr uint16_t NOTE_A4 = 440;
constexpr uint16_t NOTE_B4 = 494;
constexpr uint16_t NOTE_C5 = 523;

// Нота: частота і тривалість у тіках
struct Note
{
    uint16_t freq;
    uint8_t ticks;
};

// Тривалості в тіках по 50 ms
constexpr uint8_t DUR_4 = 6;  // чверть, 300 ms
constexpr uint8_t DUR_2 = 12; // половина, 600 ms
constexpr uint8_t DUR_1 = 24; // ціла, 1200 ms

// Jingle Bells: E E E | E E E | E G C D E
const Note JINGLE_BELLS[] = {
    {NOTE_E4, DUR_4},
    {NOTE_E4, DUR_4},
    {NOTE_E4, DUR_2},
    {NOTE_E4, DUR_4},
    {NOTE_E4, DUR_4},
    {NOTE_E4, DUR_2},
    {NOTE_E4, DUR_4},
    {NOTE_G4, DUR_4},
    {NOTE_C4, DUR_4},
    {NOTE_D4, DUR_4},
    {NOTE_E4, DUR_1},
    {REST, DUR_2},
};

// стан змінюється лише в tick(), який викликається раз на TICK_MS
class BuzzerPlayer
{
public:
    void begin(uint8_t pin, uint8_t channel)
    {
        channel_ = channel;
        ledcSetup(channel_, 1000, BUZZER_BITS);
        ledcAttachPin(pin, channel_);
        silence();
    }

    void play(const Note *melody, size_t length, bool loop)
    {
        melody_ = melody;
        length_ = length;
        loop_ = loop;
        index_ = 0;
        playing_ = true;
        startNote();
    }

    void stop()
    {
        playing_ = false;
        silence();
        Serial.println("Player: stop");
    }

    bool isPlaying() const { return playing_; }

    void tick()
    {
        if (!playing_)
            return;

        ticksLeft_--;

        // Останній тік ноти беззвучний, щоб однакові ноти підряд не зливались
        if (ticksLeft_ == 1)
            silence();

        if (ticksLeft_ > 0)
            return;

        index_++;
        if (index_ >= length_)
        {
            if (!loop_)
            {
                stop();
                return;
            }
            index_ = 0;
        }
        startNote();
    }

private:
    void startNote()
    {
        const Note &n = melody_[index_];
        ticksLeft_ = n.ticks;
        if (n.freq == REST)
        {
            silence();
        }
        else
        {
            ledcWriteTone(channel_, n.freq); // 50% заповнення на частоті ноти
        }
        Serial.printf("[%6lu ms] note %2u: %3u Hz, %2u ticks\n",
                      millis(), static_cast<unsigned>(index_), n.freq, n.ticks);
    }

    void silence() { ledcWrite(channel_, 0); }

    const Note *melody_ = nullptr;
    size_t length_ = 0;
    size_t index_ = 0;
    uint8_t ticksLeft_ = 0;
    uint8_t channel_ = 0;
    bool loop_ = false;
    bool playing_ = false;
};

BuzzerPlayer player;

uint32_t lastTick = 0;
uint32_t lastBlink = 0;
bool ledOn = false;

bool lastButtonRead = HIGH;
bool buttonState = HIGH;
uint32_t lastButtonChange = 0;

static void startMelody()
{
    Serial.println("Player: Jingle Bells");
    player.play(JINGLE_BELLS, sizeof(JINGLE_BELLS) / sizeof(JINGLE_BELLS[0]), true);
}

// Повертає true один раз на кожне натискання
static bool buttonPressed(uint32_t now)
{
    const bool reading = digitalRead(PIN_BUTTON);
    if (reading != lastButtonRead)
    {
        lastButtonRead = reading;
        lastButtonChange = now;
    }
    if (now - lastButtonChange >= DEBOUNCE_MS && reading != buttonState)
    {
        buttonState = reading;
        return buttonState == LOW;
    }
    return false;
}

void setup()
{
    Serial.begin(115200);
    delay(300);

    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    player.begin(PIN_BUZZER, BUZZER_CH);

    Serial.println("=== Non-blocking buzzer player ===");
    Serial.printf("Tick: %lu ms, BOOT button: play / stop\n", TICK_MS);

    lastTick = millis();
    startMelody();
}

void loop()
{
    const uint32_t now = millis();

    // Фіксований тік: += TICK_MS замість = now, щоб похибка не накопичувалась
    if (now - lastTick >= TICK_MS)
    {
        lastTick += TICK_MS;
        player.tick();
    }

    // Ця робота виконується паралельно з мелодією
    if (now - lastBlink >= BLINK_MS)
    {
        lastBlink = now;
        ledOn = !ledOn;
        digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
    }

    if (buttonPressed(now))
    {
        if (player.isPlaying())
            player.stop();
        else
            startMelody();
    }
}
