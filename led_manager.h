#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <FastLED.h>
#include "config.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

class LedManager {
private:
    CRGB leds[NUM_LEDS];
    LedEffect currentEffect;
    uint8_t brightness;
    bool isOn;
    unsigned long lastUpdate;
    String rainbowType;

    // Variables para efectos
    uint8_t hue = 0;
    uint8_t saturation = 255;
    uint8_t breathVal = 0;
    bool breathingUp = true;

    // Variables para pasaje
    uint8_t book;
    uint8_t chapter;
    uint8_t verse;

    // Variables para el Juego de la Vida
    bool lifeGrid[26][27];
    bool nextGrid[26][27];
    CRGB lifeColor = CRGB(255, 255, 255);
    unsigned long lastLifeUpdate = 0;
    static const unsigned long LIFE_UPDATE_INTERVAL = 100;

    // Constantes para la matriz
    static const uint8_t LED_WIDTH = 27;
    static const uint8_t LED_HEIGHT = 26;
    
    static const uint8_t PALETTE_SIZE = 6;

    bool autoRestart = true;
    bool isStaticPattern = false;

    float lifeSpeed = 1.0;
    const float SPEED_VALUES[6] = {0, 0.25, 0.5, 0.75, 1.0, 2.0};
    unsigned long LIFE_BASE_INTERVAL = 100;

    // Variables para el reloj
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    bool timeInitialized = false;
    const CRGB CLOCK_COLOR = CRGB(255, 255, 255);  // Blanco fijo
    const CRGB PASSAGE_COLOR = CRGB(0, 255, 255);  // Cian fijo
    
    
        const CRGB firePalettes[6][PALETTE_SIZE] = {
        { // Rojo
            CRGB(0, 0, 0),        // negro
            CRGB(128, 0, 0),      // rojo oscuro
            CRGB(179, 0, 0),      // rojo medio
            CRGB(255, 0, 0),      // rojo
            CRGB(255, 64, 0),     // rojo-naranja
            CRGB(255, 128, 0)     // naranja
        },
        { // Rojo claro/Naranja
            CRGB(0, 0, 0),        // negro
            CRGB(255, 0, 0),      // rojo
            CRGB(255, 64, 0),     // rojo-naranja
            CRGB(255, 128, 0),    // naranja
            CRGB(255, 192, 64),   // naranja claro
            CRGB(255, 255, 128)   // amarillo claro
        },
        { // Amarillo
            CRGB(0, 0, 0),        // negro
            CRGB(128, 64, 0),     // ámbar oscuro
            CRGB(192, 128, 0),    // ámbar
            CRGB(255, 192, 0),    // amarillo oscuro
            CRGB(255, 255, 0),    // amarillo
            CRGB(255, 255, 128)   // amarillo claro
        },
        { // Verde
            CRGB(0, 0, 0),        // negro
            CRGB(0, 32, 0),       // verde muy oscuro
            CRGB(0, 64, 0),       // verde oscuro
            CRGB(0, 128, 0),      // verde medio
            CRGB(32, 192, 0),     // verde claro
            CRGB(64, 255, 0)      // verde brillante
        },
        { // Azul
            CRGB(0, 0, 0),        // negro
            CRGB(0, 0, 128),      // azul oscuro
            CRGB(0, 0, 192),      // azul medio
            CRGB(0, 0, 255),      // azul
            CRGB(0, 128, 255),    // azul claro
            CRGB(128, 192, 255)   // azul muy claro
        },
        { // Negro
            CRGB(0, 0, 0),        // negro
            CRGB(16, 16, 16),     // gris muy oscuro
            CRGB(32, 32, 32),     // gris oscuro
            CRGB(64, 64, 64),     // gris medio
            CRGB(96, 96, 96),     // gris claro
            CRGB(128, 128, 128)   // gris
        }
    };

