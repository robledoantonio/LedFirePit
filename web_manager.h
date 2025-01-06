#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "led_manager.h"

class WebManager {
private:
    AsyncWebServer server;
    LedManager* ledManager;
    String cachedVerse;
    String cachedReference;
    unsigned long lastVerseUpdate = 0;
    const unsigned long VERSE_UPDATE_INTERVAL = 3600000; // 1 hora en milisegundos
    uint32_t stateVersion = 0;  // Contador de cambios de estado

    void incrementStateVersion() {
        stateVersion++;
    }

    uint8_t getBookNumber(String bookName) {
        const char* bookNames[] = {
            "Genesis", "Exodus", "Leviticus", "Numbers", "Deuteronomy",
            "Joshua", "Judges", "Ruth", "1 Samuel", "2 Samuel",
            "1 Kings", "2 Kings", "1 Chronicles", "2 Chronicles",
            "Ezra", "Nehemiah", "Esther", "Job", "Psalms", "Proverbs",
            "Ecclesiastes", "Song of Solomon", "Isaiah", "Jeremiah",
            "Lamentations", "Ezekiel", "Daniel", "Hosea", "Joel", "Amos",
            "Obadiah", "Jonah", "Micah", "Nahum", "Habakkuk", "Zephaniah",
            "Haggai", "Zechariah", "Malachi",
            "Matthew", "Mark", "Luke", "John", "Acts", "Romans",
            "1 Corinthians", "2 Corinthians", "Galatians", "Ephesians",
            "Philippians", "Colossians", "1 Thessalonians", "2 Thessalonians",
            "1 Timothy", "2 Timothy", "Titus", "Philemon", "Hebrews",
            "James", "1 Peter", "2 Peter", "1 John", "2 John", "3 John",
            "Jude", "Revelation"
        };

        for (uint8_t i = 0; i < 66; i++) {
            if (bookName.equals(bookNames[i])) {
                return i + 1;  // Los números de libro empiezan en 1
            }
        }
        return 1;  // Por defecto, retorna 1 (Genesis)
    }

    String getVerse(String& reference) {
        unsigned long currentMillis = millis();
        
        // Si tenemos un versículo cacheado y no ha pasado el intervalo, lo usamos
        if (!cachedVerse.isEmpty() && (currentMillis - lastVerseUpdate < VERSE_UPDATE_INTERVAL)) {
            reference = cachedReference;
            return cachedVerse;
        }

        // Si necesitamos actualizar, hacemos la petición
        String newVerse = fetchDailyVerse(reference);
        if (newVerse != "No data") {
            cachedVerse = newVerse;
            cachedReference = reference;
            lastVerseUpdate = currentMillis;
        } else if (!cachedVerse.isEmpty()) {
            // Si la petición falló pero tenemos datos cacheados, los usamos
            reference = cachedReference;
            return cachedVerse;
        }
        
        return newVerse;
    }

    String fetchDailyVerse(String& reference) {  // Añadimos un parámetro por referencia
        WiFiClientSecure client;
        String result = "No data";
        reference = "Información del Reloj";  // Valor por defecto
        
        client.setInsecure();
        
        if (client.connect("labs.bible.org", 443)) {
            client.println("GET /api/?passage=votd&type=json HTTP/1.1");
            client.println("Host: labs.bible.org");
            client.println("User-Agent: Mozilla/5.0");
            client.println("Accept: application/json");
            client.println("Connection: close");
            client.println();

            while (client.connected()) {
                String line = client.readStringUntil('\n');
                if (line == "\r") {
                    break;
                }
            }

            String response = client.readString();
            int jsonStart = response.indexOf('[');
            if (jsonStart != -1) {
                response = response.substring(jsonStart);
                
                StaticJsonDocument<1024> doc;
                DeserializationError error = deserializeJson(doc, response);
                
                if (!error) {
                    String bookName = doc[0]["bookname"].as<String>();
                    int chapter = doc[0]["chapter"].as<unsigned int>();
                    int verse = doc[0]["verse"].as<unsigned int>();
                    String text = doc[0]["text"].as<String>();

                    ledManager->setBook(getBookNumber(bookName));
                    ledManager->setChapter(chapter);
                    ledManager->setVerse(verse);

                    bookName = translateText(bookName);
                    text = translateText(text);
                    
                    // Guardar la referencia separada
                    reference = bookName + " " + chapter + ":" + verse;
                    // Solo el texto para el contenido
                    result = text;
                }
            }
        }
        
        client.stop();
        return result;
    }

    String translateText(String text) {
        WiFiClient client;
        String translated = text;  // Por defecto retornamos el texto original

        if (client.connect("clients5.google.com", 80)) {
            // Codificar el texto para URL
            text.replace(" ", "%20");
            
            String url = "/translate_a/t?client=dict-chrome-ex&sl=en&tl=es&q=" + text;
            
            client.println("GET " + url + " HTTP/1.1");
            client.println("Host: clients5.google.com");
            client.println("User-Agent: Mozilla/5.0");
            client.println("Connection: close");
            client.println();

            // Esperar la respuesta
            while (client.connected()) {
                String line = client.readStringUntil('\n');
                if (line == "\r") {
                    break;
                }
            }

            String response = client.readString();
            int jsonStart = response.indexOf('[');
            if (jsonStart != -1) {
                response = response.substring(jsonStart);
                
                StaticJsonDocument<1024> doc;
                DeserializationError error = deserializeJson(doc, response);
                
                if (!error) {
                    translated = doc[0].as<String>();
                }
            }
        }
        
        client.stop();
        return translated;
    }
    
