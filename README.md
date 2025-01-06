# LedFirePit

A simple LED controller project for ESP32 that combines WiFi control, Alexa integration, and OTA updates to manage LED strips/arrays with multiple effects. This project is designed to run on dual cores for optimal performance.

## Features

- **Multi-Platform Control**
  - Web Interface (Mobile Responsive)
  - Alexa Voice Commands
  - OTA (Over-the-Air) Updates

- **LED Effects**
  - Solid Color
  - Breathing Effect
  - Rainbow Pattern
  - Fire Simulation
  - Brightness Control
  - Color Selection (HSV)
  - Saturation Adjustment

- **Technical Features**
  - Dual Core Implementation
    - Core 0: Network Operations (Web Server, Alexa, OTA)
    - Core 1: LED Animations
  - Real-time Status Updates
  - Thread-safe Operations
  - Responsive Web Interface
  - WiFi Connection Management

## Hardware Requirements

- ESP32 Development Board
- WS2812B LED Strip/Array
- 5V Power Supply (adequate for your LED setup)
- USB Cable for Programming

## Dependencies

- ESP32 Arduino Core
- FastLED Library
- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson
- ESPAlexa

## Installation 

1. Install required libraries in Arduino IDE:
- FastLED
- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson
- ESPAlexa

2. Configure your settings in config.h:

- WiFi credentials
- LED configuration
- OTA parameters
- Alexa device name

## Configuration

Edit config.h to match your setup:


// WiFi Settings
const char* ssid = "Your_SSID";
const char* password = "Your_Password";

// LED Configuration
const int LED_PIN = 2;        // Data pin for WS2812B
const int NUM_LEDS = 702;     // Number of LEDs
const int MAX_BRIGHTNESS = 255;

// OTA Configuration
const char* OTA_HOSTNAME = "Chimenea-OTA";
const char* OTA_PASSWORD = "your_password";
const int OTA_PORT = 3232;

// Alexa Configuration
const char* ALEXA_DEVICE_NAME = "LED Device";
const int ALEXA_PORT = 80;

## Usage

1. Web Interface
- Access through: http://[ESP32_IP]:81
- Control LED state, brightness, effects, and colors
- Real-time status updates

2. Alexa Commands
- "Alexa, turn on [device name]"
- "Alexa, turn off [device name]"
- "Alexa, set [device name] brightness to 50%"

3. OTA Updates
- Access through Arduino IDE
- Select network port in Tools > Port > Network Ports
- Upload as normal

4. Project Structure
- ChimeneaAlexaOtaWebControllers.ino - Main program file
- config.h - Configuration settings
- web_manager.h - Web server and interface management
- led_manager.h - LED effects and control
- alexa_manager.h - Alexa integration
- ota_manager.h - OTA update functionality
- web_interface.h - Web interface HTML/CSS/JavaScript

5. Performance
The dual-core implementation ensures smooth operation:

- Core 0 handles all network-related tasks
- Core 1 is dedicated to LED animations
- Thread-safe operations prevent conflicts
- Responsive web interface with real-time updates

## Port Usage

The project uses different ports for various services:

|
 Port 
|
 Service 
|
 Description 
|
|
------
|
---------
|
-------------
|
|
 80 
|
 Alexa 
|
 Reserved for Alexa discovery and control 
|
|
 81 
|
 Web Interface & REST API 
|
 Main web interface and HTTP API endpoints 
|
|
 3232 
|
 OTA Updates 
|
 Over-the-air firmware updates 
|

Important considerations:
- Port 80 must remain assigned to Alexa for proper device discovery
- Web interface runs on port 81 to avoid conflicts with Alexa
- OTA port can be modified in config.h if needed
- All ports should be allowed in your network firewall for proper functionality

# LedFirePit API Documentation

## Endpoints Overview

All API endpoints are accessible through `http://[ESP32_IP]:81/api/`

| Endpoint | Method | Description |
|----------|---------|-------------|
| `/status` | GET | Get current device state |
| `/state` | POST | Change power state |
| `/brightness` | POST | Adjust brightness |
| `/effect` | POST | Change current effect |
| `/color` | POST | Set color properties |

## Detailed API Reference

### GET /api/status
Returns the current state of all LED parameters.

Response:
{
    "state": boolean,        // true = on, false = off
    "brightness": number,    // 0-255
    "effect": number,       // Current effect index
    "hue": number,         // 0-255
    "saturation": number   // 0-255
}

### POST /api/state
Toggle the LED strip on/off.

Request body:
{
    "state": boolean  // true = on, false = off
}

### POST /api/brightness
Set the LED brightness.

Request body:
{
    "brightness": number  // 0-255
}

