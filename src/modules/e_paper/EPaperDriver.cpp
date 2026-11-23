// src/modules/e_paper/EPaperDriver.cpp

#include "EPaperDriver.h"
#include "../utils/Config.h" // Pines, dimensiones
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <string.h>
#include <cmath>

// --- Definiciones de Comandos E-Paper (Genéricas) ---
// NOTA: Estos son ejemplos. Deben ser ajustados según el datasheet.
#define DRIVER_OUTPUT_CONTROL   0x01
#define DATA_ENTRY_MODE         0x03
#define SET_RAM_X_START_END     0x44
#define SET_RAM_Y_START_END     0x45
#define WRITE_RAM               0x24
#define DISPLAY_UPDATE_CONTROL  0x22
#define ACTIVATE_DISPLAY        0x20
#define VCOM_AND_DATA_INTERVAL  0x2B
#define TERMINATE_FRAME_READ    0xFF // Comando de Sleep/Standby

// --- Configuración de SPI ---
#define EPAPER_SPI_ID           spi0
#define EPAPER_SPI_SPEED        4000000 // 4MHz

// Fuentes Simples (Para el texto en la mitad derecha)
// Solo una fuente simple 5x7 como ejemplo
extern const uint8_t font5x7[]; 

// ----------------------------------------------------------------------
// Constructor y Funciones de Bajo Nivel
// ----------------------------------------------------------------------

EPaperDriver::EPaperDriver() {
    // Inicializar el buffer a blanco (0xFF en 1bpp, donde 1 es blanco)
    memset(image_buffer, 0xFF, EPAPER_SIZE); 
}

void EPaperDriver::waitUntilIdle() {
    // El pin BUSY está LOW cuando el e-paper está listo.
    while(gpio_get(EPAPER_BUSY_PIN) == 1) { 
        sleep_ms(1);
    }
}

void EPaperDriver::sendCommand(uint8_t command) {
    gpio_put(EPAPER_DC_PIN, 0); // DC LOW = Command
    gpio_put(EPAPER_CS_PIN, 0); // CS LOW = Select
    spi_write_blocking(EPAPER_SPI_ID, &command, 1);
    gpio_put(EPAPER_CS_PIN, 1); // CS HIGH = Deselect
}

void EPaperDriver::sendData(uint8_t data) {
    gpio_put(EPAPER_DC_PIN, 1); // DC HIGH = Data
    gpio_put(EPAPER_CS_PIN, 0); // CS LOW = Select
    spi_write_blocking(EPAPER_SPI_ID, &data, 1);
    gpio_put(EPAPER_CS_PIN, 1); // CS HIGH = Deselect
}

void EPaperDriver::reset() {
    gpio_put(EPAPER_RST_PIN, 0);
    sleep_ms(20);
    gpio_put(EPAPER_RST_PIN, 1);
    sleep_ms(20);
}

/**
 * @brief Inicializa los pines, el hardware SPI y configura el e-paper.
 */