    // Patrón único para los dígitos (5x3)
    static constexpr bool DIGIT_PATTERNS[10][5][3] = {
        { // 0
            {1,1,1},
            {1,0,1},
            {1,0,1},
            {1,0,1},
            {1,1,1}
        },
        { // 1
            {0,1,0},
            {1,1,0},
            {0,1,0},
            {0,1,0},
            {1,1,1}
        },
        { // 2
            {1,1,1},
            {0,0,1},
            {1,1,1},
            {1,0,0},
            {1,1,1}
        },
        { // 3
            {1,1,1},
            {0,0,1},
            {0,1,1},
            {0,0,1},
            {1,1,1}
        },
        { // 4
            {1,0,1},
            {1,0,1},
            {1,1,1},
            {0,0,1},
            {0,0,1}
        },
        { // 5
            {1,1,1},
            {1,0,0},
            {1,1,1},
            {0,0,1},
            {1,1,1}
        },
        { // 6
            {1,1,1},
            {1,0,0},
            {1,1,1},
            {1,0,1},
            {1,1,1}
        },
        { // 7
            {1,1,1},
            {0,0,1},
            {0,1,0},
            {0,1,0},
            {0,1,0}
        },
        { // 8
            {1,1,1},
            {1,0,1},
            {1,1,1},
            {1,0,1},
            {1,1,1}
        },
        { // 9
            {1,1,1},
            {1,0,1},
            {1,1,1},
            {0,0,1},
            {1,1,1}
        }
    };

    static constexpr bool MINI_DIGITS[10][3][2] = {
        { // 0
            {0,0},
            {0,0},
            {1,1}
        },
        { // 1
            {1,0},
            {1,0},
            {1,0}
        },
        { // 2
            {1,0},
            {0,0},
            {0,1}
        },
        { // 3
            {0,1},
            {0,0},
            {1,1}
        },
        { // 4
            {0,1},
            {1,1},
            {0,1}
        },
        { // 5
            {1,1},
            {1,0},
            {1,1}
        },
        { // 6
            {1,0},
            {1,1},
            {1,1}
        },
        { // 7
            {1,1},
            {0,1},
            {0,1}
        },
        { // 8
            {1,1},
            {1,1},
            {1,1}
        },
        { // 9
            {1,1},
            {1,1},
            {0,1}
        }
    };

    uint8_t currentFirePalette = 0;
    uint8_t firePixels[NUM_LEDS];

    enum LifePattern {
        RANDOM,
        BLOCK,
        BLINKER,
        GLIDER,
        TOAD,
        BEACON,
        LWSS
    };
    

    String urlEncode(String str) {
        String encodedString = "";
        char c;
        char code0;
        char code1;
        for (int i = 0; i < str.length(); i++) {
            c = str.charAt(i);
            if (c == ' ') {
                encodedString += '+';
            } else if (isalnum(c)) {
                encodedString += c;
            } else {
                code1 = (c & 0xf) + '0';
                if ((c & 0xf) > 9) {
                    code1 = (c & 0xf) - 10 + 'A';
                }
                c = (c >> 4) & 0xf;
                code0 = c + '0';
                if (c > 9) {
                    code0 = c - 10 + 'A';
                }
                encodedString += '%';
                encodedString += code0;
                encodedString += code1;
            }
        }
        return encodedString;
    }
    
    LifePattern currentLifePattern = RANDOM;

        void setLifePattern(LifePattern pattern) {
        currentLifePattern = pattern;
        
        // Determinar si el patrón es estático
        isStaticPattern = (pattern == BLOCK);
        
        // Limpiar la matriz
        for(uint8_t y = 0; y < LED_HEIGHT; y++) {
            for(uint8_t x = 0; x < LED_WIDTH; x++) {
                lifeGrid[y][x] = false;
                nextGrid[y][x] = false;
            }
        }

        // Posición aleatoria para el patrón
        uint8_t startX = random(5, LED_WIDTH - 5);
        uint8_t startY = random(5, LED_HEIGHT - 5);

        switch(pattern) {
            case BLOCK:
                lifeGrid[startY][startX] = true;
                lifeGrid[startY][startX+1] = true;
                lifeGrid[startY+1][startX] = true;
                lifeGrid[startY+1][startX+1] = true;
                break;

            case BLINKER:
                lifeGrid[startY][startX-1] = true;
                lifeGrid[startY][startX] = true;
                lifeGrid[startY][startX+1] = true;
                break;

            case GLIDER:
                lifeGrid[startY-1][startX] = true;
                lifeGrid[startY][startX+1] = true;
                lifeGrid[startY+1][startX-1] = true;
                lifeGrid[startY+1][startX] = true;
                lifeGrid[startY+1][startX+1] = true;
                break;

            case TOAD:
                lifeGrid[startY][startX-1] = true;
                lifeGrid[startY][startX] = true;
                lifeGrid[startY][startX+1] = true;
                lifeGrid[startY+1][startX-2] = true;
                lifeGrid[startY+1][startX-1] = true;
                lifeGrid[startY+1][startX] = true;
                break;

            case BEACON:
                lifeGrid[startY][startX] = true;
                lifeGrid[startY][startX+1] = true;
                lifeGrid[startY+1][startX] = true;
                lifeGrid[startY+1][startX+1] = true;
                lifeGrid[startY+2][startX+2] = true;
                lifeGrid[startY+2][startX+3] = true;
                lifeGrid[startY+3][startX+2] = true;
                lifeGrid[startY+3][startX+3] = true;
                break;

            case LWSS:
                lifeGrid[startY][startX+1] = true;
                lifeGrid[startY][startX+4] = true;
                lifeGrid[startY+1][startX] = true;
                lifeGrid[startY+2][startX] = true;
                lifeGrid[startY+2][startX+4] = true;
                lifeGrid[startY+3][startX+1] = true;
                lifeGrid[startY+3][startX+2] = true;
                lifeGrid[startY+3][startX+3] = true;
                break;

            case RANDOM:
            default:
                initLife();
                break;
        }
    }

