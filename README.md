# Sentinel-Epaper-IoT


This project was developed for the final project of the IoT course as part of my academic exchange at the Universidad Católica de Colombia (Bogotá, Colombia). ...

## Repository Structure
```
Sentinel-Epaper-IoT/
│
├── api/
│   ├── api_server.py         # Code for your external API (e.g., Flask/Python)
│   └── requirements.txt      # API dependencies (e.g., Flask, Pillow)
│
├── build/                    # Output directory for compiled binaries and objects (Ignore this)
│
├── src/
│   ├── modules/
│   │   ├── comms/
│   │   │   ├── ESPComms.cpp        # Data send/receive logic (commands and buffers)
│   │   │   └── ESPComms.h          # UART communication interface with the ESP8266
│   │   ├── e_paper/
│   │   │   ├── EPaperDriver.cpp    # Implementation of drawing functions (SPI/I2C)
│   │   │   └── EPaperDriver.h      # E-Paper interface and command definitions
│   │   ├── sensors/
│   │   │   ├── TempSensor.cpp      # Reading logic and data conversion
│   │   │   └── TempSensor.h        # Temperature sensor interface (e.g., DHT22/BMP280)
│   │   └── utils/
│   │       ├── Config.h            # Pin definitions, addresses, and constants
│   │       ├── ImageParser.cpp     # (May include bit-mapping functions)
│   │       └── ImageParser.h       # Logic for managing the received image buffer
│   │
│   └── main.cpp                    # Main loop and system control flow
│
├── .gitattributes            # Configuration for Git attribute settings (e.g., line endings, diffs)
├── .gitignore                # Specifies intentionally untracked files to ignore (e.g., 'build/', compiled files)
├── CMakeLists.txt            # Main build configuration (UF2)
└── README.md                 # Project overview, setup instructions, and usage guide
```