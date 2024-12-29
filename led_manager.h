#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <FastLED.h>
#include "config.h"

class LedManager {
private:
    CRGB leds[NUM_LEDS];
    LedEffect currentEffect;
    uint8_t brightness;
    bool isOn;
    unsigned long lastUpdate;
    
    // Variables para efectos
    uint8_t hue = 0;
    uint8_t saturation = 255;  // Nueva variable para saturación
    uint8_t breathVal = 0;
    bool breathingUp = true;

    void updateBreathing() {
        if (breathingUp) {
            breathVal += 2;
            if (breathVal >= 252) breathingUp = false;
        } else {
            breathVal -= 2;
            if (breathVal <= 0) breathingUp = true;
        }
        
        fill_solid(leds, NUM_LEDS, CHSV(hue, saturation, breathVal));
    }

    void updateRainbow() {
        fill_rainbow(leds, NUM_LEDS, hue++, 7);
    }

    void updateFire() {
        // Efecto de fuego simplificado
        int r = 255;
        int g = 100 + random(155);
        int b = random(50);
        leds[0] = CRGB(r, g, b);
    }

public:
    LedManager() : currentEffect(OFF), brightness(MAX_BRIGHTNESS), isOn(false), lastUpdate(0) {
        FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
        FastLED.setBrightness(brightness);
    }

    void begin() {
        FastLED.clear();
        FastLED.show();
    }

    void handle() {
        if (!isOn) {
            FastLED.clear();
            FastLED.show();
            return;
        }

        unsigned long currentMillis = millis();
        if (currentMillis - lastUpdate >= 20) { // Actualizar cada 20ms
            lastUpdate = currentMillis;

            switch (currentEffect) {
                case BREATHING:
                    updateBreathing();
                    break;
                case RAINBOW:
                    updateRainbow();
                    break;
                case FIRE:
                    updateFire();
                    break;
                case SOLID:
                    fill_solid(leds, NUM_LEDS, CHSV(hue, saturation, 255));
                    break;
                case OFF:
                    FastLED.clear();
                    break;
            }

            FastLED.setBrightness(brightness);
            FastLED.show();
        }
    }

    void setBrightness(uint8_t newBrightness) {
        brightness = newBrightness;
        FastLED.setBrightness(brightness);
        FastLED.show();
    }

    void setEffect(LedEffect effect) {
        currentEffect = effect;
        if (effect == OFF) {
            isOn = false;
        } else {
            isOn = true;
        }
    }

    void setState(bool state) {
        isOn = state;
        if (!isOn) {
            FastLED.clear();
            FastLED.show();
        }
    }

    void setHue(uint8_t newHue) {
        hue = newHue;
    }

    void setSaturation(uint8_t newSaturation) {  // Nuevo método
        saturation = newSaturation;
    }

    bool getState() const {
        return isOn;
    }

    uint8_t getBrightness() const {
        return brightness;
    }

    uint8_t getHue() const {
        return hue;
    }

    uint8_t getSaturation() const {  // Nuevo método
        return saturation;
    }

    LedEffect getCurrentEffect() const {
        return currentEffect;
    }
};

#endif