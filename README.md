# Maestro Captive Portal

A professional C++ captive portal application built with the Drogon framework for IoT device network configuration and Home Assistant deployment.

## Overview

Maestro Captive Portal provides a complete solution for configuring IoT devices to connect to home WiFi networks without requiring a monitor or keyboard. The system creates a temporary WiFi hotspot that users can connect to, then guides them through a professional web interface to configure their home network and start Home Assistant.

## Features

### 🌐 **WiFi Management**
- Professional captive portal with automatic device redirection
- WiFi network scanning and selection interface
- Secure connection validation and internet connectivity testing
- Automatic hotspot shutdown after successful configuration

### 🏠 **Home Assistant Integration**
- Automated Home Assistant container deployment
- Docker Compose orchestration
- Service health monitoring and management

### 🔒 **Security & Validation**
- Connection validation workflow with timeout handling
- Internet connectivity verification (ping + HTTP)
- Hardware security framework (extensible for licensing)
- Secure password handling with visibility toggle

### 📱 **Professional UI/UX**
- Responsive web interface optimized for mobile devices
- Multi-step wizard with progress indicators
- Real-time connection status updates
- Professional design suitable for commercial deployment

## Architecture

```
maestro-captive/
├── src/                    # C++ source code
│   ├── controllers/        # HTTP request handlers
│   ├── services/          # Business logic services
│   └── utils/             # Utility functions
├── include/               # Header files
├── web/                   # Web interface assets
│   ├── templates/         # HTML templates
│   └── static/           # CSS, JavaScript, images
├── scripts/              # Build and installation scripts
├── config/               # Configuration files
└── test/                 # Testing infrastructure
    ├── unit/             # Unit tests
    └── temporary/        # Temporary test programs
```

## API Endpoints

### WiFi Management
- `GET /api/wifi/scan` - Scan for available networks
- `POST /api/wifi/connect` - Connect to selected network
- `GET /api/wifi/status` - Get connection status
- `GET /api/wifi/validate` - Validate internet connectivity

### Service Management
- `POST /api/services/homeassistant/start` - Start Home Assistant
- `POST /api/services/homeassistant/stop` - Stop Home Assistant
- `GET /api/services/status` - Get service status
- `POST /api/services/hotspot/shutdown` - Shutdown captive portal mode

## Installation

### Prerequisites
- Debian 12+ with WiFi interface
- NetworkManager
- Docker and Docker Compose
- CMake and C++ build tools

### Quick Install
```bash
cd /home/maestro/captive
chmod +x scripts/install.sh
sudo ./scripts/install.sh
```

### Manual Build
```bash
# Install dependencies
sudo apt-get install build-essential cmake libssl-dev libjsoncpp-dev \
    network-manager dnsmasq hostapd docker-compose

# Install Drogon framework
git clone https://github.com/drogonframework/drogon
cd drogon && mkdir build && cd build
cmake .. && make -j$(nproc) && sudo make install

# Build Maestro Captive Portal
cd /home/maestro/captive
mkdir build && cd build
cmake .. && make -j$(nproc)
```

## Usage

### Setup Mode (First Boot)
1. Device creates "Maestro-Setup" WiFi hotspot
2. Users connect to setup network (password: maestro123)
3. Browser automatically redirects to captive portal
4. Users select home WiFi and enter password
5. System validates connection and starts Home Assistant
6. Setup mode automatically disabled

### Normal Operation
- Home Assistant accessible at `http://[device-ip]:8123`
- Device connected to home WiFi network
- Setup mode disabled until manual re-activation

## System Services

- `maestro-captive.service` - Main captive portal application
- `maestro-hotspot.service` - WiFi hotspot management
- `maestro-dhcp.service` - DHCP/DNS server for captive portal

## Development

### Testing
```bash
cd test/temporary
g++ -o test_program test_program.cpp
./test_program
rm test_program  # Clean up after testing
```

### Project Structure
- All temporary test files go in `test/temporary/`
- Unit tests belong in `test/unit/`
- No test files in project root

## Commercial Deployment

This project is designed for commercial IoT deployment with:
- Professional UI suitable for end customers
- Reliable connection validation and error handling
- Hardware security framework for licensing
- Automated service management
- Mobile-responsive design

## License

[License information to be added]

## Contributing

[Contribution guidelines to be added]