        void initLife() {
        // Inicializar con patrón aleatorio
        for(uint8_t y = 0; y < LED_HEIGHT; y++) {
            for(uint8_t x = 0; x < LED_WIDTH; x++) {
                lifeGrid[y][x] = random(2) == 1;
            }
        }
    }

    uint8_t countNeighbors(uint8_t x, uint8_t y) {
        uint8_t count = 0;
        for(int8_t i = -1; i <= 1; i++) {
            for(int8_t j = -1; j <= 1; j++) {
                if(i == 0 && j == 0) continue;
                
                int8_t newX = x + i;
                int8_t newY = y + j;
                
                // Manejo de bordes toroidales
                if(newX < 0) newX = LED_WIDTH - 1;
                if(newX >= LED_WIDTH) newX = 0;
                if(newY < 0) newY = LED_HEIGHT - 1;
                if(newY >= LED_HEIGHT) newY = 0;
                
                if(lifeGrid[newY][newX]) count++;
            }
        }
        return count;
    }

    void updateLife() {
        if (currentEffect != LIFE) return;
        if (lifeSpeed == 0) return;  // Pausa

        const unsigned long currentMillis = millis();
        const unsigned long interval = LIFE_BASE_INTERVAL / lifeSpeed;
        if (currentMillis - lastLifeUpdate < interval) {
            return;
        }
        lastLifeUpdate = currentMillis;

        // Calcular siguiente generación
        for(uint8_t y = 0; y < LED_HEIGHT; y++) {
            for(uint8_t x = 0; x < LED_WIDTH; x++) {
                uint8_t neighbors = countNeighbors(x, y);
                bool currentCell = lifeGrid[y][x];
                
                if(currentCell && (neighbors < 2 || neighbors > 3)) {
                    nextGrid[y][x] = false;
                }
                else if(!currentCell && neighbors == 3) {
                    nextGrid[y][x] = true;
                }
                else {
                    nextGrid[y][x] = currentCell;
                }
            }
        }

        // Actualizar grid y mostrar
        bool hasChange = false;
        for(uint8_t y = 0; y < LED_HEIGHT; y++) {
            for(uint8_t x = 0; x < LED_WIDTH; x++) {
                if(lifeGrid[y][x] != nextGrid[y][x]) hasChange = true;
                lifeGrid[y][x] = nextGrid[y][x];
                leds[xy(x, y)] = lifeGrid[y][x] ? lifeColor : CRGB::Black;
            }
        }

        // Solo reiniciar si no es un patrón estático y no hay cambios
        if(!hasChange && autoRestart) {
            initLife();
        }
    }

    void initClock() {
        if (!timeInitialized) {
            timeClient.begin();
            timeClient.setTimeOffset(-6 * 3600); // UTC-6 para CDMX
            timeInitialized = true;
        }
    }

