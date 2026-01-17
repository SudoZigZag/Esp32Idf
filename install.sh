#!/bin/bash

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default ESP-IDF installation path
DEFAULT_IDF_PATH="$HOME/esp/esp-idf"
IDF_INSTALL_PATH="${1:-$DEFAULT_IDF_PATH}"

echo -e "${GREEN}ESP32 Project Setup${NC}"
echo "================================"

# Check if path was provided
if [ "$1" ]; then
    echo "Using custom ESP-IDF path: $IDF_INSTALL_PATH"
else
    echo "Using default ESP-IDF path: $IDF_INSTALL_PATH"
    echo "To use a different path, run: ./install.sh /your/custom/path"
fi

echo ""

# Check if ESP-IDF already exists
if [ -d "$IDF_INSTALL_PATH" ]; then
    echo -e "${YELLOW}ESP-IDF already exists at $IDF_INSTALL_PATH${NC}"
    read -p "Do you want to skip ESP-IDF installation? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Skipping ESP-IDF installation..."
        SKIP_INSTALL=true
    else
        echo "Re-installing ESP-IDF..."
        rm -rf "$IDF_INSTALL_PATH"
        SKIP_INSTALL=false
    fi
else
    SKIP_INSTALL=false
fi

# Install ESP-IDF if needed
if [ "$SKIP_INSTALL" = false ]; then
    echo -e "${GREEN}Installing ESP-IDF to $IDF_INSTALL_PATH${NC}"
    
    # Create parent directory if it doesn't exist
    mkdir -p "$(dirname "$IDF_INSTALL_PATH")"
    
    # Clone ESP-IDF
    echo "Cloning ESP-IDF (this will take a while - ~2.3GB)..."
    git clone --recursive https://github.com/espressif/esp-idf.git "$IDF_INSTALL_PATH"
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to clone ESP-IDF${NC}"
        exit 1
    fi
    
    # Install ESP-IDF tools
    echo "Installing ESP-IDF tools..."
    cd "$IDF_INSTALL_PATH"
    ./install.sh esp32
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to install ESP-IDF tools${NC}"
        exit 1
    fi
    
    cd - > /dev/null
fi

# Create a setup script for this project
SETUP_SCRIPT="setup_env.sh"
cat > "$SETUP_SCRIPT" << EOF
#!/bin/bash
# Source this file to set up ESP-IDF environment
# Usage: source setup_env.sh

# Add Homebrew to PATH (for cmake and other tools)
export PATH="/opt/homebrew/bin:\$PATH"

export IDF_PATH="$IDF_INSTALL_PATH"
source "\$IDF_PATH/export.sh"

echo "ESP-IDF environment ready!"
echo "IDF_PATH: \$IDF_PATH"
echo "CMake: \$(cmake --version | head -n1)"
EOF

chmod +x "$SETUP_SCRIPT"

echo ""
echo -e "${GREEN}Installation complete!${NC}"
echo "================================"
echo ""
echo "Next steps:"
echo "1. Set up environment: ${YELLOW}source setup_env.sh${NC}"
echo "2. Configure target:   ${YELLOW}idf.py set-target esp32${NC}"
echo "3. Build project:      ${YELLOW}idf.py build${NC}"
echo "4. Flash and monitor:  ${YELLOW}idf.py -p PORT flash monitor${NC}"
echo ""
echo "For future terminal sessions, just run: ${YELLOW}source setup_env.sh${NC}"
echo ""