#!/bin/bash

# Maestro Captive Portal Installation Script with Full Captive Portal Setup

set -e

echo "Installing Maestro Captive Portal with Captive Portal Detection..."

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo)"
    exit 1
fi

# Install system dependencies
echo "Installing system dependencies..."
apt-get update
apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libbrotli-dev \
    git \
    dnsmasq \
    hostapd \
    network-manager \
    libnl-3-dev \
    libnl-genl-3-dev \
    iptables \
    iptables-persistent \
    curl

# Install Drogon framework
echo "Installing Drogon framework..."
if [ ! -d "/tmp/drogon" ]; then
    cd /tmp
    git clone https://github.com/drogonframework/drogon
    cd drogon
    git submodule update --init
    mkdir build
    cd build
    cmake ..
    make -j$(nproc)
    make install
    ldconfig
fi

# Create installation directories
echo "Creating installation directories..."
mkdir -p /opt/maestro/captive/{bin,config,web/{templates,static/{css,js}},logs}
mkdir -p /var/log/maestro

# Build the captive portal application
echo "Building Maestro Captive Portal..."
cd /home/maestro/captive
if [ ! -d "build" ]; then
    mkdir build
fi
cd build
cmake ..
make -j$(nproc)

# Install the binary
echo "Installing binary..."
cp maestro-captive /opt/maestro/captive/bin/
chmod +x /opt/maestro/captive/bin/maestro-captive

# Copy web assets
echo "Installing web assets..."
cp -r ../web/* /opt/maestro/captive/web/

# Copy configuration files
echo "Installing configuration files..."
cp -r ../config/* /opt/maestro/captive/config/ 2>/dev/null || true

# Ensure configuration files exist in the right place
if [ -f "/opt/maestro/config/dnsmasq.conf" ]; then
    echo "DNSmasq configuration already in place"
else
    cp /opt/maestro/captive/config/* /opt/maestro/config/ 2>/dev/null || true
fi

# Create NetworkManager hotspot connection
echo "Creating NetworkManager hotspot connection..."
nmcli connection show maestro-hotspot >/dev/null 2>&1 || \
nmcli connection add type wifi ifname wlan0 con-name maestro-hotspot autoconnect no \
    ssid "Maestro-Setup" \
    802-11-wireless.mode ap \
    802-11-wireless.band bg \
    802-11-wireless-security.key-mgmt wpa-psk \
    802-11-wireless-security.psk "maestro123" \
    ipv4.method shared \
    ipv4.address 192.168.4.1/24

# Enable and configure systemd services
echo "Configuring systemd services..."
systemctl daemon-reload
systemctl enable maestro-captive.service
systemctl enable maestro-hotspot.service  
systemctl enable maestro-dhcp.service

# Set up iptables persistence
echo "Setting up iptables persistence..."
# Save current iptables as baseline
iptables-save > /etc/iptables/rules.v4

# Enable IP forwarding for captive portal
echo 'net.ipv4.ip_forward=1' > /etc/sysctl.d/99-maestro-captive.conf
sysctl -p /etc/sysctl.d/99-maestro-captive.conf

# Create log rotation
echo "Configuring log rotation..."
cat > /etc/logrotate.d/maestro << 'LOGEOF'
/var/log/maestro/*.log {
    daily
    missingok
    rotate 7
    compress
    delaycompress
    notifempty
    create 644 root root
    postrotate
        systemctl reload maestro-captive.service >/dev/null 2>&1 || true
    endscript
}
LOGEOF

echo "Installation completed successfully!"
echo ""
echo "🌐 CAPTIVE PORTAL SETUP COMPLETE!"
echo ""
echo "To start captive portal mode:"
echo "  systemctl start maestro-hotspot"
echo "  systemctl start maestro-dhcp"
echo "  systemctl start maestro-captive"
echo ""
echo "📱 Device Detection:"
echo "  - WiFi Name: Maestro-Setup"
echo "  - Password: maestro123"
echo "  - Portal IP: 192.168.4.1"
echo ""
echo "✅ Automatic redirection configured for:"
echo "  📱 Android devices (generate_204)"
echo "  🍎 Apple devices (hotspot-detect.html)"
echo "  🪟 Windows devices (connecttest.txt)"
echo "  🦊 Firefox browsers (success.txt)"
echo "  🐧 Ubuntu systems (connectivity-check)"
echo ""
echo "🔧 Test the captive portal:"
echo "  cd /home/maestro/captive/test/temporary"
echo "  ./captive_portal_test.sh"
echo ""
echo "Service status:"
systemctl status maestro-captive.service --no-pager -l || true
