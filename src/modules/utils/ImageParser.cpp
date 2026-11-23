#include "ImageParser.h"
#include <cstdio>

ImageParser::ImageParser() {
    // Constructor
}

size_t ImageParser::getExpectedBufferSize() {
    // Retorna el tamaño exacto que la imagen de 1-bit debe tener.
    return EPD_IMAGE_BUFFER_SIZE;
}

bool ImageParser::isValidImageBuffer(const std::vector<uint8_t>& buffer) {
    size_t expected = getExpectedBufferSize();
    
    if (buffer.size() < expected) {
        printf("ImageParser ERROR: Buffer de imagen demasiado pequeño. Recibido: %zu, Esperado: %zu\n", 
               buffer.size(), expected);
        return false;
    }
    
    if (buffer.size() > expected) {
        // Esto podría indicar un problema de protocolo (datos basura al final)
        printf("ImageParser WARNING: Buffer de imagen demasiado grande. Recibido: %zu, Esperado: %zu. Se truncará.\n", 
               buffer.size(), expected);
    }
    
    return true;
}