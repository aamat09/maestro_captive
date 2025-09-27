#!/bin/bash

# Maestro Captive Portal Installation Script

set -e

echo "Installing Maestro Captive Portal..."

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
    libnl-genl-3-dev

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

# Copy configuration
echo "Installing configuration..."
cp ../config/* /opt/maestro/captive/config/ 2>/dev/null || true

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

# Configure iptables rules for captive portal
echo "Configuring firewall rules..."
iptables -I INPUT -p tcp --dport 80 -j ACCEPT
iptables -I INPUT -p tcp --dport 443 -j ACCEPT
iptables -I INPUT -p udp --dport 53 -j ACCEPT
iptables -I INPUT -p udp --dport 67 -j ACCEPT

# Save iptables rules
if command -v iptables-save >/dev/null 2>&1; then
    iptables-save > /etc/iptables/rules.v4 2>/dev/null || true
fi

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
echo "To start the captive portal mode:"
echo "  systemctl start maestro-hotspot"
echo "  systemctl start maestro-dhcp"
echo "  systemctl start maestro-captive"
echo ""
echo "To configure WiFi and start Home Assistant:"
echo "  Access http://192.168.4.1 on a connected device"
echo ""
echo "Service status:"
systemctl status maestro-captive.service --no-pager -l || true