    void updateClock() {
        if (currentEffect != CLOCK) return;
        
        timeClient.update();
        
        FastLED.clear();
        
        // Obtener y mostrar la hora
        int hours = timeClient.getHours();
        int minutes = timeClient.getMinutes();
        
        if (hours > 12) hours -= 12;
        if (hours == 0) hours = 12;
        
        String timeStr = (hours < 10 ? "0" : "") + String(hours) + ":" + 
                    (minutes < 10 ? "0" : "") + String(minutes);

        // Calcular posición central
        // Ancho total = (4 dígitos * 4 espacios) + (2 espacios para los dos puntos) = 18 pixels
        int totalWidth = (4 * 4) + 2;  // 4 dígitos de 4 pixels cada uno + 2 pixels para ':'
        int startX = (LED_WIDTH - totalWidth) / 2;

        drawTime(timeStr, startX, 3, CLOCK_COLOR);  // Usando color blanco
        
        // Mostrar el pasaje bíblico en cian
        String passageStr = String(book) + ":" + 
                          String(chapter) + ":" + 
                          String(verse);
        drawPassage(passageStr, 3, LED_HEIGHT - 8, PASSAGE_COLOR);  // Añadido el parámetro de color
        
        FastLED.show();
    }

    // Función para dibujar la hora
    void drawTime(String time, int x, int y, CRGB color) {
        int xOffset = x;
        for (char c : time) {
            if (c >= '0' && c <= '9') {
                drawDigit(c, xOffset, y, color);
                xOffset += 4;  // Espacio entre dígitos
            } else if (c == ':') {
                drawColon(xOffset, y, color);
                xOffset += 2;  // Espacio para los dos puntos
            }
        }
    }

    // Función para dibujar dígitos grandes (hora)
    void drawDigit(char digit, int x, int y, CRGB color) {
        int idx = digit - '0';
        if (idx < 0 || idx > 9) return;

        for (int dy = 0; dy < 5; dy++) {
            for (int dx = 0; dx < 3; dx++) {
                if (DIGIT_PATTERNS[idx][dy][dx]) {
                    int pixelX = x + dx;
                    int pixelY = y + dy;
                    if (pixelX < LED_WIDTH && pixelY < LED_HEIGHT) {
                        leds[clockXY(pixelX, pixelY)] = color;  // Usar clockXY
                    }
                }
            }
        }
    }

    // Función para dibujar dígitos pequeños (pasaje bíblico)
    void drawMiniDigit(char digit, int x, int y, CRGB color) {
        int idx = digit - '0';
        if (idx < 0 || idx > 9) return;

        for (int dy = 0; dy < 3; dy++) {
            for (int dx = 0; dx < 2; dx++) {
                if (MINI_DIGITS[idx][dy][dx]) {
                    int pixelX = x + dx;
                    int pixelY = y + dy;
                    if (pixelX < LED_WIDTH && pixelY < LED_HEIGHT) {
                        leds[clockXY(pixelX, pixelY)] = color;  // Usar clockXY
                    }
                }
            }
        }
    }

    void drawColon(int x, int y, CRGB color) {
        // Posicionar los dos puntos centrados verticalmente con respecto a los dígitos
        // Los dígitos son de 5 pixels de alto, así que ponemos los puntos en y+1 y y+3
        int y1 = y + 1;
        int y2 = y + 3;
        
        if (y1 >= 0 && y1 < LED_HEIGHT) {
            leds[clockXY(x, y1)] = color;
        }
        if (y2 >= 0 && y2 < LED_HEIGHT) {
            leds[clockXY(x, y2)] = color;
        }
    }

    void drawPassage(String passage, int x, int y, CRGB color) {
    int xOffset = x;
    for (char c : passage) {
        if (c >= '0' && c <= '9') {
            drawMiniDigit(c, xOffset, y, color);
            xOffset += 3;  // Espacio entre dígitos
        } else if (c == ':') {
            // Dibujar un solo punto para separar, más compacto
            if (y + 1 < LED_HEIGHT) {
                int dotY = y + 1;  // Centrar el punto verticalmente
                leds[clockXY(xOffset, dotY)] = color;
            }
            xOffset += 2;  // Menor espacio para el separador
        }
    }
}



    // Métodos para los diferentes efectos rainbow
        void updateRainbowDiagonal() {
        static uint8_t deltaHue = 0;
        
        for(uint8_t x = 0; x < LED_WIDTH; x++) {
            uint8_t columnHue = hue + (x * 255 / LED_WIDTH);
            
            for(uint8_t y = 0; y < LED_HEIGHT; y++) {
                uint8_t finalHue = columnHue + (y * 255 / LED_HEIGHT / 2);
                uint16_t ledIndex = xy(x, y);
                leds[ledIndex] = CHSV(finalHue, saturation, 255);
            }
        }
        hue++;
    }

