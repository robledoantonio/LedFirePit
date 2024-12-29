#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H	

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "config.h"

class OTAManager {
private:
    DeviceState currentState;
    unsigned long previousWifiCheck;
    unsigned long lastInfoPrint;
    bool isWiFiConnected;

    void setupOTA() {
        ArduinoOTA.setPort(OTA_PORT);
        ArduinoOTA.setHostname(OTA_HOSTNAME);
        ArduinoOTA.setPassword(OTA_PASSWORD);

        ArduinoOTA.onStart([]() {
            Serial.println(SystemMessages::OTA_START);
        });
        
        ArduinoOTA.onEnd([]() {
            Serial.println(SystemMessages::OTA_COMPLETE);
        });
        
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progreso: %u%%\r", (progress / (total / 100)));
        });
        
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            switch (error) {
                case OTA_AUTH_ERROR: Serial.println(SystemMessages::OTA_AUTH_ERROR); break;
                case OTA_BEGIN_ERROR: Serial.println(SystemMessages::OTA_BEGIN_ERROR); break;
                case OTA_CONNECT_ERROR: Serial.println(SystemMessages::OTA_CONNECT_ERROR); break;
                case OTA_RECEIVE_ERROR: Serial.println(SystemMessages::OTA_RECEIVE_ERROR); break;
                case OTA_END_ERROR: Serial.println(SystemMessages::OTA_END_ERROR); break;
                default: Serial.println(SystemMessages::OTA_UNKNOWN_ERROR); break;
            }
        });

        ArduinoOTA.begin();
        Serial.println(SystemMessages::OTA_INITIALIZED);
    }

    void checkWiFiConnection() {
        if (WiFi.status() != WL_CONNECTED) {
            if (isWiFiConnected) {
                Serial.println(SystemMessages::WIFI_LOST);
                isWiFiConnected = false;
            }
            
            Serial.println(SystemMessages::WIFI_RECONNECTING);
            WiFi.begin(ssid, password);
            
            int retries = 0;
            while (WiFi.status() != WL_CONNECTED && retries < MAX_WIFI_RETRIES) {
                delay(WIFI_RETRY_DELAY);
                retries++;
                Serial.print(".");
            }
            Serial.println();
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println(SystemMessages::WIFI_RECONNECTED);
                Serial.printf(SystemInfo::IP_FORMAT, WiFi.localIP().toString().c_str());
                isWiFiConnected = true;
            }
        }
    }

    void printSystemInfo() {
        Serial.println(SystemInfo::HEADER);
        Serial.printf(SystemInfo::STATE_FORMAT, currentState);
        Serial.printf(SystemInfo::SSID_FORMAT, ssid);
        Serial.printf(SystemInfo::HOSTNAME_FORMAT, OTA_HOSTNAME);
        Serial.printf(SystemInfo::IP_FORMAT, WiFi.localIP().toString().c_str());
        Serial.printf(SystemInfo::MAC_FORMAT, WiFi.macAddress().c_str());
        Serial.printf(SystemInfo::RSSI_FORMAT, WiFi.RSSI());
        Serial.printf(SystemInfo::UPTIME_FORMAT, millis() / 1000);
        Serial.println(SystemInfo::FOOTER);
    }

public:
    OTAManager() : currentState(INITIALIZING), previousWifiCheck(0), lastInfoPrint(0), isWiFiConnected(false) {}

    void begin() {
        Serial.begin(SERIAL_BAUD_RATE);
        Serial.println(SystemMessages::STARTING);
        
        // Configuración WiFi
        currentState = CONNECTING_WIFI;
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        
        Serial.println(SystemMessages::WIFI_CONNECTING);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            isWiFiConnected = true;
            Serial.println(SystemMessages::WIFI_CONNECTED);
            Serial.printf(SystemInfo::IP_FORMAT, WiFi.localIP().toString().c_str());
        }
        
        setupOTA();
        currentState = RUNNING;
        printSystemInfo();
    }

    void handle() {
        ArduinoOTA.handle();
        
        unsigned long currentMillis = millis();
        
        // Verificar conexión WiFi
        if (currentMillis - previousWifiCheck >= WIFI_CHECK_INTERVAL) {
            previousWifiCheck = currentMillis;
            checkWiFiConnection();
        }
        
        // Imprimir información del sistema
        if (currentMillis - lastInfoPrint >= SYSTEM_INFO_INTERVAL) {
            lastInfoPrint = currentMillis;
            printSystemInfo();
        }
        
        // Manejar errores
        if (currentState == ERROR) {
            Serial.println(SystemMessages::ERROR_RECOVERY);
            delay(ERROR_RETRY_DELAY);
            ESP.restart();
        }
    }
};

#endif