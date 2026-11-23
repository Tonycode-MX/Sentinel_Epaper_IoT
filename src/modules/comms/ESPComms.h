// src/modules/comms/ESPComms.h

#ifndef ESPCOMMS_H
#define ESPCOMMS_H

#include <string>

class ESPComms {
public:
    // Inicializa el hardware UART
    void init();

    // Envía un comando AT y espera una respuesta
    bool sendATCommand(const std::string& command, const std::string& expected_response, int timeout_ms);

    // Conecta el ESP a la red Wi-Fi y obtiene la IP
    bool connectToWiFi(std::string& ip_address);

    // Espera un comando específico del servidor web del ESP (ej: "IMAGE_UPDATE")
    bool waitForUpdateCommand();

    // Recibe el buffer de la imagen del ESP
    bool receiveImageBuffer(uint8_t* buffer, size_t buffer_size);
    
    // Obtiene la hora actual del ESP (que a su vez la obtiene por NTP/Servidor)
    std::string getTimeAndDate();

    bool startServer();
};

#endif // ESPCOMMS_H