#ifndef ALEXA_MANAGER_H
#define ALEXA_MANAGER_H

#include <Espalexa.h>
#include "config.h"
#include "led_manager.h"

class AlexaManager {
private:
    Espalexa espalexa;
    LedManager* ledManager;
    bool deviceState;
    int brightness;

    static void mainDeviceChanged(uint8_t brightness) {
        extern LedManager ledManager;
        
        if (brightness) {
            Serial.print("Alexa encendió el dispositivo. Brillo: ");
            Serial.println(brightness);
            ledManager.setState(true);
            ledManager.setBrightness(brightness);
        }
        else {
            Serial.println("Alexa apagó el dispositivo");
            ledManager.setState(false);
        }
    }

public:
    AlexaManager(LedManager* ledMgr) 
        : deviceState(false), brightness(0), ledManager(ledMgr) {}

    void begin() {
        espalexa.addDevice(ALEXA_DEVICE_NAME, mainDeviceChanged);
        espalexa.begin();
    }

    void handle() {
        espalexa.loop();
    }

    void setState(bool state, uint8_t bright = 255) {
        deviceState = state;
        brightness = bright;
        ledManager->setState(state);
        ledManager->setBrightness(bright);
    }

    bool getState() const {
        return deviceState;
    }

    int getBrightness() const {
        return brightness;
    }
};

#endif
