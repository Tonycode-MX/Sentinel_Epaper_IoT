// src/modules/e_paper/EPaperDriver.h

#ifndef EPAPERDRIVER_H
#define EPAPERDRIVER_H

#include <stdint.h>
#include <string>
#include "../utils/Config.h"

class EPaperDriver {
public:
    // Constructor
    EPaperDriver();

    // Inicializa los pines SPI, el hardware y la pantalla.
    void init();

    // Borra completamente la pantalla.
    void clearScreen();

    // Duerme la pantalla para minimizar el consumo de energía.
    void sleep();

    // Espera hasta que la pantalla no esté ocupada (Busy pin LOW).
    void waitUntilIdle();

    // Dibuja la imagen en el buffer de la pantalla (Mitad Izquierda).
    void setImageBuffer(const uint8_t* buffer);

    // Dibuja texto e información ambiental (Mitad Derecha).
    void drawInfo(float temp_c, const std::string& time_str, const std::string& ip_str);

    // Actualiza la pantalla con el contenido del buffer interno.
    void updateDisplay();

private:
    // Buffer interno de la pantalla (250x122 / 8 bits = 3812 bytes)
    uint8_t image_buffer[EPAPER_SIZE]; 

    // --- Funciones de bajo nivel ---
    void sendCommand(uint8_t command);
    void sendData(uint8_t data);
    void reset();
    void setLut(const uint8_t* lut); // Para control de tonos/refresco

    // Funciones de dibujo de bajo nivel
    void drawPixel(int x, int y, uint8_t color);
    void drawChar(int x, int y, char character, int size, uint8_t color);
};

#endif // EPAPERDRIVER_H