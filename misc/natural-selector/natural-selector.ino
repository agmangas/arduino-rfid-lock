#include <Adafruit_NeoPixel.h>
#include <Automaton.h>

/**
 * Relays.
 */

const int PIN_RELAY = 12;

/**
 * Buttons.
 */

const int DSWITCH_NUM_OUTPUTS = 6;
const int DSWITCH_DEBOUNCE_MS = 1000;
const int DSWITCH_PINS[DSWITCH_NUM_OUTPUTS] = {3, 4, 5, 6, 7, 8};
const int DSWITCH_VALID_IDX = 3;

const int BUTTONS_NUM = 3;
const int BUTTONS_DEBOUNCE_MS = 1000;
const int BUTTONS_PINS[BUTTONS_NUM] = {9, 10, 11};

Atm_button dswitchButtons[DSWITCH_NUM_OUTPUTS];
Atm_controller dswitchControl;
Atm_button buttons[BUTTONS_NUM];
Atm_controller buttonsControl;
Atm_controller validControl;

/**
 * LED strip.
 */

const uint8_t LEDS_PIN = 2;
const int LEDS_BRIGHTNESS = 150;

Adafruit_NeoPixel pixelStrip = Adafruit_NeoPixel(DSWITCH_NUM_OUTPUTS, LEDS_PIN, NEO_GRB + NEO_KHZ800);

/**
 * Button functions.
 */

bool dswitchAndButtonsOk()
{
    return allButtonsPressed() && isValidDswitch();
}

void onDswitchAndButtonsOk()
{
    Serial.println(F("Dswitch and buttons OK"));
}

int getDswitchPressedIdx()
{
    for (int i = 0; i < DSWITCH_NUM_OUTPUTS; i++)
    {
        if (dswitchButtons[i].state() == Atm_button::PRESSED)
        {
            return i;
        }
    }

    return -1;
}

bool isValidDswitch()
{
    return getDswitchPressedIdx() == DSWITCH_VALID_IDX;
}

void onValidDswitch()
{
    Serial.println(F("Valid dswitch"));
}

void initDswitch()
{
    for (int i = 0; i < DSWITCH_NUM_OUTPUTS; i++)
    {
        dswitchButtons[i]
            .begin(DSWITCH_PINS[i])
            .debounce(DSWITCH_DEBOUNCE_MS)
            .onPress(refreshLeds)
            .onRelease(refreshLeds);
    }

    dswitchControl
        .begin()
        .IF(isValidDswitch)
        .onChange(true, onValidDswitch);

    validControl
        .begin(dswitchAndButtonsOk)
        .IF(onDswitchAndButtonsOk);
}

bool allButtonsPressed()
{
    for (int i = 0; i < BUTTONS_NUM; i++)
    {
        if (buttons[i].state() != Atm_button::PRESSED)
        {
            return false;
        }
    }

    return true;
}

void onButtonsPressed()
{
    Serial.println(F("All buttons pressed"));
}

void initButtons()
{
    for (int i = 0; i < BUTTONS_NUM; i++)
    {
        buttons[i]
            .begin(BUTTONS_PINS[i])
            .debounce(BUTTONS_DEBOUNCE_MS);
    }

    buttonsControl
        .begin()
        .IF(allButtonsPressed)
        .onChange(true, onButtonsPressed);
}

/**
 * Relay functions.
 */

void lockRelay()
{
    if (digitalRead(PIN_RELAY) == HIGH)
    {
        Serial.println(F("Relay:Lock"));
    }

    digitalWrite(PIN_RELAY, LOW);
}

void openRelay()
{
    if (digitalRead(PIN_RELAY) == LOW)
    {
        Serial.println(F("Relay:Open"));
    }

    digitalWrite(PIN_RELAY, HIGH);
}

void initRelays()
{
    pinMode(PIN_RELAY, OUTPUT);
    lockRelay();
}

/**
 * LED functions.
 */

uint32_t randomColor()
{
    return Adafruit_NeoPixel::Color(random(100, 250), 0, 0);
}

void refreshLeds()
{
    for (int i = 0; i < DSWITCH_NUM_OUTPUTS; i++)
    {
        pixelStrip.setPixelColor(i, 0);
    }

    for (int i = 0; i < DSWITCH_NUM_OUTPUTS; i++)
    {
        if (dswitchButtons[i].state() == Atm_button::PRESSED)
        {
            pixelStrip.setPixelColor(i, randomColor());
            break;
        }
    }

    pixelStrip.show();
}

void initLeds()
{
    pixelStrip.begin();
    pixelStrip.setBrightness(LEDS_BRIGHTNESS);
    pixelStrip.show();

    clearLeds();
}

void clearLeds()
{
    pixelStrip.clear();
    pixelStrip.show();
}

/**
 * Entrypoint.
 */

void setup()
{
    initRelays();
    initLeds();
    initDswitch();
    initButtons();
}

void loop()
{
    automaton.run();
}