    void updateRainbowHorizontal() {
        for(uint8_t y = 0; y < LED_HEIGHT; y++) {
            uint8_t rowHue = hue + (y * 255 / LED_HEIGHT);
            for(uint8_t x = 0; x < LED_WIDTH; x++) {
                leds[xy(x, y)] = CHSV(rowHue, saturation, 255);
            }
        }
        hue++;
    }

    void updateRainbowVertical() {
        for(uint8_t x = 0; x < LED_WIDTH; x++) {
            uint8_t columnHue = hue + (x * 255 / LED_WIDTH);
            for(uint8_t y = 0; y < LED_HEIGHT; y++) {
                leds[xy(x, y)] = CHSV(columnHue, saturation, 255);
            }
        }
        hue++;
    }

    void updateRainbowCircular() {
        uint8_t centerX = LED_WIDTH / 2;
        uint8_t centerY = LED_HEIGHT / 2;
        
        for(uint8_t x = 0; x < LED_WIDTH; x++) {
            for(uint8_t y = 0; y < LED_HEIGHT; y++) {
                float distance = sqrt(pow(x - centerX, 2) + pow(y - centerY, 2));
                uint8_t finalHue = hue + (distance * 255 / max(LED_WIDTH, LED_HEIGHT));
                leds[xy(x, y)] = CHSV(finalHue, saturation, 255);
            }
        }
        hue++;
    }

    void updateRainbow() {
        if (rainbowType == "diagonal") {
            updateRainbowDiagonal();
        } else if (rainbowType == "horizontal") {
            updateRainbowHorizontal();
        } else if (rainbowType == "vertical") {
            updateRainbowVertical();
        } else if (rainbowType == "circular") {
            updateRainbowCircular();
        } else {
            updateRainbowDiagonal();
        }
    }

    // Función para convertir coordenadas x,y a índice LED
    uint16_t xy(uint8_t x, uint8_t y) {
        uint16_t i;
        if (y & 0x01) { // Filas impares
            i = (y * LED_WIDTH) + (LED_WIDTH - 1 - x);
        } else {        // Filas pares
            i = (y * LED_WIDTH) + x;
        }
        return i;
    }

    uint16_t clockXY(uint8_t x, uint8_t y) {
        // Invertir coordenadas para rotación 180°
        //x = LED_WIDTH - 1 - x;
        y = LED_HEIGHT - 1 - y;
        return xy(x, y);
    }

    // Método para inicializar el fuego
        void initFire() {
        const uint8_t centerStart = (uint8_t)(LED_WIDTH * 0.15);
        const uint8_t centerEnd = (uint8_t)(LED_WIDTH * 0.85);
        
        for(uint8_t x = 0; x < LED_WIDTH; x++) {
            if (x >= centerStart && x <= centerEnd) {
                firePixels[xy(x, 0)] = PALETTE_SIZE - 1;
            } else {
                uint8_t distanceFromCenter = min((int)abs(x - centerStart), (int)abs(x - centerEnd));
                uint8_t intensity = (4 - distanceFromCenter) > 0 ? (4 - distanceFromCenter) : 0;
                firePixels[xy(x, 0)] = intensity;
            }
        }
    }

