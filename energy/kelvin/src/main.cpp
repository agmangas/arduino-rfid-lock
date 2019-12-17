#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Automaton.h>

/**
 * Energy LED strip.
 */

const uint8_t LED_ENERGY_BRIGHTNESS = 200;
const uint16_t LED_ENERGY_NUM = 30;
const uint16_t LED_ENERGY_PIN = 2;
const uint16_t LED_ENERGY_BLOB_SIZE = 10;

Adafruit_NeoPixel ledEnergy = Adafruit_NeoPixel(
    LED_ENERGY_NUM, LED_ENERGY_PIN, NEO_GRB + NEO_KHZ800);

const uint16_t LED_ENERGY_SURFACE_SEGMENT_INI = 10;
const uint16_t LED_ENERGY_SURFACE_SEGMENT_END = 20;

const uint32_t COLOR_COLD = Adafruit_NeoPixel::gamma32(
    Adafruit_NeoPixel::Color(0, 0, 255));

const uint32_t COLOR_HOT = Adafruit_NeoPixel::gamma32(
    Adafruit_NeoPixel::Color(255, 0, 0));

const uint16_t LED_ENERGY_MODULO_SLOW = 10;
const uint16_t LED_ENERGY_MODULO_MEDIUM = 5;
const uint16_t LED_ENERGY_MODULO_FAST = 2;

const uint16_t LED_ENERGY_HIDDEN_PATCH_SIZE = 20;

Atm_timer timerLedEnergy;

const int LED_ENERGY_TIMER_MS = 20;

/**
 * Indicator LED strips.
 */

const uint8_t SIZE_LED_INDICATOR = 4;
const uint8_t LED_INDICATOR_BRIGHTNESS = 150;

const uint16_t LED_INDICATOR_NUM[SIZE_LED_INDICATOR] = {
    10, 10, 10, 10
};

const uint16_t LED_INDICATOR_PIN[SIZE_LED_INDICATOR] = {
    3, 4, 5, 6
};

Adafruit_NeoPixel ledIndicators[SIZE_LED_INDICATOR] = {
    Adafruit_NeoPixel(LED_INDICATOR_NUM[0], LED_INDICATOR_PIN[0],
        NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(LED_INDICATOR_NUM[1], LED_INDICATOR_PIN[1],
        NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(LED_INDICATOR_NUM[2], LED_INDICATOR_PIN[2],
        NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(LED_INDICATOR_NUM[3], LED_INDICATOR_PIN[3],
        NEO_GRB + NEO_KHZ800)
};

const uint8_t SIZE_COLORS_INDICATOR = 5;

const uint32_t COLORS_INDICATOR[SIZE_COLORS_INDICATOR] = {
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(255, 0, 0)),
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(0, 255, 0)),
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(0, 0, 255)),
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(255, 0, 255)),
    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(0, 255, 255))
};

const uint8_t COLORS_INDICATOR_KEY[SIZE_LED_INDICATOR] = {
    0, 1, 2, 3
};

Atm_timer timerLedIndicator;

const int LED_INDICATOR_TIMER_MS = 200;

/**
 * Indicator buttons.
 */

const uint8_t BTN_INDICATOR_NUM = SIZE_LED_INDICATOR;

const uint8_t BTN_INDICATOR_PINS[BTN_INDICATOR_NUM] = {
    7, 8, 9, 10
};

Atm_button btnsIndicator[BTN_INDICATOR_NUM];

/**
 * Energy buttons.
 */

const uint8_t BTN_ENERGY_NUM = 3;

const uint8_t BTN_ENERGY_PINS[BTN_ENERGY_NUM] = {
    11, 12, A0
};

Atm_button btnsEnergy[BTN_ENERGY_NUM];

/**
 * Program state.
 */

uint8_t ledIndicatorColorIdx[SIZE_LED_INDICATOR];

typedef struct programState {
    uint16_t ledEnergyPivot;
    uint16_t ledEnergyLoop;
    uint16_t ledEnergyTickCounter;
    uint16_t ledEnergyHiddenCountdown;
    uint8_t* ledIndicatorColorIdx;
    bool indicatorOk;
} ProgramState;

ProgramState progState;

void initState()
{
    progState.ledEnergyPivot = 0;
    progState.ledEnergyLoop = 0;
    progState.ledEnergyTickCounter = 0;
    progState.ledEnergyHiddenCountdown = 0;
    progState.ledIndicatorColorIdx = ledIndicatorColorIdx;
    progState.indicatorOk = false;

    for (int i = 0; i < SIZE_LED_INDICATOR; i++) {
        ledIndicatorColorIdx[i] = 0;
    }
}

/**
 * Energy functions.
 */

void initLedEnergy()
{
    ledEnergy.begin();
    ledEnergy.setBrightness(LED_ENERGY_BRIGHTNESS);
    ledEnergy.clear();
    ledEnergy.show();
}

uint32_t getEnergyPixelColor(uint16_t idx)
{
    return idx >= LED_ENERGY_SURFACE_SEGMENT_INI && idx <= LED_ENERGY_SURFACE_SEGMENT_END
        ? COLOR_COLD
        : COLOR_HOT;
}

