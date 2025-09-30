#!/bin/bash

# Maestro Captive Portal Update Script
# This script rebuilds the project and deploys it to the installation directory

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Maestro Captive Portal Update Script ===${NC}"
echo ""

# Get the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Load configuration to get installation directory
CONFIG_FILE="$SCRIPT_DIR/config/maestro.conf"
if [ ! -f "$CONFIG_FILE" ]; then
    echo -e "${RED}Error: Configuration file not found: $CONFIG_FILE${NC}"
    exit 1
fi

# Parse INSTALL_DIR from config
INSTALL_DIR=$(grep "^INSTALL_DIR=" "$CONFIG_FILE" | cut -d'=' -f2)
if [ -z "$INSTALL_DIR" ]; then
    echo -e "${YELLOW}Warning: INSTALL_DIR not found in config, using default${NC}"
    INSTALL_DIR="/opt/maestro/captive"
fi

echo -e "Installation directory: ${GREEN}$INSTALL_DIR${NC}"
echo ""

# Check if running with sufficient privileges for installation
if [ "$INSTALL_DIR" != "$SCRIPT_DIR" ] && [ ! -w "$(dirname "$INSTALL_DIR")" ] 2>/dev/null; then
    echo -e "${YELLOW}Note: You may need sudo privileges to install to $INSTALL_DIR${NC}"
    USE_SUDO="sudo"
else
    USE_SUDO=""
fi

# Clean previous build
echo -e "${YELLOW}Cleaning previous build...${NC}"
rm -rf build
mkdir -p build

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
cd build
cmake -G Ninja .. || {
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
}

# Build the project
echo -e "${YELLOW}Building project...${NC}"
ninja || {
    echo -e "${RED}Build failed!${NC}"
    exit 1
}

echo -e "${GREEN}Build completed successfully!${NC}"
echo ""

# Create installation directory if it doesn't exist
if [ ! -d "$INSTALL_DIR" ]; then
    echo -e "${YELLOW}Creating installation directory...${NC}"
    $USE_SUDO mkdir -p "$INSTALL_DIR"/{bin,config,web,logs}
fi

# Stop the service if it's running
echo -e "${YELLOW}Stopping service if running...${NC}"
$USE_SUDO systemctl stop maestro-captive 2>/dev/null || true
$USE_SUDO pkill -f maestro-captive 2>/dev/null || true
sleep 1

# Deploy binary
echo -e "${YELLOW}Deploying binary...${NC}"
$USE_SUDO cp maestro-captive "$INSTALL_DIR/bin/" || {
    echo -e "${RED}Failed to copy binary!${NC}"
    exit 1
}
$USE_SUDO chmod +x "$INSTALL_DIR/bin/maestro-captive"

# Deploy configuration (only if it doesn't exist, to preserve user settings)
if [ ! -f "$INSTALL_DIR/config/maestro.conf" ]; then
    echo -e "${YELLOW}Deploying configuration...${NC}"
    $USE_SUDO cp "$SCRIPT_DIR/config/maestro.conf" "$INSTALL_DIR/config/"
else
    echo -e "${YELLOW}Configuration exists, creating backup and updating...${NC}"
    $USE_SUDO cp "$INSTALL_DIR/config/maestro.conf" "$INSTALL_DIR/config/maestro.conf.backup.$(date +%Y%m%d_%H%M%S)"
    echo -e "${GREEN}Backup created. Review new settings in: $SCRIPT_DIR/config/maestro.conf${NC}"
fi

# Deploy web files
echo -e "${YELLOW}Deploying web files...${NC}"
$USE_SUDO cp -r "$SCRIPT_DIR/web"/* "$INSTALL_DIR/web/" || {
    echo -e "${RED}Failed to copy web files!${NC}"
    exit 1
}

# Set proper permissions
echo -e "${YELLOW}Setting permissions...${NC}"
$USE_SUDO chown -R root:root "$INSTALL_DIR" 2>/dev/null || true
$USE_SUDO chmod -R 755 "$INSTALL_DIR"

echo ""
echo -e "${GREEN}=== Update completed successfully! ===${NC}"
echo ""
echo "Binary installed to: $INSTALL_DIR/bin/maestro-captive"
echo "Configuration: $INSTALL_DIR/config/maestro.conf"
echo "Web files: $INSTALL_DIR/web/"
echo ""
echo "To start the service:"
echo "  sudo systemctl start maestro-captive"
echo ""
echo "Or run manually:"
echo "  cd $INSTALL_DIR/bin && sudo ./maestro-captive"
echo ""