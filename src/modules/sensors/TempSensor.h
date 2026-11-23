// src/modules/sensors/TempSensor.h

#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H

#include <stdint.h>
#include <vector>

class TempSensor {
public:
    // Estructura para almacenar los datos de lectura del sensor
    struct DHTData {
        float temperature_c; // Temperatura en grados Celsius
        float humidity;      // Humedad relativa
        bool valid;          // Indica si la lectura fue exitosa (checksum válido)
    };

    // Constructor que inicializa el pin del sensor
    TempSensor();

    // Inicializa el pin GPIO
    void init();

    // Realiza la secuencia de lectura del sensor
    DHTData readData();

private:
    // Funciones de bajo nivel para el protocolo one-wire
    bool sendStartSignal();
    bool waitForResponse();
    uint8_t readByte();
    
    // Almacena el último resultado de la lectura
    DHTData last_data;
};

#endif // TEMPSENSOR_H