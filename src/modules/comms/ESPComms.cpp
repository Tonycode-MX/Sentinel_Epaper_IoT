#include "ESPComms.h"
#include "../utils/Config.h" // Para pines y baudios
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

// Define un tamaño máximo para el buffer de respuesta AT
#define RESPONSE_BUFFER_SIZE 256

// --- Funciones Auxiliares ---

// Función para leer bytes disponibles del UART en un buffer
size_t uart_read_bytes(char *buffer, size_t max_len, uint32_t timeout_ms) {
    uint64_t start_time = to_ms_since_boot(get_absolute_time());
    size_t count = 0;
    
    // Esperar hasta que se cumpla el tiempo de espera o se llene el buffer
    while (to_ms_since_boot(get_absolute_time()) - start_time < timeout_ms && count < max_len - 1) {
        if (uart_is_readable(ESP_UART_ID)) {
            buffer[count++] = uart_getc(ESP_UART_ID);
        }
    }
    buffer[count] = '\0'; // Asegurar terminación nula
    return count;
}


// --- Implementación de Métodos de la Clase ESPComms ---

/**
 * @brief Inicializa el hardware UART para la comunicación con el ESP8266.
 */
void ESPComms::init() {
    uart_init(ESP_UART_ID, ESP_BAUD_RATE);
    gpio_set_function(ESP_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(ESP_RX_PIN, GPIO_FUNC_UART);

    printf("UART para ESP8266 inicializado en Baudios: %d\n", ESP_BAUD_RATE);
}

/**
 * @brief Envía un comando AT, espera una respuesta y verifica si contiene una cadena esperada.
 * @param command El comando AT a enviar (ej: "AT+CWMODE=1\r\n").
 * @param expected_response La subcadena esperada en la respuesta (ej: "OK").
 * @param timeout_ms El tiempo de espera máximo en milisegundos.
 * @return true si se recibió la respuesta esperada, false en caso contrario.
 */
bool ESPComms::sendATCommand(const std::string& command, const std::string& expected_response, int timeout_ms) {
    char response_buffer[RESPONSE_BUFFER_SIZE] = {0};
    
    // 1. Limpiar cualquier basura en el buffer de RX
    while (uart_is_readable(ESP_UART_ID)) {
        uart_getc(ESP_UART_ID);
    }

    // 2. Enviar el comando
    uart_puts(ESP_UART_ID, command.c_str());
    printf("Comando AT enviado: %s", command.c_str());

    // 3. Esperar la respuesta
    uart_read_bytes(response_buffer, RESPONSE_BUFFER_SIZE, timeout_ms);
    printf("Respuesta recibida:\n%s\n", response_buffer);

    // 4. Verificar la respuesta esperada
    std::string response_str(response_buffer);
    return response_str.find(expected_response) != std::string::npos;
}

/**
 * @brief Conecta el ESP a la red Wi-Fi usando credenciales de Config.h.
 * @param ip_address Referencia para almacenar la dirección IP obtenida.
 * @return true si la conexión fue exitosa y se obtuvo la IP, false en caso contrario.
 */
bool ESPComms::connectToWiFi(std::string& ip_address) {
    // 1. Resetear el módulo para asegurar un estado limpio (AT+RST)
    if (!sendATCommand("AT+RST\r\n", "ready", 2000)) {
        printf("Error: Fallo al reiniciar el ESP.\n");
        return false;
    }

    // 2. Establecer el modo Wi-Fi a Estación (Station Mode)
    if (!sendATCommand("AT+CWMODE=1\r\n", "OK", 1000)) {
        printf("Error: Fallo al establecer el modo Estación.\n");
        return false;
    }
    
    // 3. Conectarse a la red usando las credenciales de Config.h
    std::string connect_cmd = "AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASSWORD "\"\r\n";
    printf("Intentando conectar a Wi-Fi: %s...\n", WIFI_SSID);

    // Esperamos "WIFI GOT IP" o "OK" si ya está conectado (Timeout largo de 20s)
    if (!sendATCommand(connect_cmd, "WIFI GOT IP", 20000)) {
         printf("ERROR: Fallo al conectar o conseguir la IP.\n");
         // Intenta de nuevo por si solo recibio un 'OK' y ya estaba conectado
         if (!sendATCommand("AT\r\n", "OK", 500)) return false; 
    }

    // 4. Obtener la IP local
    char response_buffer[RESPONSE_BUFFER_SIZE] = {0};
    // El comando CIFSR devuelve la IP, debemos leer la respuesta para parsearla.
    if (!sendATCommand("AT+CIFSR\r\n", "OK", 1000)) {
        printf("Error: Fallo al obtener CIFSR.\n");
        return false;
    }
    
    // NOTA: Volvemos a leer el buffer por si la respuesta de CIFSR no fue capturada por sendATCommand
    uart_read_bytes(response_buffer, RESPONSE_BUFFER_SIZE, 100);
    std::string response_str(response_buffer);
    
    size_t ip_start = response_str.find("STAIP,\"");
    
    if (ip_start != std::string::npos) {
        ip_start += 7; 
        size_t ip_end = response_str.find("\"", ip_start);
        if (ip_end != std::string::npos) {
            ip_address = response_str.substr(ip_start, ip_end - ip_start);
            printf("IP Obtenida: %s\n", ip_address.c_str());
            return true;
        }
    }

    printf("Error: Fallo al obtener la IP (Parsing error).\n");
    return false;
}

/**
 * @brief Inicia el servidor TCP/IP en el ESP8266 (Puerto 80).
 */
bool ESPComms::startServer() {
    // 1. Habilitar modo Múltiple de Conexión
    if (!sendATCommand("AT+CIPMUX=1\r\n", "OK", 1000)) {
        printf("Error: Fallo al establecer CIPMUX=1.\n");
        return false;
    }
    
    // 2. Iniciar el servidor en el puerto 80 (AT+CIPSERVER=1,80)
    // El AT-Firmware del ESP8266 no puede manejar servidores web complejos (upload), 
    // pero puede establecer una conexión TCP (servidor) y notificar a la Pico.
    return sendATCommand("AT+CIPSERVER=1,80\r\n", "OK", 2000);
}

/**
 * @brief Espera por una conexión de cliente, maneja la solicitud y notifica la actualizacion.
 * @return La cadena recibida del cliente (e.g., la solicitud HTTP GET).
 */
std::string ESPComms::handleWebServerRequest() {
    char response_buffer[RESPONSE_BUFFER_SIZE * 4] = {0}; // Usar un buffer más grande
    printf("Esperando solicitud web (+IPD) o comando de actualizacion...\n");

    // Bucle de polling para esperar la respuesta del ESP (e.g., "+IPD,0,256:")
    // Timeout de 60 segundos
    size_t len = uart_read_bytes(response_buffer, sizeof(response_buffer), 60000);

    if (len > 0) {
        std::string response_str(response_buffer);
        size_t ipd_pos = response_str.find("+IPD,");

        if (ipd_pos != std::string::npos) {
            // Se encontró una solicitud de datos (por ejemplo, el navegador que se conectó)
            printf("Solicitud Web Recibida:\n%s\n", response_buffer);

            // Devolvemos la cadena para que main.cpp decida si es una subida de imagen
            return response_str; 
        }
    }
    return ""; // No hubo actividad
}

// ... getTimeAndDate() y receiveImageBuffer() a implementar cuando sea necesario
//    (Dependerán del formato exacto de datos que use el ESP)

std::string ESPComms::getTimeAndDate() {
    // Por simplicidad, aquí solo enviaremos un comando para obtener la hora
    // Asumimos que el ESP obtiene la hora por NTP y la guarda en una variable.
    // sendATCommand("AT+NTPTIME?\r\n", "+NTPTIME:", 1000);
    // Luego parsear la respuesta.
    return "10/11/2025 15:43"; // Placeholder
}

bool ESPComms::receiveImageBuffer(uint8_t* buffer, size_t buffer_size) {
    printf("Iniciando recepcion de buffer de imagen (%zu bytes).\n", buffer_size);
    // El ESP debe ser configurado para enviar datos binarios crudos.
    
    // Aquí implementas la lectura byte a byte con un timeout
    for (size_t i = 0; i < buffer_size; ++i) {
        uint64_t start_time = to_ms_since_boot(get_absolute_time());
        // Espera un byte con un timeout corto
        while (!uart_is_readable(ESP_UART_ID)) {
            if (to_ms_since_boot(get_absolute_time()) - start_time > 100) { // 100ms timeout
                printf("Error: Timeout al recibir el byte %zu.\n", i);
                return false;
            }
        }
        buffer[i] = uart_getc(ESP_UART_ID);
    }

    printf("Buffer de imagen recibido correctamente.\n");
    return true;
}