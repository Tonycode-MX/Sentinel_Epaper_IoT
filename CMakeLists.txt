# Versión mínima de CMake requerida
cmake_minimum_required(VERSION 3.13)

# --- Configuración del SDK de la Pico (Tu método probado) ---
# Importar el SDK usando la variable de entorno
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# La función pico_sdk_init() se define después de la importación
pico_sdk_init()

# --- Declaración del Proyecto ---
# Declarar el proyecto (usamos SentinelEpaper para evitar conflictos de nombres)
project(SentinelEpaper C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# --- Definición de Archivos Fuente ---
set(SOURCE_DIR src)

# Archivos C++ y C (main.cpp y todos los módulos)
file(GLOB PROJECT_SOURCES
    ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/modules/comms/ESPComms.cpp
    ${SOURCE_DIR}/modules/e_paper/EPaperDriver.cpp
    ${SOURCE_DIR}/modules/sensors/TempSensor.cpp
)

# --- Añadir el ejecutable ---

# Crear el nombre del ejecutable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# --- Propiedades y Directorios ---

# Añadir directorios de cabeceras para que los includes funcionen
target_include_directories(${PROJECT_NAME} PUBLIC
    ${SOURCE_DIR}
    ${SOURCE_DIR}/modules/comms
    ${SOURCE_DIR}/modules/e_paper
    ${SOURCE_DIR}/modules/sensors
    ${SOURCE_DIR}/modules/utils
)

# --- Enlazar librerías de Pico ---
# Enlazar las librerías necesarias
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_spi
    hardware_uart
    hardware_gpio
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# Añadir la generación de archivos UF2 y ELF
pico_add_extra_outputs(${PROJECT_NAME})