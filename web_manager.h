#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "led_manager.h"

class WebManager {
private:
    AsyncWebServer server;
    LedManager* ledManager;
    
    void setupRoutes() {
        // Configurar rutas como antes
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(200, "text/html", getIndexHTML());
        });

        // Endpoint de estado
        server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request){
            String response;
            StaticJsonDocument<200> doc;
            doc["state"] = ledManager->getState();
            doc["brightness"] = ledManager->getBrightness();
            doc["effect"] = ledManager->getCurrentEffect();
            doc["hue"] = ledManager->getHue();
            doc["saturation"] = ledManager->getSaturation();
            serializeJson(doc, response);
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

        // API para cambiar el estado
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

        // API para cambiar el brillo
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

        // API para cambiar el efecto
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
        // Agregar manejador para rutas no encontradas
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
            /* Modifica el estilo existente */
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
    /* Añade este nuevo estilo */
    display: grid;
    grid-template-columns: auto 1fr;
    gap: 20px;
    align-items: start;
}

.color-picker-container {
    /* Añade este nuevo estilo */
    width: 200px;
}

.color-picker {
    /* Añade este nuevo estilo */
    width: 100%;
    height: 200px;
    border-radius: 8px;
    border: none;
    cursor: pointer;
    padding: 0;
    box-shadow: inset 0 2px 4px rgba(0,0,0,0.1);
}

.color-input {
    /* Añade este nuevo estilo */
    width: 100%;
    padding: 8px;
    border: 1px solid #ddd;
    border-radius: 8px;
    font-size: 14px;
    margin-top: 8px;
    text-align: center;
    font-family: monospace;
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
                <option value="0">Sólido</option>
                <option value="1">Respiración</option>
                <option value="2">Arcoíris</option>
                <option value="3">Fuego</option>
                <option value="4">Apagado</option>
            </select>
        </div>
        <div class="color-controls" id="colorControls">
    <div class="color-grid">
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
            <input type="text" id="colorInput" class="color-input" value="#FF0000" maxlength="7" oninput="updateFromInput(this.value)">
        </div>
    </div>
</div>
        <div class="status" id="status"></div>
    </div>
    

    <script>
        let currentState = {
            state: false,
            brightness: 255,
            effect: 0,
            hue: 0,
            saturation: 255
        };

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

function updateFromPicker(hexColor) {
    // Actualizar input de texto
    document.getElementById('colorInput').value = hexColor.toUpperCase();
    
    // Convertir hex a RGB
    const r = parseInt(hexColor.substr(1,2), 16);
    const g = parseInt(hexColor.substr(3,2), 16);
    const b = parseInt(hexColor.substr(5,2), 16);
    
    // Convertir RGB a HSV
    const hsv = rgbToHsv(r, g, b);
    
    // Actualizar sliders
    document.getElementById('hue').value = hsv.h;
    document.getElementById('saturation').value = hsv.s;
    
    updateColor();
}

function updateFromInput(value) {
    // Validar formato hex
    if (/^#[0-9A-F]{6}$/i.test(value)) {
        updateFromPicker(value);
    }
}

        function updateColorPreview() {
    const hue = parseInt(document.getElementById('hue').value);
    const saturation = parseInt(document.getElementById('saturation').value);
    const rgb = hsvToRgb(hue, saturation, 255);
    const color = `rgb(${rgb.r},${rgb.g},${rgb.b})`;
    
    // Actualizar preview y slider de saturación
    document.documentElement.style.setProperty('--current-color', color);
    
    // Actualizar color picker y input
    const hexColor = '#' + 
        rgb.r.toString(16).padStart(2, '0') + 
        rgb.g.toString(16).padStart(2, '0') + 
        rgb.b.toString(16).padStart(2, '0');
    document.getElementById('colorPicker').value = hexColor;
    document.getElementById('colorInput').value = hexColor.toUpperCase();
}

        function updateStatus() {
            fetch('http://' + window.location.hostname + ':81/api/status')
                .then(response => response.json())
                .then(data => {
                    currentState = data;
                    const button = document.getElementById('toggleButton');
                    button.textContent = data.state ? 'ENCENDIDO' : 'APAGADO';
                    button.className = data.state ? 'primary' : 'off';
                    
                    document.getElementById('brightness').value = data.brightness;
                    document.getElementById('effect').value = data.effect;
                    document.getElementById('hue').value = data.hue;
                    document.getElementById('saturation').value = data.saturation;
                    
                    const colorControls = document.getElementById('colorControls');
                    if (data.effect === 0 || data.effect === 1) {
                        colorControls.classList.add('visible');
                        updateColorPreview();
                    } else {
                        colorControls.classList.remove('visible');
                    }
                    
                    const status = document.getElementById('status');
                    status.textContent = `Estado: ${data.state ? 'Encendido' : 'Apagado'} | Brillo: ${data.brightness} | Efecto: ${getEffectName(data.effect)}`;
                })
                .catch(error => console.error('Error:', error));
        }

        function updateColor() {
            const hue = parseInt(document.getElementById('hue').value);
            const saturation = parseInt(document.getElementById('saturation').value);
            updateColorPreview();
            
            fetch('http://' + window.location.hostname + ':81/api/color', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ 
                    hue: hue,
                    saturation: saturation
                })
            }).then(() => updateStatus());
        }

        function getEffectName(effect) {
            const effects = ['Sólido', 'Respiración', 'Arcoíris', 'Fuego', 'Apagado'];
            return effects[effect] || 'Desconocido';
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
            fetch('http://' + window.location.hostname + ':81/api/brightness', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ brightness: parseInt(value) })
            }).then(() => updateStatus());
        }

        function updateEffect(value) {
            fetch('http://' + window.location.hostname + ':81/api/effect', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ effect: parseInt(value) })
            }).then(() => updateStatus());
        }

        // Inicialización
        updateStatus();
        setInterval(updateStatus, 2000);
    </script>
</body>
</html>
        )HTMLCONTENT";
    }

public:
    WebManager(LedManager* ledMgr) : server(81), ledManager(ledMgr) {}  // Cambiar puerto a 81

    void begin() {
        setupRoutes();
        server.begin();
    }
};

#endif