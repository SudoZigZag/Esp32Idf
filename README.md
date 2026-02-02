# ESP32 Project Template

A professional ESP-IDF project template with multi-target support and automated build scripts.

## Features

- ✅ Multi-target build support (ESP32, ESP32-S3, ESP32-C3, etc.)
- ✅ Automated toolchain installation
- ✅ CMake-based build system
- ✅ Easy distribution - clone and build

## Hardware Support

Currently tested on:
- ESP32-S3 WROOM-1 (N16R8) - 16MB Flash, 8MB PSRAM

## Prerequisites

- macOS (tested) / Linux / Windows WSL
- Python 3.10+
- Git

## Quick Start

### 1. Clone and Install
```bash
git clone <your-repo-url>
cd <your-project>

# Install ESP-IDF (one-time setup)
./install.sh

# Or specify custom ESP-IDF path
./install.sh /opt/esp-idf
```

### 2. Set Up Environment

**Every time you open a new terminal:**
```bash
source setup_env.sh
```

### 3. Build
```bash
# Build for specific target
./build.sh -t esp32s3

# Or build for all supported targets
./build.sh -t all
```

### 4. Flash and Monitor
```bash
# Find your serial port
ls /devmacOS
ls /dev/ttyUSB* # Linux

# Flash and monitor (ESP32-S3 via native USB)
idf.py -p /dev/cu.usbmodem-* flash monitor

# Exit monitor: Ctrl + ]
```

## Supported Targets

- `esp32` - ESP32 (Xtensa dual-core)
- `esp32s3` - ESP32-S3 (Xtensa dual-core + USB)
- `esp32s2` - ESP32-S2 (Xtensa single-core + USB)
- `esp32c3` - ESP32-C3 (RISC-V single-core)
- `esp32c6` - ESP32-C6 (RISC-V single-core + WiFi6)

## Project Structure
```
.
├── CMakeLists.txt              # Root project config
├── sdkconfig.defaults          # Default ESP-IDF configuration
├── install.sh                  # ESP-IDF installation script
├── build.sh                    # Multi-target build script
├── main/
│   ├── CMakeLists.txt         # Main component config
│   └── main.cpp               # Application entry point
└── components/                 # Custom components (optional)
```

## Configuration

### Flash Size and PSRAM

Default configuration (in `sdkconfig.defaults`):
- Flash: 16MB
-df.py menuconfig
# Navigate to settings and save
```

### Adding Source Files

Edit `main/CMakeLists.txt`:
```cmake
idf_component_register(SRCS "main.cpp" "new_file.cpp"
                       INCLUDE_DIRS ".")
```

## Troubleshooting

### "cmake not found"
```bash
brew install cmake
```

### "Python 3.10+ not found"
```bash
brew install python@3.12
echo 'export PATH="/opt/homebrew/bin:$PATH"' >> ~/.zshrc
```

### Serial Port Not Found (macOS)

For **ESP32-S3**: Use the native USB port (shows as `/dev/cu.usbmodem-*`)
- No driver needed!
- Connect to the port labeled "USB" (not "UART")

For **other ESP32 variants**: May need CH340 or CP2102 drivers

## Development Workflow
```bash
# 1. Make changes to code
vim main/main.cpp

# 2. Build
./build.sh -t esp32s3

# 3. Flash
idf.py -p /dev/cu.usbmodem-* flash monitor

# 4. Debug via serial output
# Press Ctrl+] to exit monitor
```

## License

[Your License Here]

## Contributing

Pull requests welcome!

## Modular Architecture

This project uses a modular app system. Each feature is a separate application.

### Available Apps

- `app_wifi_basic` - WiFi connection + mDNS
- `app_multi_thread` - Dual-core threading examples  
- `app_http_server` - HTTP web server (coming soon)

### Switching Apps

Edit `main/app_registry.h`:
```cpp
#define ACTIVE_APP 0  // Change this number
```

Then rebuild and flash.

### Adding New Apps

1. Create `main/apps/app_yourname.cpp`
2. Add to `main/app_registry.h`
3. Add to `main/CMakeLists.txt`
4. Build and run!