    void setupRoutes() {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(200, "text/html", getIndexHTML());
        });

        server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request){
            String response;
            StaticJsonDocument<2048> doc;
            
            // Asegurarnos de que siempre enviamos el efecto actual
            doc["effect"] = static_cast<int>(ledManager->getCurrentEffect());  // Añadir cast explícito
            doc["state"] = ledManager->getState();
            doc["brightness"] = ledManager->getBrightness();
            
            // Añadir el resto de la información según el efecto actual
            LedEffect currentEffect = ledManager->getCurrentEffect();
            
            switch(currentEffect) {
                case SOLID:
                case BREATHING:
                    doc["hue"] = ledManager->getHue();
                    doc["saturation"] = ledManager->getSaturation();
                    break;
                    
                case RAINBOW:
                    doc["saturation"] = ledManager->getSaturation();
                    doc["rainbowType"] = ledManager->getRainbowType();
                    break;
                    
                case FIRE:
                    doc["firePalette"] = ledManager->getFirePalette();
                    break;
                    
                case LIFE:
                    doc["lifePattern"] = ledManager->getCurrentLifePattern();
                    doc["lifeAutoRestart"] = ledManager->getAutoRestart();
                    doc["lifeSpeed"] = ledManager->getLifeSpeed();
                    break;
                    
                case CLOCK:
                    String reference;
                    doc["clockText"] = getVerse(reference);
                    doc["passageReference"] = reference;
                    break;
            }
            
            serializeJson(doc, response);
            Serial.println("Sending status: " + response);  // Debug
            request->send(200, "application/json", response);
        });

        // Endpoint de color para manejar tanto hue como saturación
                server.on("/api/color", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
            [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
                String json = String((char*)data);
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, json);
                
                if (!error) {
                    if (doc.containsKey("hue")) {
                        ledManager->setHue(doc["hue"].as<uint8_t>());
                    }
                    if (doc.containsKey("saturation")) {
                        ledManager->setSaturation(doc["saturation"].as<uint8_t>());
                    }
                }
                request->send(200);
            });

        server.on("/api/state", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
            [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
                String json = String((char*)data);
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, json);
                
                if (!error && doc.containsKey("state")) {
                    ledManager->setState(doc["state"].as<bool>());
                }
                request->send(200);
            });

        server.on("/api/brightness", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
            [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
                String json = String((char*)data);
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, json);
                
                if (!error && doc.containsKey("brightness")) {
                    ledManager->setBrightness(doc["brightness"].as<uint8_t>());
                }
                request->send(200);
            });

        server.on("/api/effect", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
            [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
                String json = String((char*)data);
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, json);
                
                if (!error && doc.containsKey("effect")) {
                    ledManager->setEffect(static_cast<LedEffect>(doc["effect"].as<int>()));
                }
                request->send(200);
            });
        
                server.on("/api/rainbow-type", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
            [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
                String json = String((char*)data);
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, json);
                
                if (!error && doc.containsKey("type")) {
                    String type = doc["type"].as<String>();
                    ledManager->setRainbowType(type);
                    
                    String response;
                    StaticJsonDocument<200> responseDoc;
                    responseDoc["success"] = true;
                    responseDoc["type"] = type;
                    serializeJson(responseDoc, response);
                    
                    request->send(200, "application/json", response);
                } else {
                    request->send(400);
                }
            });
        
        server.on("/api/fire-palette", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
            [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
                String json = String((char*)data);
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, json);
                
                if (!error && doc.containsKey("palette")) {
                    uint8_t palette = doc["palette"].as<uint8_t>();
                    ledManager->setFirePalette(palette);
                    request->send(200);
                } else {
                    request->send(400);
                }
            });

        server.on("/api/life-pattern", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
            [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
                String json = String((char*)data);
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, json);
                
                if (!error && doc.containsKey("pattern")) {
                    uint8_t pattern = doc["pattern"].as<uint8_t>();
                    ledManager->setLifePatternFromWeb(pattern);
                    request->send(200);
                } else {
                    request->send(400);
                }
            });

        server.on("/api/life-auto-restart", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
            [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
                String json = String((char*)data);
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, json);
                
                if (!error && doc.containsKey("enabled")) {
                    bool enabled = doc["enabled"].as<bool>();
                    ledManager->setAutoRestart(enabled);
                    request->send(200);
                } else {
                    request->send(400);
                }
            });

        server.on("/api/life-speed", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
            [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
                String json = String((char*)data);
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, json);
                
                if (!error && doc.containsKey("speed")) {
                    float speed = doc["speed"].as<float>();
                    ledManager->setLifeSpeed(speed);
                    request->send(200);
                } else {
                    request->send(400);
                }
            });

        server.onNotFound([](AsyncWebServerRequest *request){
            request->send(404, "text/plain", "Not found");
        });
    }

        static String getIndexHTML() {
        return R"HTMLCONTENT(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Control LED</title>
    <style>
        :root {
            --primary-color: #4CAF50;
            --danger-color: #f44336;
            --background-color: #f5f5f5;
            --card-background: #ffffff;
            --text-color: #333333;
            --border-radius: 12px;
            --shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
        }

        body {
            font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
            max-width: 600px;
            margin: 0 auto;
            padding: 20px;
            background-color: var(--background-color);
            color: var(--text-color);
        }

        .card {
            background: var(--card-background);
            padding: 24px;
            border-radius: var(--border-radius);
            box-shadow: var(--shadow);
            margin-bottom: 20px;
        }

        h1 {
            margin: 0 0 24px 0;
            font-size: 24px;
            font-weight: 600;
        }

        .control-group {
            margin-bottom: 10px;
        }

        .control-group:last-child {
            margin-bottom: 0;
        }

        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 500;
            font-size: 14px;
            color: #666;
        }

        select {
            width: 100%;
            padding: 12px;
            border: 1px solid #ddd;
            border-radius: 8px;
            font-size: 16px;
            appearance: none;
        }

        input[type="range"] {
            width: 100%;
            margin: 8px 0;
            -webkit-appearance: none;
            background: transparent;
        }

        input[type="range"]::-webkit-slider-runnable-track {
            width: 100%;
            height: 6px;
            background: #ddd;
            border-radius: 3px;
        }

        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            height: 18px;
            width: 18px;
            border-radius: 50%;
            background: var(--primary-color);
            margin-top: -6px;
            cursor: pointer;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }

                button {
            width: 100%;
            padding: 14px;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s ease;
        }

        button:active {
            transform: scale(0.98);
        }

        button.primary {
            background-color: var(--primary-color);
            color: white;
        }

        button.off {
            background-color: var(--danger-color);
            color: white;
        }

        .status {
            margin-top: 16px;
            padding: 12px;
            border-radius: 8px;
            background-color: #f8f9fa;
            font-size: 14px;
            color: #666;
        }

        .color-controls {
            display: none;
            margin-top: 16px;
        }

        .color-controls.visible {
            display: block;
        }

        .color-preview {
            width: 100%;
            height: 60px;
            border-radius: 8px;
            margin: 12px 0;
            border: none;
            box-shadow: inset 0 2px 4px rgba(0,0,0,0.1);
            transition: background-color 0.3s ease;
        }

        .color-sliders {
            display: grid;
            gap: 16px;
            grid-template-columns: repeat(2fr);
        }

        .color-slider {
            background: linear-gradient(to right, 
                hsl(0, 100%, 50%), hsl(60, 100%, 50%), 
                hsl(120, 100%, 50%), hsl(180, 100%, 50%), 
                hsl(240, 100%, 50%), hsl(300, 100%, 50%), 
                hsl(360, 100%, 50%));
        }

        .saturation-slider {
            background: linear-gradient(to right, 
                rgb(128, 128, 128), 
                var(--current-color, #ff0000));
        }

        .color-grid {
            display: grid;
            grid-template-columns: auto 1fr;
            gap: 20px;
            align-items: start;
        }

        .color-picker-container {
            width: 200px;
        }

        .color-picker {
            width: 100%;
            height: 200px;
            border-radius: 8px;
            border: none;
            cursor: pointer;
            padding: 0;
            box-shadow: inset 0 2px 4px rgba(0,0,0,0.1);
        }

                .color-input {
            width: 100%;
            padding: 8px;
            border: 1px solid #ddd;
            border-radius: 8px;
            font-size: 14px;
            margin-top: 8px;
            text-align: center;
            font-family: monospace;
            background: white;
        }

        .color-input:focus {
            outline: none;
            border-color: var(--primary-color);
        }

        .rainbow-selector, .fire-selector, .life-selector {
            margin-bottom: 20px;
        }

        .rainbow-options, .fire-options, .life-options {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(100px, 1fr));
            gap: 10px;
            margin-top: 10px;
        }

        .rainbow-option, .fire-option, .life-option {
            cursor: pointer;
            text-align: center;
            padding: 10px;
            border-radius: 8px;
            border: 2px solid transparent;
            transition: all 0.3s ease;
        }

        .rainbow-option:hover, .fire-option:hover, .life-option:hover {
            background-color: #f0f0f0;
        }

        .rainbow-option.selected, .fire-option.selected, .life-option.selected {
            border-color: var(--primary-color);
            background-color: #f0f0f0;
        }

        .rainbow-option .preview, .fire-option .preview, .life-option .preview {
            width: 100%;
            height: 60px;
            border-radius: 6px;
            margin-bottom: 8px;
        }

        .preview.diagonal {
            background: linear-gradient(45deg, 
                red, orange, yellow, green, blue, indigo, violet);
        }

        .preview.horizontal {
            background: linear-gradient(180deg,
                red, orange, yellow, green, blue, indigo, violet);
        }

        .preview.vertical {
            background: linear-gradient(90deg,
                red, orange, yellow, green, blue, indigo, violet);
        }

        .preview.circular {
            background: radial-gradient(circle,
                violet, indigo, blue, green, yellow, orange, red);
        }

                .preview.fire-red {
            background: linear-gradient(0deg,
                #000000 0%,
                #800000 20%,
                #b30000 40%,
                #ff0000 60%,
                #ff4000 80%,
                #ff8000 100%);
        }

        .preview.fire-light-red {
            background: linear-gradient(0deg,
                #000000 0%,
                #ff0000 20%,
                #ff4000 40%,
                #ff8000 60%,
                #ffc040 80%,
                #ffff80 100%);
        }

        .preview.fire-yellow {
            background: linear-gradient(0deg,
                #000000 0%,
                #804000 20%,
                #c08000 40%,
                #ffc000 60%,
                #ffff00 80%,
                #ffff80 100%);
        }

        .preview.fire-green {
            background: linear-gradient(0deg,
                #000000 0%,
                #002000 20%,
                #004000 40%,
                #008000 60%,
                #20c000 80%,
                #40ff00 100%);
        }

        .preview.fire-blue {
            background: linear-gradient(0deg,
                #000000 0%,
                #000080 20%,
                #0000c0 40%,
                #0000ff 60%,
                #0080ff 80%,
                #80c0ff 100%);
        }

        .preview.fire-black {
            background: linear-gradient(0deg,
                #000000 0%,
                #101010 20%,
                #202020 40%,
                #404040 60%,
                #606060 80%,
                #808080 100%);
        }

        .life-option .preview {
            background-color: #000;
            position: relative;
        }

        .preview.life-random {
            background-image: radial-gradient(circle, #fff 2px, transparent 2px);
            background-size: 10px 10px;
            background-position: 0 0, 5px 5px;
        }

                .preview.life-block::after {
            content: '';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            width: 20px;
            height: 20px;
            background-color: #fff;
            box-shadow: 0 0 10px rgba(255,255,255,0.5);
        }

        .preview.life-blinker::after {
            content: '';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            width: 30px;
            height: 10px;
            background-color: #fff;
            box-shadow: 0 0 10px rgba(255,255,255,0.5);
        }

        .preview.life-glider::after {
            content: '';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            width: 30px;
            height: 30px;
            background-image: 
                radial-gradient(circle, #fff 3px, transparent 3px);
            background-position: 
                10px 0px,
                20px 10px,
                0px 20px, 10px 20px, 20px 20px;
            background-size: 10px 10px;
            background-repeat: no-repeat;
        }

        .preview.life-toad::after {
            content: '';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            width: 40px;
            height: 20px;
            background-image: 
                linear-gradient(to right, #fff 30px, transparent 30px),
                linear-gradient(to right, transparent 10px, #fff 10px, #fff 40px);
            background-position: 
                10px 0px,
                0px 10px;
            background-size: 40px 10px;
            background-repeat: no-repeat;
        }

        .preview.life-beacon::after {
            content: '';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            width: 40px;
            height: 40px;
            background-image: 
                linear-gradient(to right, #fff 20px, transparent 20px),
                linear-gradient(to right, #fff 20px, transparent 20px),
                linear-gradient(to right, transparent 20px, #fff 20px),
                linear-gradient(to right, transparent 20px, #fff 20px);
            background-position: 
                0px 0px,
                0px 10px,
                20px 20px,
                20px 30px;
            background-size: 40px 10px;
            background-repeat: no-repeat;
        }

                .preview.life-lwss::after {
            content: '';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            width: 50px;
            height: 40px;
            background-image: 
                radial-gradient(circle, #fff 3px, transparent 3px);
            background-position: 
                10px 0px, 40px 0px,
                0px 10px,
                0px 20px, 40px 20px,
                10px 30px, 20px 30px, 30px 30px;
            background-size: 10px 10px;
            background-repeat: no-repeat;
        }

        .life-controls {
            margin-top: 20px;
            padding-top: 15px;
            border-top: 1px solid #eee;
        }

        .control-row {
            display: flex;
            justify-content: space-around;
            align-items: center;
            flex-wrap: wrap;
            gap: 20px;
        }

        .toggle-container {
            display: flex;
            justify-content: center;
        }

        .toggle {
            display: inline-flex;
            align-items: center;
            cursor: pointer;
            padding: 10px;
            border-radius: 8px;
            transition: background-color 0.3s;
        }

        .toggle:hover {
            background-color: #f0f0f0;
        }

        .toggle-label {
            margin-right: 10px;
            font-size: 14px;
            color: var(--text-color);
        }

        .toggle-switch {
            position: relative;
            width: 44px;
            height: 24px;
            background-color: #ccc;
            border-radius: 12px;
            transition: background-color 0.3s;
        }

        .toggle-switch::before {
            content: '';
            position: absolute;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background-color: white;
            top: 2px;
            left: 2px;
            transition: transform 0.3s;
        }

        .toggle input {
            display: none;
        }

        .toggle input:checked + .toggle-switch {
            background-color: var(--primary-color);
        }

        .toggle input:checked + .toggle-switch::before {
            transform: translateX(20px);
        }

        .speed-container {
            display: flex;
            flex-direction: column;
            align-items: center;
            width: 200px;
        }

        .speed-label {
            font-size: 14px;
            color: var(--text-color);
            margin-bottom: 8px;
        }

        .speed-slider {
            width: 100%;
            margin: 8px 0;
            -webkit-appearance: none;
            background: transparent;
        }

        .speed-slider::-webkit-slider-runnable-track {
            width: 100%;
            height: 6px;
            background: #ddd;
            border-radius: 3px;
            background-image: linear-gradient(90deg, 
                #666 16.66%,
                #ddd 16.66%, #ddd 33.33%,
                #ddd 33.33%, #ddd 50%,
                #ddd 50%, #ddd 66.66%,
                #ddd 66.66%, #ddd 83.33%,
                #ddd 83.33%);
        }

        .speed-slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            height: 18px;
            width: 18px;
            border-radius: 50%;
            background: var(--primary-color);
            margin-top: -6px;
            cursor: pointer;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }

        .clock-info {
            margin-top: 15px;
            padding: 15px;
            background: #f8f9fa;
            border-radius: 8px;
            color: #2c3e50;
        }

        .clock-info p {
            white-space: pre-wrap;
            line-height: 1.5;
        }

        @media (max-width: 480px) {
            .color-sliders {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>Control de LED</h1>
        <div class="control-group">
            <button id="toggleButton" class="primary" onclick="toggleState()">ENCENDIDO</button>
        </div>
        <div class="control-group">
            <label for="brightness">Brillo</label>
            <input type="range" id="brightness" min="0" max="255" value="255" oninput="updateBrightness(this.value)">
        </div>
        <div class="control-group">
            <label for="effect">Efecto</label>
            <select id="effect" onchange="updateEffect(this.value)">
                <option value="3">Fuego</option>
                <option value="2">Arcoíris</option>
                <option value="1">Respiración</option>
                <option value="0">Sólido</option>
                <option value="4">Vida</option>
                <option value="5">Reloj</option>
                <option value="6">Apagado</option>
            </select>
        </div>

        <div class="color-controls" id="colorControls">

            <!-- Selector de reloj -->
            <div class="control-group clock-selector" id="clockSelector" style="display: none;">
                <label id="passageReference">Información del Reloj</label>
                <div class="clock-info">
                    <p id="clockText"></p>
                </div>
            </div>

            <!-- Selector de tipo de rainbow -->
            <div class="control-group rainbow-selector" id="rainbowSelector" style="display: none;">
                <label>Tipo de Arcoíris</label>
                <div class="rainbow-options">
                    <div class="rainbow-option" data-type="diagonal">
                        <div class="preview diagonal"></div>
                        <span>Diagonal</span>
                    </div>
                    <div class="rainbow-option" data-type="horizontal">
                        <div class="preview horizontal"></div>
                        <span>Horizontal</span>
                    </div>
                    <div class="rainbow-option" data-type="vertical">
                        <div class="preview vertical"></div>
                        <span>Vertical</span>
                    </div>
                    <div class="rainbow-option" data-type="circular">
                        <div class="preview circular"></div>
                        <span>Circular</span>
                    </div>
                </div>
            </div>

            <!-- Selector de tipo de fuego -->
            <div class="control-group fire-selector" id="fireSelector" style="display: none;">
                <label>Tipo de Fuego</label>
                <div class="fire-options">
                    <div class="fire-option" data-palette="0">
                        <div class="preview fire-red"></div>
                        <span>Rojo</span>
                    </div>
                    <div class="fire-option" data-palette="1">
                        <div class="preview fire-light-red"></div>
                        <span>Naranja</span>
                    </div>
                    <div class="fire-option" data-palette="2">
                        <div class="preview fire-yellow"></div>
                        <span>Amarillo</span>
                    </div>
                    <div class="fire-option" data-palette="3">
                        <div class="preview fire-green"></div>
                        <span>Verde</span>
                    </div>
                    <div class="fire-option" data-palette="4">
                        <div class="preview fire-blue"></div>
                        <span>Azul</span>
                    </div>
                    <div class="fire-option" data-palette="5">
                        <div class="preview fire-black"></div>
                        <span>Negro</span>
                    </div>
                </div>
            </div>

                        <!-- Selector de patrones de vida -->
            <div class="control-group life-selector" id="lifeSelector" style="display: none;">
                <label>Patrones de Vida</label>
                <div class="life-options">
                    <div class="life-option" data-pattern="0">
                        <div class="preview life-random"></div>
                        <span>Aleatorio</span>
                    </div>
                    <div class="life-option" data-pattern="1">
                        <div class="preview life-block"></div>
                        <span>Bloque</span>
                    </div>
                    <div class="life-option" data-pattern="2">
                        <div class="preview life-blinker"></div>
                        <span>Blinker</span>
                    </div>
                    <div class="life-option" data-pattern="3">
                        <div class="preview life-glider"></div>
                        <span>Glider</span>
                    </div>
                    <div class="life-option" data-pattern="4">
                        <div class="preview life-toad"></div>
                        <span>Toad</span>
                    </div>
                    <div class="life-option" data-pattern="5">
                        <div class="preview life-beacon"></div>
                        <span>Beacon</span>
                    </div>
                    <div class="life-option" data-pattern="6">
                        <div class="preview life-lwss"></div>
                        <span>LWSS</span>
                    </div>
                </div>
                <div class="life-controls">
                    <div class="control-row">
                        <div class="toggle-container">
                            <label class="toggle">
                                <span class="toggle-label">Reinicio Automático</span>
                                <input type="checkbox" id="autoRestart" checked>
                                <span class="toggle-switch"></span>
                            </label>
                        </div>
                        <div class="speed-container">
                            <label class="speed-label">Velocidad: <span id="speedValue">1x</span></label>
                            <input type="range" 
                                   id="speedSlider" 
                                   class="speed-slider" 
                                   min="0" 
                                   max="5" 
                                   step="1" 
                                   value="4">
                        </div>
                    </div>
                </div>
            </div>

            <div class="control-group" id="saturationControl">
                <label for="saturation">Saturación</label>
                <input type="range" id="saturation" class="saturation-slider" 
                      min="0" max="255" value="255" oninput="updateColor()">
            </div>

            <!-- Controles de color -->
            <div class="color-grid" id="colorGrid">
                <div class="color-picker-container">
                    <input type="color" id="colorPicker" class="color-picker" value="#ff0000" oninput="updateFromPicker(this.value)">
                </div>
                <div class="color-sliders">
                    <div class="control-group">
                        <label for="hue">Color</label>
                        <input type="range" id="hue" class="color-slider" min="0" max="255" value="0" oninput="updateColor()">
                    </div>
                    <div class="control-group">
                        <label for="saturation">Saturación</label>
                        <input type="range" id="saturation" class="saturation-slider" min="0" max="255" value="255" oninput="updateColor()">
                    </div>
                    <div class="color-inputs">
                        <div class="input-group">
                            <label for="colorInput">HEX</label>
                            <input type="text" id="colorInput" class="color-input" value="#FF0000" maxlength="7" oninput="updateFromInput(this.value)">
                        </div>
                        <div class="input-group">
                            <label for="rgbInput">RGB</label>
                            <input type="text" id="rgbInput" class="color-input" value="255, 0, 0" oninput="updateFromRgbInput(this.value)">
                        </div>
                    </div>
                </div>
            </div>
        </div>
        <div class="status" id="status"></div>
    </div>
        <script>

        let currentRainbowType = 'diagonal';
        let currentFirePalette = 0;
        let isChangingFirePalette = false;
        let currentLifePattern = 0;
        let isChangingLifePattern = false;
        let isChangingRainbowType = false;
        let currentStateVersion = 0;

        const speedValues = [0, 0.25, 0.5, 0.75, 1, 2];

        let currentState = {
            state: false,
            brightness: 255,
            effect: 0,
            hue: 0,
            saturation: 255
        };

        // Añadir aquí la clase UpdateQueue
        class UpdateQueue {
            constructor() {
                this.queue = [];
                this.processing = false;
            }

            add(update) {
                this.queue.push(update);
                if (!this.processing) {
                    this.process();
                }
            }

            async process() {
                if (this.queue.length === 0) {
                    this.processing = false;
                    return;
                }

                this.processing = true;
                const update = this.queue.shift();

                try {
                    await update();
                } catch (error) {
                    console.error('Error processing update:', error);
                }

                requestAnimationFrame(() => this.process());
            }
        }

        // Crear instancia global de UpdateQueue
        const updateQueue = new UpdateQueue();

        // Función helper para encolar actualizaciones
        function queueUpdate(updateFunction) {
            updateQueue.add(updateFunction);
        }

        // Función debounce
        function debounce(func, wait) {
            let timeout;
            return function executedFunction(...args) {
                const later = () => {
                    clearTimeout(timeout);
                    func(...args);
                };
                clearTimeout(timeout);
                timeout = setTimeout(later, wait);
            };
        }

        // Crear versiones debounced de las funciones de actualización
        const debouncedUpdateBrightness = debounce((value) => {
            fetch('http://' + window.location.hostname + ':81/api/brightness', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ brightness: parseInt(value) })
            });
        }, 100);

        const debouncedUpdateColor = debounce(() => {
            const hue = parseInt(document.getElementById('hue').value);
            const saturation = parseInt(document.getElementById('saturation').value);
            fetch('http://' + window.location.hostname + ':81/api/color', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ hue, saturation })
            });
        }, 100);

        function updateSpeedLabel(value) {
            const speedLabel = document.getElementById('speedValue');
            const speed = speedValues[value];
            speedLabel.textContent = speed === 0 ? 'Pausa' : speed + 'x';
        }

        function initSpeedControls() {
            const speedSlider = document.getElementById('speedSlider');
            if (speedSlider) {
                speedSlider.addEventListener('input', function() {
                    updateSpeedLabel(this.value);
                    updateLifeSpeed(speedValues[this.value]);
                });
            }
        }

        function updateLifeSpeed(speed) {
            fetch('http://' + window.location.hostname + ':81/api/life-speed', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ speed: speed })
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
            })
            .catch(error => console.error('Error:', error));
        }

        function updateAutoRestart(enabled) {
            fetch('http://' + window.location.hostname + ':81/api/life-auto-restart', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ enabled: enabled })
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
            })
            .catch(error => console.error('Error:', error));
        }

        function updateLifePattern() {
            isChangingLifePattern = true;
            
            fetch('http://' + window.location.hostname + ':81/api/life-pattern', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ pattern: currentLifePattern })
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
                isChangingLifePattern = false;
            })
            .catch(error => {
                console.error('Error:', error);
                isChangingLifePattern = false;
            });
        }


        function initLifeSelector() {
            const options = document.querySelectorAll('.life-option');
            options.forEach(option => {
                option.addEventListener('click', function() {
                    if (isChangingLifePattern) return;
                    
                    console.log('Life pattern clicked:', this.dataset.pattern);
                    options.forEach(opt => opt.classList.remove('selected'));
                    this.classList.add('selected');
                    currentLifePattern = parseInt(this.dataset.pattern);
                    updateLifePattern();
                });
            });
        }

        function initFireSelector() {
            const options = document.querySelectorAll('.fire-option');
            options.forEach(option => {
                option.addEventListener('click', function() {
                    if (isChangingFirePalette) return;
                    
                    console.log('Fire option clicked:', this.dataset.palette);
                    options.forEach(opt => opt.classList.remove('selected'));
                    this.classList.add('selected');
                    currentFirePalette = parseInt(this.dataset.palette);
                    updateFirePalette();
                });
            });
        }

        function updateFirePalette() {
            isChangingFirePalette = true;
            
            fetch('http://' + window.location.hostname + ':81/api/fire-palette', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ palette: currentFirePalette })
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
                isChangingFirePalette = false;
            })
            .catch(error => {
                console.error('Error:', error);
                isChangingFirePalette = false;
            });
        }

        function initRainbowSelector() {
            const options = document.querySelectorAll('.rainbow-option');
            options.forEach(option => {
                option.addEventListener('click', function() {
                    if (isChangingRainbowType) return;
                    
                    console.log('Rainbow option clicked:', this.dataset.type);
                    options.forEach(opt => opt.classList.remove('selected'));
                    this.classList.add('selected');
                    currentRainbowType = this.dataset.type;
                    updateRainbowType();
                });
            });
        }

        function updateRainbowType() {
            isChangingRainbowType = true;
            
            fetch('http://' + window.location.hostname + ':81/api/rainbow-type', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ type: currentRainbowType })
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
                return response.json();
            })
            .then(data => {
                console.log('Rainbow type updated successfully:', data);
                isChangingRainbowType = false;
            })
            .catch(error => {
                console.error('Error:', error);
                isChangingRainbowType = false;
            });
        }

        function hsvToRgb(h, s, v) {
            h = h / 255 * 360;
            s = s / 255;
            v = v / 255;
            
            let c = v * s;
            let x = c * (1 - Math.abs((h / 60) % 2 - 1));
            let m = v - c;
            let r, g, b;

            if (h >= 0 && h < 60) { r = c; g = x; b = 0; }
            else if (h >= 60 && h < 120) { r = x; g = c; b = 0; }
            else if (h >= 120 && h < 180) { r = 0; g = c; b = x; }
            else if (h >= 180 && h < 240) { r = 0; g = x; b = c; }
            else if (h >= 240 && h < 300) { r = x; g = 0; b = c; }
            else { r = c; g = 0; b = x; }

            return {
                r: Math.round((r + m) * 255),
                g: Math.round((g + m) * 255),
                b: Math.round((b + m) * 255)
            };
        }

        function rgbToHsv(r, g, b) {
            r /= 255;
            g /= 255;
            b /= 255;

            const max = Math.max(r, g, b);
            const min = Math.min(r, g, b);
            let h, s, v = max;

            const d = max - min;
            s = max === 0 ? 0 : d / max;

            if (max === min) {
                h = 0;
            } else {
                switch (max) {
                    case r:
                        h = (g - b) / d + (g < b ? 6 : 0);
                        break;
                    case g:
                        h = (b - r) / d + 2;
                        break;
                    case b:
                        h = (r - g) / d + 4;
                        break;
                }
                h /= 6;
            }

            return {
                h: Math.round(h * 255),
                s: Math.round(s * 255),
                v: Math.round(v * 255)
            };
        }

        function updateFromRgbInput(value) {
            const rgb = value.replace(/\s/g, '').split(',');
            
            if (rgb.length === 3) {
                const r = parseInt(rgb[0]);
                const g = parseInt(rgb[1]);
                const b = parseInt(rgb[2]);
                
                if (!isNaN(r) && !isNaN(g) && !isNaN(b) &&
                    r >= 0 && r <= 255 && 
                    g >= 0 && g <= 255 && 
                    b >= 0 && b <= 255) {
                    
                    const hexColor = '#' + 
                        r.toString(16).padStart(2, '0') + 
                        g.toString(16).padStart(2, '0') + 
                        b.toString(16).padStart(2, '0');
                    
                    updateFromPicker(hexColor.toUpperCase());
                }
            }
        }

        function updateFromPicker(hexColor) {
            document.getElementById('colorInput').value = hexColor.toUpperCase();
            
            const r = parseInt(hexColor.substr(1,2), 16);
            const g = parseInt(hexColor.substr(3,2), 16);
            const b = parseInt(hexColor.substr(5,2), 16);
            
            const hsv = rgbToHsv(r, g, b);
            
            document.getElementById('hue').value = hsv.h;
            document.getElementById('saturation').value = hsv.s;
            
            updateColor();
        }

        function updateFromInput(value) {
            if (/^#[0-9A-F]{6}$/i.test(value)) {
                updateFromPicker(value);
            }
        }

        function updateColorPreview() {
            const hue = parseInt(document.getElementById('hue').value);
            const saturation = parseInt(document.getElementById('saturation').value);
            const rgb = hsvToRgb(hue, saturation, 255);
            const color = `rgb(${rgb.r},${rgb.g},${rgb.b})`;
            
            document.documentElement.style.setProperty('--current-color', color);
            
            const hexColor = '#' + 
                rgb.r.toString(16).padStart(2, '0') + 
                rgb.g.toString(16).padStart(2, '0') + 
                rgb.b.toString(16).padStart(2, '0');
            
            document.getElementById('colorPicker').value = hexColor;
            document.getElementById('colorInput').value = hexColor.toUpperCase();
            document.getElementById('rgbInput').value = `${rgb.r}, ${rgb.g}, ${rgb.b}`;
        }

        function updateColor() {
            const hue = parseInt(document.getElementById('hue').value);
            const saturation = parseInt(document.getElementById('saturation').value);
            updateColorPreview();
            
            debouncedUpdateColor();
        }

        function getEffectName(effect) {
            console.log('Efecto recibido:', effect, typeof effect); // Debug
            const effects = [
                'Sólido',      // 0
                'Respiración', // 1
                'Arcoíris',    // 2
                'Fuego',       // 3
                'Vida',        // 4
                'Reloj',       // 5
                'Apagado'      // 6
            ];
            const effectName = effects[effect] || 'Desconocido';
            console.log('Nombre del efecto:', effectName); // Debug
            return effectName;
        }

        // Ajustar el intervalo de actualización según el efecto
        function setUpdateInterval() {
            const effect = document.getElementById('effect').value;
            const interval = (effect == 5) ? 5000 : 2000; // 5 segundos para reloj, 2 para otros
            
            if (window.statusInterval) {
                clearInterval(window.statusInterval);
            }
            window.statusInterval = setInterval(updateStatus, interval);
        }

        

        function updateRequiredElements(data) {
            // Actualizar elementos básicos siempre
            document.getElementById('toggleButton').textContent = data.state ? 'ENCENDIDO' : 'APAGADO';
            document.getElementById('toggleButton').className = data.state ? 'primary' : 'off';
            document.getElementById('effect').value = data.effect;

            // Actualizar elementos específicos del efecto
            const effectSpecificUpdates = {
                [SOLID]: () => updateColorControls(data),
                [BREATHING]: () => updateColorControls(data),
                [RAINBOW]: () => updateRainbowControls(data),
                [FIRE]: () => updateFireControls(data),
                [LIFE]: () => updateLifeControls(data),
                [CLOCK]: () => updateClockDisplay(data)
            };

            if (effectSpecificUpdates[data.effect]) {
                effectSpecificUpdates[data.effect]();
            }

            // Actualizar visibilidad de controles
            updateControlsVisibility(data.effect);
        }

        function toggleState() {
            const newState = !currentState.state;
            fetch('http://' + window.location.hostname + ':81/api/state', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ state: newState })
            }).then(() => updateStatus());
        }

        function updateBrightness(value) {
            debouncedUpdateBrightness(value);
        }

        function updateEffect(value) {
            fetch('http://' + window.location.hostname + ':81/api/effect', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ effect: parseInt(value) })
            })
            .then(() => {
                updateStatus();
                setUpdateInterval();
            });
        }

        // Inicialización
        function updateStatus() {
            fetch('http://' + window.location.hostname + ':81/api/status')
                .then(response => response.json())
                .then(data => {
                    console.log('Data recibida:', data); // Debug completo
                    console.log('Efecto actual:', data.effect, typeof data.effect); // Debug del efecto
                    queueUpdate(() => {
                        currentState = data;
                        const button = document.getElementById('toggleButton');
                        button.textContent = data.state ? 'ENCENDIDO' : 'APAGADO';
                        button.className = data.state ? 'primary' : 'off';
                        
                        document.getElementById('brightness').value = data.brightness;
                        document.getElementById('effect').value = data.effect;
                        document.getElementById('hue').value = data.hue;
                        document.getElementById('saturation').value = data.saturation;
                        
                        const colorControls = document.getElementById('colorControls');
                        const rainbowSelector = document.getElementById('rainbowSelector');
                        const fireSelector = document.getElementById('fireSelector');
                        const lifeSelector = document.getElementById('lifeSelector');
                        const clockSelector = document.getElementById('clockSelector');
                        const colorPickers = document.querySelector('.color-grid');
                        const saturationControl = document.getElementById('saturationControl');
                        
                        // Ocultar todos los controles primero
                        rainbowSelector.style.display = 'none';
                        fireSelector.style.display = 'none';
                        lifeSelector.style.display = 'none';
                        clockSelector.style.display = 'none';
                        colorPickers.style.display = 'none';
                        if (saturationControl) saturationControl.style.display = 'none';
                        
                        colorControls.classList.add('visible');
                        
                        // Mostrar los controles según el efecto
                        switch(parseInt(data.effect)) {
                            case 0: // SOLID
                            case 1: // BREATHING
                                colorPickers.style.display = 'grid';
                                break;
                                
                            case 2: // RAINBOW
                                rainbowSelector.style.display = 'block';
                                if (saturationControl) saturationControl.style.display = 'block';
                                
                                if (!isChangingRainbowType && data.rainbowType) {
                                    const rainbowOptions = document.querySelectorAll('.rainbow-option');
                                    rainbowOptions.forEach(opt => {
                                        opt.classList.remove('selected');
                                        if (opt.dataset.type === data.rainbowType) {
                                            opt.classList.add('selected');
                                        }
                                    });
                                }
                                break;
                                
                            case 3: // FIRE
                                fireSelector.style.display = 'block';
                                if (!isChangingFirePalette) {
                                    const fireOptions = document.querySelectorAll('.fire-option');
                                    fireOptions.forEach(opt => {
                                        opt.classList.remove('selected');
                                        if (parseInt(opt.dataset.palette) === data.firePalette) {
                                            opt.classList.add('selected');
                                        }
                                    });
                                }
                                break;
                                
                            case 4: // LIFE
                                lifeSelector.style.display = 'block';
                                if (!isChangingLifePattern) {
                                    const lifeOptions = document.querySelectorAll('.life-option');
                                    lifeOptions.forEach(opt => {
                                        opt.classList.remove('selected');
                                        if (parseInt(opt.dataset.pattern) === data.lifePattern) {
                                            opt.classList.add('selected');
                                        }
                                    });
                                }
                                
                                if (data.lifeAutoRestart !== undefined) {
                                    document.getElementById('autoRestart').checked = data.lifeAutoRestart;
                                }
                                if (data.lifeSpeed !== undefined) {
                                    const speedIndex = speedValues.indexOf(data.lifeSpeed);
                                    if (speedIndex !== -1) {
                                        document.getElementById('speedSlider').value = speedIndex;
                                        updateSpeedLabel(speedIndex);
                                    }
                                }
                                break;

                            case 5: // CLOCK
                                clockSelector.style.display = 'block';
                                if (data.clockText) {
                                    document.getElementById('clockText').textContent = data.clockText;
                                }
                                if (data.passageReference) {
                                    document.getElementById('passageReference').textContent = data.passageReference;
                                }
                                break;
                                
                            case 6: // OFF
                                colorControls.classList.remove('visible');
                                break;
                        }
                        
                        if (data.effect === 0 || data.effect === 1) {
                            updateColorPreview();
                        }
                        
                        console.log('Actualizando status con efecto:', data.effect);
                        const status = document.getElementById('status');
                        status.textContent = `Estado: ${data.state ? 'Encendido' : 'Apagado'} | Brillo: ${data.brightness} | Efecto: ${getEffectName(parseInt(data.effect))}`;
                    });
                })
                .catch(error => console.error('Error:', error));
        }

        // Definir setUpdateInterval también fuera
        function setUpdateInterval() {
            const effect = document.getElementById('effect').value;
            const interval = (effect == 5) ? 5000 : 2000; // 5 segundos para reloj, 2 para otros
            
            if (window.statusInterval) {
                clearInterval(window.statusInterval);
            }
            window.statusInterval = setInterval(updateStatus, interval);
        }

        // En el DOMContentLoaded, solo llamar a las funciones
        document.addEventListener('DOMContentLoaded', function() {
            initRainbowSelector();
            initFireSelector();
            initLifeSelector();
            initSpeedControls();

            const autoRestartToggle = document.getElementById('autoRestart');
            if (autoRestartToggle) {
                autoRestartToggle.addEventListener('change', function() {
                    updateAutoRestart(this.checked);
                });
            }
            
            updateStatus();
            setUpdateInterval();
        });
    </script>
</body>
</html>
        )HTMLCONTENT";
    }

public:
    WebManager(LedManager* ledMgr) : server(81), ledManager(ledMgr) {}

    void begin() {
        setupRoutes();
        server.begin();
    }
};

#endif