void EPaperDriver::init() {
    // 1. Inicialización de GPIO
    gpio_init(EPAPER_CS_PIN);
    gpio_init(EPAPER_DC_PIN);
    gpio_init(EPAPER_RST_PIN);
    gpio_init(EPAPER_BUSY_PIN);
    gpio_set_dir(EPAPER_CS_PIN, GPIO_OUT);
    gpio_set_dir(EPAPER_DC_PIN, GPIO_OUT);
    gpio_set_dir(EPAPER_RST_PIN, GPIO_OUT);
    gpio_set_dir(EPAPER_BUSY_PIN, GPIO_IN);

    gpio_put(EPAPER_CS_PIN, 1); // Deseleccionar por defecto
    
    // 2. Inicialización de SPI
    spi_init(EPAPER_SPI_ID, EPAPER_SPI_SPEED);
    gpio_set_function(EPAPER_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(EPAPER_MOSI_PIN, GPIO_FUNC_SPI);
    
    // 3. Reset y Configuración (Secuencia específica del datasheet)
    reset();
    
    // Configuración de inicialización básica (DEBE ser adaptada)
    sendCommand(DRIVER_OUTPUT_CONTROL);
    sendData(0xF9); // Ejemplo para 250x122
    // ... Más comandos de configuración ...

    clearScreen(); // Limpiar el buffer RAM en la pantalla
}

/**
 * @brief Envía el buffer de imagen al área de RAM del e-paper.
 * La imagen cubre la mitad izquierda (0 a 124).
 * @param buffer Puntero al buffer de la imagen pre-procesada (125x122 / 8 bytes).
 */
void EPaperDriver::setImageBuffer(const uint8_t* buffer) {
    // E-paper maneja un solo buffer, por lo que copiamos los datos al área de la imagen.
    // Asumimos que el buffer de entrada es solo la mitad de la imagen: 125x122 / 8 = 1907.5 bytes.
    // Usaremos el tamaño del buffer total (EPAPER_SIZE) para mayor seguridad.

    // 1. Dibuja la imagen en la mitad izquierda del buffer interno
    // NOTA: Esto asume que el buffer de entrada 'buffer' es para el área izquierda.
    // Si el ESP envía SOLO la imagen (125x122), necesitamos copiarla al lugar correcto.
    size_t image_half_size = (LEFT_WIDTH * EPAPER_HEIGHT) / 8; // 1907 bytes
    memcpy(image_buffer, buffer, image_half_size); 
}

/**
 * @brief Dibuja texto e información ambiental en la mitad derecha del buffer interno.
 */
void EPaperDriver::drawInfo(float temp_c, const std::string& time_str, const std::string& ip_str) {
    // Iniciar dibujo en la mitad derecha (X >= 125)
    int start_x = RIGHT_START_X + 5; // 130
    int start_y = 10;
    int line_spacing = 15;

    // 1. Temperatura
    std::string temp_str = "Temp: " + std::to_string(static_cast<int>(std::round(temp_c))) + " C";
    // Usamos 'drawText' (implementada simplificada más abajo)
    // drawText(start_x, start_y, temp_str, 2, 0x00); // Tamaño 2, color negro (0x00)

    // Placeholder de dibujo:
    printf("Dibujando info: %s | %s | %s\n", temp_str.c_str(), time_str.c_str(), ip_str.c_str());
    
    // 2. Hora y Fecha
    // drawText(start_x, start_y + line_spacing * 2, time_str, 2, 0x00);
    
    // 3. IP Address
    // drawText(start_x, start_y + line_spacing * 4, "IP:", 2, 0x00);
    // drawText(start_x, start_y + line_spacing * 5, ip_str, 2, 0x00);
    
    // NOTA: La implementación de 'drawText' y 'drawChar' se deja como ejercicio
    // avanzado, ya que requiere la definición de la fuente y la manipulación
    // del buffer de bits. Aquí simplemente marcamos el área.
}

/**
 * @brief Transfiere el buffer de imagen completo a la RAM del e-paper y actualiza la pantalla.
 */
void EPaperDriver::updateDisplay() {
    waitUntilIdle(); // Esperar antes de empezar la transferencia
    
    // 1. Comando de escritura de RAM
    sendCommand(WRITE_RAM);
    
    // 2. Transferir el buffer completo (Mitad Izquierda + Mitad Derecha)
    for (int i = 0; i < EPAPER_SIZE; i++) {
        sendData(image_buffer[i]);
    }
    
    // 3. Comando de actualización del display
    sendCommand(DISPLAY_UPDATE_CONTROL); 
    sendData(0xC7); // Ejemplo de parámetro: Full update
    
    sendCommand(ACTIVATE_DISPLAY); // Activa el proceso de refresco
    waitUntilIdle(); // Esperar a que termine el refresco
}

void EPaperDriver::clearScreen() {
    // 1. Escribir 'Blanco' (0xFF) en toda la RAM
    sendCommand(WRITE_RAM);
    for (int i = 0; i < EPAPER_SIZE; i++) {
        sendData(0xFF); // 0xFF para Blanco
    }
    
    // 2. Actualizar la pantalla
    updateDisplay();
    
    // También limpiamos el buffer interno
    memset(image_buffer, 0xFF, EPAPER_SIZE);
}

void EPaperDriver::sleep() {
    // Comando para poner la pantalla en modo de bajo consumo
    sendCommand(TERMINATE_FRAME_READ); 
    sendData(0x01); // Parámetro de sleep
}