    void updateFire() {
        static uint32_t frameCount = 0;
        if (frameCount++ % 2 != 0) return;
        
        for(uint8_t x = 0; x < LED_WIDTH; x++) {
            for(uint8_t y = 1; y < LED_HEIGHT; y++) {
                const uint8_t centerX = LED_WIDTH / 2;
                const uint8_t distanceFromCenter = abs(x - centerX) / (LED_WIDTH / 2);
                
                const uint8_t decay = random(2.1);
                int8_t drift = random(3) - 1;
                
                if (x < LED_WIDTH * 0.2) {
                    drift = drift < 0 ? 0 : drift;
                } else if (x > LED_WIDTH * 0.8) {
                    drift = drift > 0 ? 0 : drift;
                }
                
                int16_t newX = x + drift;
                newX = constrain(newX, 0, LED_WIDTH - 1);
                
                uint16_t belowIndex = xy(x, y-1);
                uint16_t targetIndex = xy(newX, y);
                
                int16_t value = firePixels[belowIndex];
                if(value > decay) {
                    value -= decay;
                } else {
                    value = 0;
                }
                
                if (random(10) == 0 && value > 0) {
                    value += random(3);
                    if(value >= PALETTE_SIZE) value = PALETTE_SIZE - 1;
                }
                
                firePixels[targetIndex] = value;
            }
        }

        for(uint8_t y = 0; y < LED_HEIGHT; y++) {
            for(uint8_t x = 0; x < LED_WIDTH; x++) {
                uint16_t index = xy(x, y);
                uint8_t colorIndex = firePixels[index];
                leds[index] = firePalettes[currentFirePalette][colorIndex];
            }
        }
    }

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

public:
    LedManager() : 
        currentEffect(FIRE), 
        brightness(MAX_BRIGHTNESS), 
        isOn(true), 
        lastUpdate(0),
        rainbowType("diagonal"),
        currentFirePalette(0),
        timeClient(ntpUDP, "pool.ntp.org", -6 * 3600)  // UTC-6 para CDMX
    {
        FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
        FastLED.setBrightness(brightness);
        memset(firePixels, 0, NUM_LEDS);
        initFire();
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
        if (currentMillis - lastUpdate >= 20) {
            lastUpdate = currentMillis;

            switch (currentEffect) {
                case BREATHING:
                    updateBreathing();
                    break;
                case RAINBOW:
                    updateRainbow();
                    break;
                case FIRE:
                    static bool fireInitialized = false;
                    if (!fireInitialized) {
                        initFire();
                        fireInitialized = true;
                    }
                    updateFire();
                    break;
                case LIFE:
                    static bool lifeInitialized = false;
                    if (!lifeInitialized) {
                        initLife();
                        lifeInitialized = true;
                    }
                    updateLife();
                    break;
                case SOLID:
                    fill_solid(leds, NUM_LEDS, CHSV(hue, saturation, 255));
                    break;
                case OFF:
                    FastLED.clear();
                    break;
                case CLOCK:
                    static bool clockInitialized = false;
                    if (!clockInitialized) {
                        initClock();
                        clockInitialized = true;
                    }
                    updateClock();
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
    handle();
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
        if (state == isOn) return;
        
        isOn = state;
        if (!isOn) {
            FastLED.clear();
            FastLED.show();
            handle();
        } else {
            if (currentEffect == OFF) {
                currentEffect = SOLID;
            }
            FastLED.setBrightness(brightness);
            FastLED.show();
            handle();
        }
    }

    void setHue(uint8_t newHue) {
        hue = newHue;
    }

    void setSaturation(uint8_t newSaturation) {
        saturation = newSaturation;
    }

    void setBook(uint8_t newBook) {
        book = newBook;
    }

    void setChapter(uint8_t newChapter) {
        chapter = newChapter;
    }

    void setVerse(uint8_t newVerse) {
        verse = newVerse;
    }

    void setRainbowType(String type) {
        if (type == "diagonal" || type == "horizontal" || 
            type == "vertical" || type == "circular") {
            rainbowType = type;
        }
    }

    void setFirePalette(uint8_t paletteIndex) {
        if (paletteIndex < 6) {
            currentFirePalette = paletteIndex;
        }
    }

    void setLifePatternFromWeb(uint8_t pattern) {
        setLifePattern(static_cast<LifePattern>(pattern));
    }

    void setAutoRestart(bool enabled) {
        autoRestart = enabled;
    }

    void setLifeSpeed(float speed) {
        for (int i = 0; i < 6; i++) {
            if (abs(SPEED_VALUES[i] - speed) < 0.01) {
                lifeSpeed = SPEED_VALUES[i];
                break;
            }
        }
    }

    float getLifeSpeed() const {
        return lifeSpeed;
    }

    bool getAutoRestart() const {
        return autoRestart;
    }

    uint8_t getCurrentLifePattern() const {
        return static_cast<uint8_t>(currentLifePattern);
    }

    uint8_t getFirePalette() const {
        return currentFirePalette;
    }

    String getRainbowType() const {
        return rainbowType;
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

    uint8_t getSaturation() const {
        return saturation;
    }

    LedEffect getCurrentEffect() const {
        return currentEffect;
    }
};

#endif