uint16_t getCurrentEnergyTickModulo() { return LED_ENERGY_MODULO_SLOW; }

void onEnergyCatch() { Serial.println(F("Catch")); }

void onEnergyPress(int idx, int v, int up)
{
    Serial.print(F("BE :: "));
    Serial.println(idx);

    if (progState.ledEnergyHiddenCountdown > 0) {
        onEnergyCatch();
    }
}

void initEnergyButtons()
{
    for (int i = 0; i < BTN_ENERGY_NUM; i++) {
        btnsEnergy[i].begin(BTN_ENERGY_PINS[i]).onPress(onEnergyPress, i);
    }
}

void ledEnergyRefreshTick()
{
    uint16_t mod = getCurrentEnergyTickModulo();
    progState.ledEnergyTickCounter++;
    progState.ledEnergyTickCounter = progState.ledEnergyTickCounter % mod;
}

bool shouldRefreshLedEnergy() { return progState.ledEnergyTickCounter == 0; }

void refreshLedEnergy()
{
    ledEnergy.clear();

    if (progState.ledEnergyHiddenCountdown > 0) {
        progState.ledEnergyHiddenCountdown--;
        ledEnergy.show();
        return;
    }

    uint16_t numPix = ledEnergy.numPixels();

    uint16_t ledIni = progState.ledEnergyPivot;
    uint16_t ledEnd = ledIni + LED_ENERGY_BLOB_SIZE - 1;
    ledEnd = ledEnd >= numPix ? numPix - 1 : ledEnd;

    for (uint16_t i = ledIni; i <= ledEnd; i++) {
        ledEnergy.setPixelColor(i, getEnergyPixelColor(i));
    }

    progState.ledEnergyPivot++;

    if (progState.ledEnergyPivot >= numPix) {
        progState.ledEnergyPivot = 0;
        progState.ledEnergyLoop++;
        progState.ledEnergyHiddenCountdown = LED_ENERGY_HIDDEN_PATCH_SIZE;
    }

    ledEnergy.show();
}

void onLedEnergyTimer(int idx, int v, int up)
{
    if (!progState.indicatorOk) {
        ledEnergy.clear();
        ledEnergy.show();
        return;
    }

    if (shouldRefreshLedEnergy()) {
        refreshLedEnergy();
    }

    ledEnergyRefreshTick();
}

void initLedEnergyTimer()
{
    timerLedEnergy.begin(LED_ENERGY_TIMER_MS)
        .repeat(-1)
        .onTimer(onLedEnergyTimer)
        .start();
}

/**
 * Indicator functions.
 */

void initLedIndicators()
{
    for (int i = 0; i < SIZE_LED_INDICATOR; i++) {
        ledIndicators[i].begin();
        ledIndicators[i].setBrightness(LED_INDICATOR_BRIGHTNESS);
        ledIndicators[i].clear();
        ledIndicators[i].show();
    }
}

void refreshLedIndicators()
{
    for (int i = 0; i < SIZE_LED_INDICATOR; i++) {
        ledIndicators[i].clear();
        ledIndicators[i].fill(COLORS_INDICATOR[progState.ledIndicatorColorIdx[i]]);
        ledIndicators[i].show();
    }
}

bool isIndicatorCorrect()
{
    for (int i = 0; i < SIZE_LED_INDICATOR; i++) {
        if (COLORS_INDICATOR_KEY[i] != progState.ledIndicatorColorIdx[i]) {
            return false;
        }
    }

    return true;
}

void onIndicatorCorrect() { Serial.println(F("Indicator OK")); }

void onLedIndicatorTimer(int idx, int v, int up)
{
    refreshLedIndicators();

    if (progState.indicatorOk == false && isIndicatorCorrect() == true) {
        onIndicatorCorrect();
        progState.indicatorOk = true;
    }
}

void initLedIndicatorTimer()
{
    timerLedIndicator.begin(LED_INDICATOR_TIMER_MS)
        .repeat(-1)
        .onTimer(onLedIndicatorTimer)
        .start();
}

void onIndicatorPress(int idx, int v, int up)
{
    Serial.print(F("BI :: "));
    Serial.println(idx);

    progState.ledIndicatorColorIdx[idx]++;

    progState.ledIndicatorColorIdx[idx] = progState.ledIndicatorColorIdx[idx] % SIZE_COLORS_INDICATOR;
}

void initIndicatorButtons()
{
    for (int i = 0; i < BTN_INDICATOR_NUM; i++) {
        btnsIndicator[i].begin(BTN_INDICATOR_PINS[i]).onPress(onIndicatorPress, i);
    }
}

/**
 * Entrypoint.
 */

void setup()
{
    Serial.begin(9600);

    initState();
    initLedEnergy();
    initLedIndicators();
    initIndicatorButtons();
    initLedIndicatorTimer();
    initEnergyButtons();

    Serial.println(F(">> Starting Kelvin program"));
}

void loop() { automaton.run(); }