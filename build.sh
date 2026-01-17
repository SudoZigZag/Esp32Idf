#!/bin/bash

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Supported targets
SUPPORTED_TARGETS=("esp32" "esp32s3" "esp32c3" "esp32s2" "esp32c6")

# Function to display usage
usage() {
    echo -e "${BLUE}ESP32 Multi-Target Build Script${NC}"
    echo ""
    echo "Usage: $0 -t <target>"
    echo ""
    echo "Options:"
    echo "  -t <target>    Specify target to build"
    echo "  -h             Show this help message"
    echo ""
    echo "Available targets:"
    echo "  all            Build for all supported targets"
    echo "  esp32          ESP32 (Xtensa dual-core)"
    echo "  esp32s3        ESP32-S3 (Xtensa dual-core + USB)"
    echo "  esp32s2        ESP32-S2 (Xtensa single-core + USB)"
    echo "  esp32c3        ESP32-C3 (RISC-V single-core)"
    echo "  esp32c6        ESP32-C6 (RISC-V single-core + WiFi6)"
    echo ""
    echo "Examples:"
    echo "  $0 -t esp32s3        # Build only for ESP32-S3"
    echo "  $0 -t all            # Build for all targets"
    exit 1
}

# Function to build for a specific target
build_target() {

    local target=$1
    
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}Building for: $target${NC}"
    echo -e "${BLUE}================================${NC}"
    
    # Check if target has changed
    CURRENT_TARGET=$(grep "CONFIG_IDF_TARGET=" sdkconfig 2>/dev/null | cut -d'"' -f2)
    
    if [ "$CURRENT_TARGET" != "$target" ] || [ ! -f "sdkconfig" ]; then
        # Target changed or no config exists - full reconfigure needed
        echo "Setting target to $target..."
        idf.py fullclean > /dev/null 2>&1
        idf.py set-target $target
    else
        # Same target - do incremental build (no cleaning!)
        echo "Target already set to $target, doing incremental build..."
    fi
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Failed to set target $target${NC}"
        return 1
    fi
    
    # Build (CMake will automatically detect what changed)
    echo "Building..."
    idf.py build
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Build failed for $target${NC}"
        return 1
    fi
    
    # Create target-specific binary directory
    mkdir -p binaries/$target
    
    # Copy binaries
    echo "Copying binaries to binaries/$target/..."
    cp build/*.bin binaries/$target/ 2>/dev/null
    cp build/bootloader/bootloader.bin binaries/$target/
    cp build/partition_table/partition-table.bin binaries/$target/
    
    # Copy flash args for easy flashing later
    cp build/flash_args binaries/$target/ 2>/dev/null
    cp build/flasher_args.json binaries/$target/ 2>/dev/null
    
    echo -e "${GREEN}✓ Build complete for $target${NC}"
    echo -e "${GREEN}  Binaries saved to: binaries/$target/${NC}"
    echo ""
    
    return 0
}

# Function to validate target
is_valid_target() {
    local target=$1
    
    if [ "$target" == "all" ]; then
        return 0
    fi
    
    for valid in "${SUPPORTED_TARGETS[@]}"; do
        if [ "$target" == "$valid" ]; then
            return 0
        fi
    done
    
    return 1
}

# Main script
main() {
    local target=""
    
    # Parse command line arguments
    while getopts "t:h" opt; do
        case $opt in
            t)
                target="$OPTARG"
                ;;
            h)
                usage
                ;;
            \?)
                echo -e "${RED}Invalid option: -$OPTARG${NC}" >&2
                usage
                ;;
            :)
                echo -e "${RED}Option -$OPTARG requires an argument${NC}" >&2
                usage
                ;;
        esac
    done
    
    # Check if target was provided
    if [ -z "$target" ]; then
        echo -e "${RED}Error: No target specified${NC}"
        echo ""
        usage
    fi
    
    # Validate target
    if ! is_valid_target "$target"; then
        echo -e "${RED}Error: Invalid target '$target'${NC}"
        echo ""
        usage
    fi
    
    # Check if ESP-IDF environment is set up
    if [ -z "$IDF_PATH" ]; then
        echo -e "${RED}Error: ESP-IDF environment not set up${NC}"
        echo -e "${YELLOW}Run: source setup_env.sh${NC}"
        exit 1
    fi
    
    # Create binaries directory
    mkdir -p binaries
    
    # Build for specified target(s)
    if [ "$target" == "all" ]; then
        echo -e "${YELLOW}Building for all targets...${NC}"
        echo ""
        
        local failed_targets=()
        local success_count=0
        
        for t in "${SUPPORTED_TARGETS[@]}"; do
            if build_target "$t"; then
                ((success_count++))
            else
                failed_targets+=("$t")
            fi
        done
        
        echo -e "${BLUE}================================${NC}"
        echo -e "${BLUE}Build Summary${NC}"
        echo -e "${BLUE}================================${NC}"
        echo -e "${GREEN}Successful builds: $success_count/${#SUPPORTED_TARGETS[@]}${NC}"
        
        if [ ${#failed_targets[@]} -gt 0 ]; then
            echo -e "${RED}Failed targets: ${failed_targets[*]}${NC}"
            exit 1
        else
            echo -e "${GREEN}All builds completed successfully!${NC}"
            echo -e "${GREEN}Binaries available in ./binaries/${NC}"
        fi
    else
        build_target "$target"
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}Build successful!${NC}"
            echo -e "${GREEN}Binaries available in ./binaries/$target/${NC}"
        else
            exit 1
        fi
    fi
}

# Run main function
main "$@"