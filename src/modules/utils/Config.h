// src/modules/utils/Config.h

#ifndef CONFIG_H
#define CONFIG_H

// --- Pines de la pantalla E-Paper (SPI) ---
#define EPAPER_SCK_PIN      10
#define EPAPER_MOSI_PIN     11
#define EPAPER_CS_PIN       12
#define EPAPER_DC_PIN       13  // Data/Command
#define EPAPER_RST_PIN      14  // Reset
#define EPAPER_BUSY_PIN     15  // Busy

// --- Pines de Comunicación con ESP8266 (UART) ---
#define ESP_UART_ID         uart0
#define ESP_TX_PIN          0   // GPIO0 de la Pico para TX al RX del ESP
#define ESP_RX_PIN          1   // GPIO1 de la Pico para RX al TX del ESP
#define ESP_BAUD_RATE       115200

// --- Pin del Sensor DHT11 ---
#define DHT11_DATA_PIN      28

// --- Dimensiones de la Pantalla (WeAct 2.13" 250x122) ---
#define EPAPER_WIDTH        250
#define EPAPER_HEIGHT       122
#define EPAPER_SIZE         (EPAPER_WIDTH * EPAPER_HEIGHT / 8) // Tamaño del buffer en bytes

// --- División de la Pantalla ---
// Mitad Izquierda: 125x122 (Imagen) | Mitad Derecha: 125x122 (Datos)
#define LEFT_WIDTH          (EPAPER_WIDTH / 2) // 125
#define RIGHT_START_X       LEFT_WIDTH         // 125

// --- Credenciales Wi-Fi (Si las almacenas en la Pico para enviarlas al ESP) ---
// NOTA: Es más común configurarlas directamente en el ESP para firmware AT
#define WIFI_SSID           "POCO X5 5G"
#define WIFI_PASSWORD       "tonycool"

#endif // CONFIG_H