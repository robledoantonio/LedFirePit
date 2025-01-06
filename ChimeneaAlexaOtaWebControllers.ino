#include "config.h"
#include "ota_manager.h"
#include "alexa_manager.h"
#include "led_manager.h"
#include "web_manager.h"


OTAManager otaManager;
LedManager ledManager;
AlexaManager alexaManager(&ledManager);
WebManager webManager(&ledManager);

void setup() {
    Serial.begin(115200);

    // Inicializar WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    otaManager.begin();
    ledManager.begin();
    webManager.begin();
    alexaManager.begin();
    
    ledManager.setEffect(FIRE);
}

void loop() {
    otaManager.handle();
    alexaManager.handle();
    ledManager.handle();
}