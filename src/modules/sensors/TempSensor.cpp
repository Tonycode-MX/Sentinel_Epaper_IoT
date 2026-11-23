// src/modules/sensors/TempSensor.cpp

#include "TempSensor.h"
#include "../utils/Config.h" // Para DHT11_DATA_PIN
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>

// Definiciones de tiempo para el protocolo DHT11
#define DHT11_PULSE_TIMEOUT_US 10000 // 10ms

TempSensor::TempSensor() {
    last_data = {0.0f, 0.0f, false};
}

/**
 * @brief Inicializa el pin GPIO para el DHT11 y lo configura como entrada.
 */
void TempSensor::init() {
    gpio_init(DHT11_DATA_PIN);
    gpio_set_dir(DHT11_DATA_PIN, GPIO_IN); // Inicialmente como entrada con pull-up
    gpio_pull_up(DHT11_DATA_PIN);
    printf("Sensor DHT11 inicializado en Pin %d.\n", DHT11_DATA_PIN);
}

/**
 * @brief Envía la señal de inicio (pull-down largo) al DHT11.
 * @return true si la señal fue enviada correctamente.
 */
bool TempSensor::sendStartSignal() {
    // 1. MCU envía start signal: Pin a LOW por 18ms o más
    gpio_set_dir(DHT11_DATA_PIN, GPIO_OUT);
    gpio_put(DHT11_DATA_PIN, 0);
    sleep_ms(20); // 18ms mínimo
    
    // 2. Pin a HIGH por 20-40us, luego vuelve a entrada
    gpio_put(DHT11_DATA_PIN, 1);
    sleep_us(30); // 20-40us
    gpio_set_dir(DHT11_DATA_PIN, GPIO_IN);
    
    return true;
}

/**
 * @brief Espera la respuesta del DHT11 a la señal de inicio.
 * El DHT11 debe responder con LOW (80us) y luego HIGH (80us).
 * @return true si se detectó la secuencia de respuesta esperada.
 */
bool TempSensor::waitForResponse() {
    uint64_t start_time = time_us_64();
    
    // 1. Esperar la caída del DHT11 (80us LOW)
    while (gpio_get(DHT11_DATA_PIN) == 1) {
        if (time_us_64() - start_time > DHT11_PULSE_TIMEOUT_US) {
            printf("DHT11 Error: Timeout esperando LOW.\n");
            return false;
        }
    }
    
    // 2. Esperar la subida del DHT11 (80us HIGH)
    start_time = time_us_64();
    while (gpio_get(DHT11_DATA_PIN) == 0) {
        if (time_us_64() - start_time > DHT11_PULSE_TIMEOUT_US) {
            printf("DHT11 Error: Timeout esperando HIGH.\n");
            return false;
        }
    }
    
    // 3. Esperar la caída para el inicio de la transmisión de datos (Fin del 80us HIGH)
    start_time = time_us_64();
    while (gpio_get(DHT11_DATA_PIN) == 1) {
        if (time_us_64() - start_time > DHT11_PULSE_TIMEOUT_US) {
            printf("DHT11 Error: Timeout esperando el inicio de datos.\n");
            return false;
        }
    }

    return true;
}

/**
 * @brief Lee 8 bits de datos del sensor.
 * Un bit '0' es 50us HIGH + 26-28us LOW.
 * Un bit '1' es 50us HIGH + 70us LOW.
 * @return El byte leído.
 */
uint8_t TempSensor::readByte() {
    uint8_t byte = 0;
    
    for (int i = 0; i < 8; i++) {
        uint64_t start_time;
        
        // 1. Esperar el pulso de 50us LOW (Ignoramos, es el preámbulo)
        start_time = time_us_64();
        while (gpio_get(DHT11_DATA_PIN) == 0) {
            if (time_us_64() - start_time > DHT11_PULSE_TIMEOUT_US) return 0xFF; // Error
        }

        // 2. Esperar el pulso HIGH para medir su duración
        start_time = time_us_64();
        while (gpio_get(DHT11_DATA_PIN) == 1) {
            if (time_us_64() - start_time > DHT11_PULSE_TIMEOUT_US) return 0xFF; // Error
        }
        
        // Medir la duración del pulso HIGH
        uint32_t duration = (uint32_t)(time_us_64() - start_time);
        
        // Si la duración es > 45us, es un bit '1' (cercano a 70us).
        // Si la duración es < 45us, es un bit '0' (cercano a 28us).
        if (duration > 45) {
            byte |= (1 << (7 - i)); // Establecer el bit
        }
    }
    return byte;
}

/**
 * @brief Realiza la secuencia completa de lectura del sensor DHT11.
 * @return La estructura DHTData con los resultados.
 */
TempSensor::DHTData TempSensor::readData() {
    
    // 1. Enviar señal de inicio
    if (!sendStartSignal()) {
        last_data.valid = false;
        return last_data;
    }
    
    // 2. Esperar respuesta del sensor
    if (!waitForResponse()) {
        last_data.valid = false;
        return last_data;
    }

    // 3. Leer 5 bytes de datos (40 bits)
    uint8_t data[5];
    for (int i = 0; i < 5; i++) {
        data[i] = readByte();
    }
    
    // 4. Calcular y verificar el Checksum
    uint8_t checksum_calc = data[0] + data[1] + data[2] + data[3];
    
    if (checksum_calc == data[4]) {
        // Lectura Válida
        last_data.humidity = (float)data[0];    // DHT11 solo da enteros para Humedad
        last_data.temperature_c = (float)data[2]; // DHT11 solo da enteros para Temperatura
        last_data.valid = true;

        printf("DHT11 Lectura OK: Temp=%.0f°C, Hum=%.0f%%\n", last_data.temperature_c, last_data.humidity);
    } else {
        // Checksum Inválido
        printf("DHT11 Error: Checksum incorrecto. Calculado: %d, Recibido: %d\n", checksum_calc, data[4]);
        last_data.valid = false;
    }
    
    // Restaurar el pin a estado de entrada para que el sensor pueda tirar LOW cuando sea necesario.
    gpio_set_dir(DHT11_DATA_PIN, GPIO_IN);
    
    return last_data;
}