### POST /api/effect
Change the current effect.

Request body:
{
    "effect": number  // Effect index
}

Current effect indices:
- 0: Solid
- 1: Breathing
- 2: Rainbow
- 3: Fire
- 4: Off

### POST /api/color
Set color properties (hue and saturation).

Request body:
{
    "hue": number,        // 0-255
    "saturation": number  // 0-255
}

## Adding New Effects

### 1. Update Effect Enum
Add the effect to the `LedEffect` enum in `config.h`:

enum LedEffect {
    SOLID,
    BREATHING,
    RAINBOW,
    FIRE,
    OFF,
    YOUR_NEW_EFFECT  // Add your effect here
};

### 2. Update Web Interface
Add the effect to the select element options in `web_interface.h`:

<select id="effect" onchange="updateEffect(this.value)">
    <option value="0">Sólido</option>
    <option value="1">Respiración</option>
    <option value="2">Arcoiris</option>
    <option value="3">Fuego</option>
    <option value="4">Apagado</option>
    <option value="5">Your New Effect</option>  // Add this line
</select>

Update getEffectName function:
function getEffectName(effect) {
    const effects = ['Sólido', 'Respiración', 'Arcoiris', 'Fuego', 'Apagado', 'Your New Effect'];
    return effects[effect] || 'Desconocido';
}

### 3. Implement Effect
Add the effect implementation in `led_manager.h`:

class LedManager {
private:
    // Add any variables needed for your effect
    void updateYourNewEffect() {
        // Your effect implementation
        // Example:
        for(int i = 0; i < NUM_LEDS; i++) {
            // Effect logic here
            leds[i] = CRGB::Red;  // Example
        }
    }

public:
    void handle() {
        if (!isOn) return;

        portENTER_CRITICAL(&mux);
        switch (currentEffect) {
            // ... existing cases ...
            case YOUR_NEW_EFFECT:
                updateYourNewEffect();
                break;
        }
        FastLED.show();
        portEXIT_CRITICAL(&mux);
    }
};

## Effect Implementation Guidelines

### Thread Safety
- Use `portENTER_CRITICAL(&mux)` and `portEXIT_CRITICAL(&mux)` for shared resource access
- Keep critical sections as short as possible

### Performance
- Avoid blocking operations
- Use FastLED's built-in functions when possible
- Consider using EVERY_N_MILLISECONDS for timing

### Memory Usage
- Declare effect-specific variables in the class
- Use appropriate data types to minimize memory usage

### Example Effect Implementation
class LedManager {
private:
    // Effect-specific variables
    uint8_t wavePosition;
    
    void updateWaveEffect() {
        EVERY_N_MILLISECONDS(50) {
            fadeToBlackBy(leds, NUM_LEDS, 20);
            int pos = beatsin16(13, 0, NUM_LEDS-1) + wavePosition;
            leds[pos % NUM_LEDS] += CHSV(hue, saturation, 255);
            wavePosition = (wavePosition + 1) % NUM_LEDS;
        }
    }
};

## Testing New Effects

### 1. Basic Functionality
- Effect initialization
- Color transitions
- Brightness control
- Power on/off behavior

### 2. Performance
- Memory usage
- CPU usage
- Frame rate consistency

### 3. Integration
- Web interface control
- Status updates
- Effect switching

## Common FastLED Functions

### Basic Color Setting
leds[i] = CRGB::Red;                    // Solid color
leds[i] = CHSV(hue, saturation, value); // HSV color

### Color Manipulation
fadeToBlackBy(leds, NUM_LEDS, fade_amount);  // Fade effect
fill_solid(leds, NUM_LEDS, color);           // Fill strip
fill_rainbow(leds, NUM_LEDS, starting_hue);  // Rainbow effect

### Timing Functions
EVERY_N_MILLISECONDS(ms) { }  // Timing control
beatsin16(bpm, low, high);    // Sine wave movement

## Example API Usage

Using curl to control the device:

# Get status
curl http://[ESP32_IP]:81/api/status

# Turn on
curl -X POST -H "Content-Type: application/json" \
     -d '{"state":true}' \
     http://[ESP32_IP]:81/api/state

# Set color
curl -X POST -H "Content-Type: application/json" \
     -d '{"hue":120,"saturation":255}' \
     http://[ESP32_IP]:81/api/color

# Change effect
curl -X POST -H "Content-Type: application/json" \
     -d '{"effect":2}' \
     http://[ESP32_IP]:81/api/effect

## Contributing
Contributions are welcome! Please feel free to submit a Pull Request.

## Support
If you encounter any problems or have questions, please open an issue on GitHub.

------------------------------------------------------------------------------------

Made with ❤️ by [Antonio Robledo]