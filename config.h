#ifndef CONFIG_H
#define CONFIG_H

// Credenciales WiFi
const char* ssid = "Totalplay-E4A7";
const char* password = "E4A7E89Fn4vAVM78";

// Configuración OTA
const char* OTA_HOSTNAME = "Chimenea-OTA";
const char* OTA_PASSWORD = "admin3765";
const int OTA_PORT = 3232;

// Configuración Alexa
const char* ALEXA_DEVICE_NAME = "LED Prueba";
const int ALEXA_PORT = 80;  

// Configuración LED WS2812B
const int LED_PIN = 2;           // Pin de datos para WS2812B
//const int LED_WIDTH = 27;         // Ancho de la matriz
//const int LED_HEIGHT = 16;        // Alto de la matriz
//const int NUM_LEDS = LED_WIDTH * LED_HEIGHT;  // Total de LEDs
const int NUM_LEDS = 702;
const int MAX_BRIGHTNESS = 255;   // Brillo máximo

// Intervalos de tiempo (en millisegundos)
const long WIFI_CHECK_INTERVAL = 30000;     // Intervalo para verificar WiFi (30 segundos)
const int WIFI_RETRY_DELAY = 5000;          // Tiempo entre intentos de reconexión (5 segundos)
const long SYSTEM_INFO_INTERVAL = 300000;   // Intervalo para imprimir info del sistema (5 minutos)
const int ERROR_RETRY_DELAY = 5000;         // Tiempo antes de reiniciar por error (5 segundos)

// Parámetros de configuración
const int MAX_WIFI_RETRIES = 5;             // Número máximo de intentos de reconexión WiFi
const int SERIAL_BAUD_RATE = 115200;        // Velocidad del puerto serial

// Estados del dispositivo
enum DeviceState {
    INITIALIZING,
    CONNECTING_WIFI,
    RUNNING,
    UPDATING_OTA,
    ERROR
};

enum AlexaDevices {
    DEVICE_MAIN = 0,
    // DEVICE_SECONDARY = 1,
};

// Efectos disponibles
enum LedEffect {
    SOLID,
    BREATHING,
    RAINBOW,
    FIRE,
    OFF
};

// Mensajes del sistema (facilita la internacionalización)
namespace SystemMessages {
    const char* const STARTING = "Iniciando sistema...";
    const char* const WIFI_CONNECTING = "Conectando a WiFi...";
    const char* const WIFI_CONNECTED = "WiFi conectado";
    const char* const WIFI_LOST = "Conexión WiFi perdida";
    const char* const WIFI_RECONNECTING = "Intentando reconectar WiFi...";
    const char* const WIFI_RECONNECTED = "WiFi reconectado";
    const char* const OTA_INITIALIZED = "OTA inicializado";
    const char* const OTA_START = "Iniciando actualización ";
    const char* const OTA_COMPLETE = "\nActualización completada";
    const char* const ERROR_RECOVERY = "Intentando recuperar de error...";
    
    // Mensajes de error OTA
    const char* const OTA_AUTH_ERROR = "Error de Autenticación";
    const char* const OTA_BEGIN_ERROR = "Error de Inicio";
    const char* const OTA_CONNECT_ERROR = "Error de Conexión";
    const char* const OTA_RECEIVE_ERROR = "Error de Recepción";
    const char* const OTA_END_ERROR = "Error de Finalización";
    const char* const OTA_UNKNOWN_ERROR = "Error Desconocido";
}

// Formato para la información del sistema
namespace SystemInfo {
    const char* const HEADER = "\n--- Información del Sistema ---";
    const char* const FOOTER = "---------------------------\n";
    const char* const STATE_FORMAT = "Estado actual: %d\n";
    const char* const SSID_FORMAT = "SSID: %s\n";
    const char* const HOSTNAME_FORMAT = "Hostname OTA: %s\n";
    const char* const IP_FORMAT = "Dirección IP: %s\n";
    const char* const MAC_FORMAT = "MAC Address: %s\n";
    const char* const RSSI_FORMAT = "RSSI: %d dBm\n";
    const char* const UPTIME_FORMAT = "Tiempo encendido: %lu segundos\n";
}

#endif // CONFIG_H