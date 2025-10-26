# Mínima versión requerida de CMake
cmake_minimum_required(VERSION 3.13)

# Incluir el SDK del Pico
include(pico_sdk_init)

# Nombre del ejecutable (.uf2)
project(Sentinel_Epaper_IoT C CXX ASM)

# Inicializar el SDK del Pico
pico_sdk_init()

# Definir el ejecutable a construir
add_executable(Sentinel_Epaper_IoT
    src/main.cpp
    # Módulos del driver E-Paper
    src/modules/e_paper/EPaperDriver.cpp
    # Módulos del sensor
    src/modules/sensors/TempSensor.cpp
    # Módulos de comunicación
    src/modules/comms/ESPComms.cpp
    # Módulos de utilidades
    src/modules/utils/ImageParser.cpp
)

# Enlazar las librerías necesarias del SDK
target_link_libraries(Sentinel_Epaper_IoT
    pico_stdlib
    hardware_spi        # Necesario para el E-Paper
    hardware_i2c        # Necesario para el sensor I2C
    hardware_uart       # Necesario para la comunicación con el ESP8266
)

# Configuración de propiedades para la compilación
pico_enable_stdio_usb(Sentinel_Epaper_IoT 1) # Habilitar serial sobre USB para debugging
pico_enable_stdio_uart(Sentinel_Epaper_IoT 0) # Deshabilitar UART para usarlo con el ESP8266

# Crear el archivo UF2 para flashear
pico_add_extra_outputs(Sentinel_Epaper_IoT)