#ifndef IMAGEPARSER_H
#define IMAGEPARSER_H

#include <vector>
#include <cstdint>
#include "Config.h" // Incluimos las definiciones de tamaño

class ImageParser {
public:
    ImageParser();
    
    // 1. Verifica si el buffer recibido tiene el tamaño correcto para la Zona A.
    bool isValidImageBuffer(const std::vector<uint8_t>& buffer);
    
    // 2. Proporciona la longitud esperada para la verificación del ESPComms.
    static size_t getExpectedBufferSize();
};

#endif // IMAGEPARSER_H