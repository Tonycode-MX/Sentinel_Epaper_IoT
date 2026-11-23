// src/main.cpp

#include "pico/stdlib.h"
#include <string>
#include <stdio.h>

#include "modules/utils/Config.h"
#include "modules/comms/ESPComms.h"
#include "modules/e_paper/EPaperDriver.h"
#include "modules/sensors/TempSensor.h"

// Variables globales para los componentes
ESPComms comms;
EPaperDriver epaper;
TempSensor tempSensor;

// Buffer para almacenar la imagen recibida (Mitad Izquierda, 1907 bytes)
uint8_t image_half_buffer[(LEFT_WIDTH * EPAPER_HEIGHT) / 8];
std::string current_ip_address = "";

/**
 * @brief Inicializa todos los componentes del sistema.
 */
void initialize_system() {
    stdio_init_all();
    sleep_ms(2000); // Dar tiempo para iniciar el terminal

    printf("--- Sentinel E-paper IoT: Iniciando ---\n");
    
    // 1. Inicializar E-Paper
    epaper.init();
    epaper.clearScreen();
    
    // 2. Inicializar Sensor de Temperatura
    tempSensor.init();
    
    // 3. Inicializar Comunicaciones (UART)
    comms.init();
    
    // 4. Conectar ESP8266 al WiFi
    if (comms.connectToWiFi(current_ip_address)) {
        printf("Conexion WiFi OK. IP: %s\n", current_ip_address.c_str());
        
        // 5. Iniciar Servidor Web en el ESP8266 (Puerto 80)
        if (comms.startServer()) {
            printf("Servidor ESP8266 iniciado en el puerto 80.\n");
        } else {
            printf("ERROR: Fallo al iniciar el servidor en el ESP8266.\n");
        }
    } else {
        printf("ERROR: Fallo al conectar el ESP8266 a WiFi.\n");
    }
}

/**
 * @brief Realiza la actualización de la pantalla con los datos y la imagen.
 */
void update_display_content() {
    printf("Actualizando contenido de la pantalla...\n");

    // 1. Obtener datos ambientales
    TempSensor::DHTData data = tempSensor.readData();
    float temp_c = data.valid ? data.temperature_c : 99.9f;

    // 2. Obtener hora/fecha (asumiendo que ESP la proporciona)
    std::string time_str = comms.getTimeAndDate(); 
    
    // 3. Actualizar la mitad derecha con datos y IP
    epaper.drawInfo(temp_c, time_str, current_ip_address);

    // 4. El buffer de la imagen ya debe estar cargado en epaper.image_buffer
    //    después de una llamada a update_image_from_server().
    
    // 5. Refrescar la pantalla
    epaper.updateDisplay();
    printf("Actualizacion completada.\n");
}


bool get_image_from_server() {
    const char* api_host = "192.168.1.73"; // IP de tu PC (DEBE SER CORRECTA)
    const int api_port = 5000;              // Puerto de la API
    
    // 1. Establecer conexión TCP con el servidor API
    std::string connect_cmd = "AT+CIPSTART=0,\"TCP\",\"";
    connect_cmd += api_host;
    connect_cmd += "\",";
    connect_cmd += std::to_string(api_port);
    connect_cmd += "\r\n";
    
    if (!comms.sendATCommand(connect_cmd, "OK", 5000)) {
        printf("Error CIPSTART: No se pudo conectar a la API.\n");
        return false;
    }
    
    // 2. Enviar solicitud HTTP GET
    std::string get_request = "GET / HTTP/1.1\r\nHost: ";
    get_request += api_host;
    get_request += "\r\n\r\n";
    
    // Notificar al ESP la longitud de la solicitud
    std::string send_len_cmd = "AT+CIPSEND=0,";
    send_len_cmd += std::to_string(get_request.length());
    send_len_cmd += "\r\n";
    
    if (!comms.sendATCommand(send_len_cmd, ">", 1000)) {
        printf("Error CIPSEND: No se pudo enviar el GET.\n");
        comms.sendATCommand("AT+CIPCLOSE=0\r\n", "CLOSED", 100);
        return false;
    }
    
    // Enviar el GET
    comms.sendATCommand(get_request, "+IPD", 10000); // Esperamos la respuesta (+IPD)
    
    // 3. Recibir el buffer de la imagen binaria (Asumiendo que +IPD notifica)
    // NOTA: Esta recepción es compleja y requeriría parsing del header HTTP.
    // Aquí usamos la funcion simple que espera la recepción completa.
    // Esto es un punto de fallo potencial que depende del firmware AT.
    
    // Usamos el buffer temporal 'image_half_buffer'
    if (comms.receiveImageBuffer(image_half_buffer, sizeof(image_half_buffer))) {
        epaper.setImageBuffer(image_half_buffer);
        printf("Buffer de imagen cargado en el driver.\n");
    } else {
        printf("Fallo al recibir buffer de imagen de la API.\n");
    }
    
    // 4. Cerrar la conexión
    comms.sendATCommand("AT+CIPCLOSE=0\r\n", "CLOSED", 1000);
    return true;
}

// ...

int main() {
    initialize_system();

    uint32_t last_update_time = to_ms_since_boot(get_absolute_time());
    const uint32_t minute_ms = 60 * 1000; 
    
    // **INTENTA OBTENER LA IMAGEN AL INICIO**
    get_image_from_server(); 

    while (true) {
        
        // --- Opción 1: Actualización por Tiempo (cada minuto) ---
        if (to_ms_since_boot(get_absolute_time()) - last_update_time >= minute_ms) {
            // Actualizar datos ambientales
            update_display_content(); 
            // Vuelve a solicitar la imagen del servidor al mismo tiempo que los datos.
            get_image_from_server();
            
            last_update_time = to_ms_since_boot(get_absolute_time());
        }

        // --- Opción 2: Actualización por Evento (Servidor AT) ---
        // Aquí la Pico podría detectar una conexión HTTP en el puerto 80, 
        // pero la lógica de subida es demasiado compleja para AT.
        // Si el usuario simplemente visita la IP, la Pico lo detectará.
        std::string request = comms.handleWebServerRequest();
        if (!request.empty()) {
            printf("Solicitud HTTP detectada. Refrescando datos/imagen.\n");
            // Cierra la conexión del cliente (Conexion 0)
            comms.sendATCommand("AT+CIPCLOSE=0\r\n", "CLOSED", 1000); 
            // Forzar una nueva solicitud para obtener la imagen
            get_image_from_server();
            update_display_content(); 
        }

        sleep_ms(100); 
    }

    return 0;
}