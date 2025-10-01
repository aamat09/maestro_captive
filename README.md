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

### Production Installation

#### 1. Install System Dependencies
```bash
# Install all required dependencies (non-interactive for automation)
sudo DEBIAN_FRONTEND=noninteractive apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential cmake libssl-dev libjsoncpp-dev uuid-dev \
    zlib1g-dev libbrotli-dev git dnsmasq hostapd network-manager \
    libnl-3-dev libnl-genl-3-dev iptables iptables-persistent curl

# Fix any interrupted package installations
sudo DEBIAN_FRONTEND=noninteractive dpkg --configure -a
```

#### 2. Install Drogon Framework
```bash
cd /tmp
git clone https://github.com/drogonframework/drogon
cd drogon
git submodule update --init
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

#### 3. Build Maestro Captive Portal
```bash
cd /home/maestro/maestro_captive
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

#### 4. Install to System
```bash
# Create directory structure
sudo mkdir -p /opt/maestro/captive/{bin,config,web/{templates,static/{css,js}},logs}
sudo mkdir -p /var/log/maestro

# Install binary and assets
sudo cp maestro-captive /opt/maestro/captive/bin/
sudo chmod +x /opt/maestro/captive/bin/maestro-captive
sudo cp -r ../web/* /opt/maestro/captive/web/
sudo cp -r ../config/* /opt/maestro/captive/config/
```

#### 5. Configure Wireless Interface
**IMPORTANT:** Update the wireless interface name in the configuration file to match your system.

```bash
# Find your wireless interface name
ip link show | grep -E "^[0-9]+: w"

# Edit the configuration file
sudo nano /opt/maestro/captive/config/maestro.conf
# Change NETWORK_INTERFACE=wlan0 to your actual interface (e.g., wlo2, wlp2s0)
```

#### 6. Create Systemd Services
Create `/etc/systemd/system/maestro-captive.service`:
```ini
[Unit]
Description=Maestro Captive Portal Service
Documentation=https://github.com/aamat09/maestro_captive
After=network.target maestro-hotspot.service maestro-dhcp.service
Wants=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/maestro/captive
ExecStart=/opt/maestro/captive/bin/maestro-captive
Restart=always
RestartSec=10
KillMode=mixed
TimeoutStopSec=30
StandardOutput=journal
StandardError=journal
SyslogIdentifier=maestro-captive
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
```

Create `/etc/systemd/system/maestro-hotspot.service`:
```ini
[Unit]
Description=Maestro WiFi Hotspot Service
Documentation=https://github.com/aamat09/maestro_captive
After=network.target NetworkManager.service
Requires=NetworkManager.service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/bin/nmcli connection up maestro-hotspot
ExecStop=/usr/bin/nmcli connection down maestro-hotspot
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Create `/etc/systemd/system/maestro-dhcp.service`:
```ini
[Unit]
Description=Maestro DHCP/DNS Service for Captive Portal
Documentation=https://github.com/aamat09/maestro_captive
After=network.target maestro-hotspot.service
Requires=maestro-hotspot.service

[Service]
Type=forking
PIDFile=/run/dnsmasq-maestro.pid
ExecStartPre=/bin/mkdir -p /opt/maestro/config
ExecStart=/usr/sbin/dnsmasq --conf-file=/opt/maestro/config/dnsmasq.conf --pid-file=/run/dnsmasq-maestro.pid
ExecStop=/bin/kill -TERM $MAINPID
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Create dnsmasq configuration at `/opt/maestro/config/dnsmasq.conf`:
```conf
# Maestro Captive Portal DNS/DHCP Configuration

# Interface to listen on (update to match your wireless interface)
interface=wlo2
bind-interfaces

# DHCP range for captive portal clients
dhcp-range=192.168.4.50,192.168.4.150,12h

# DNS settings - redirect all DNS queries to captive portal
address=/#/192.168.4.1

# Do not read /etc/resolv.conf or /etc/hosts
no-resolv
no-hosts

# Log for debugging (optional)
log-queries
log-dhcp

# Cache size
cache-size=1000

# Disable negative caching
no-negcache
```

#### 7. Enable and Start Services
```bash
# Reload systemd to recognize new services
sudo systemctl daemon-reload

# Enable services to start on boot
sudo systemctl enable maestro-captive.service
sudo systemctl enable maestro-hotspot.service
sudo systemctl enable maestro-dhcp.service

# Start the captive portal
sudo systemctl start maestro-captive.service

# Check status
sudo systemctl status maestro-captive.service
```

### Quick Install (Automated)
```bash
cd /home/maestro/maestro_captive
chmod +x scripts/install.sh
sudo ./scripts/install.sh
```

**Note:** The automated installer may require manual configuration of the wireless interface name in `/opt/maestro/captive/config/maestro.conf`

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

## Configuration

### Key Configuration Files

#### `/opt/maestro/captive/config/maestro.conf`
Main configuration file for the captive portal:
```conf
NETWORK_INTERFACE=wlo2        # Update to your wireless interface
HOTSPOT_SSID=Maestro-Setup
HOTSPOT_PASSWORD=maestro123
HOTSPOT_IP=192.168.4.1
SERVER_PORT=8080
```

**Important Notes:**
- The `NETWORK_INTERFACE` must match your system's actual wireless interface name
- Use `ip link show` to find your wireless interface name
- Common names: `wlan0`, `wlo2`, `wlp2s0`, `wlp3s0`

### Path Configuration

The application uses **relative paths** from the working directory. The systemd service sets `WorkingDirectory=/opt/maestro/captive`, which means:

- Configuration: `config/maestro.conf` → `/opt/maestro/captive/config/maestro.conf`
- Web templates: `web/templates/` → `/opt/maestro/captive/web/templates/`
- Static files: `web/static/` → `/opt/maestro/captive/web/static/`
- Logs: `logs/` → `/opt/maestro/captive/logs/`

## Development

### Recent Code Changes (2025-10-01)

#### Path Handling
Changed from absolute/parent-relative paths to relative paths for better portability:

**src/main.cpp:**
- `../config/maestro.conf` → `config/maestro.conf`
- `../web` → `web`
- `./logs` → `logs`

**src/controllers/CaptivePortalController.cpp:**
- `../web/templates/index.html` → `web/templates/index.html`

These paths are now relative to the working directory set in the systemd service (`/opt/maestro/captive`).

#### Configuration
- Added wireless interface configuration documentation
- Emphasized the need to configure `NETWORK_INTERFACE` in `maestro.conf` to match the actual wireless interface on the system

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

### Troubleshooting

#### WiFi Scanning Not Working
1. Check your wireless interface name: `ip link show | grep -E "^[0-9]+: w"`
2. Update `/opt/maestro/captive/config/maestro.conf` with the correct interface name
3. Restart the service: `sudo systemctl restart maestro-captive.service`
4. Verify NetworkManager is managing the interface: `nmcli device status`

#### Web UI Shows 404
1. Verify the working directory in the systemd service is `/opt/maestro/captive`
2. Check that web files exist: `ls -la /opt/maestro/captive/web/`
3. Check service logs: `journalctl -u maestro-captive.service -f`

#### Static Files Not Loading
1. Verify document root is set: Check logs for Drogon configuration
2. Test static file access: `curl -I http://localhost:8080/static/js/app.js`
3. Check file permissions: `ls -la /opt/maestro/captive/web/static/`

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