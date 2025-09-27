#!/bin/bash
# Maestro Captive Portal - Dependency Installation Script

set -e

echo "Installing Maestro Captive Portal dependencies..."

# Update package list
apt update

# Install build tools
echo "Installing build tools..."
apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libnl-3-dev \
    libnl-genl-3-dev \
    libssl-dev \
    libsystemd-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    curl \
    wget \
    network-manager \
    hostapd \
    dnsmasq

# Install Drogon framework
echo "Installing Drogon framework..."
if ! pkg-config --exists drogon; then
    echo "Drogon not found, installing from source..."
    
    # Install additional dependencies for Drogon
    apt install -y \
        libbrotli-dev \
        libhiredis-dev \
        libpq-dev \
        libsqlite3-dev \
        libmysqlclient-dev
    
    # Clone and build Drogon
    cd /tmp
    if [ -d "drogon" ]; then
        rm -rf drogon
    fi
    
    git clone https://github.com/drogonframework/drogon.git
    cd drogon
    git checkout v1.9.1  # Use stable version
    git submodule update --init
    
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    make install
    ldconfig
    
    echo "Drogon framework installed successfully"
else
    echo "Drogon framework already installed"
fi

# Verify installations
echo "Verifying installations..."
cmake --version
pkg-config --exists drogon && echo "✓ Drogon framework installed"
pkg-config --exists libnl-3.0 && echo "✓ libnl-3 installed"
pkg-config --exists openssl && echo "✓ OpenSSL installed"
pkg-config --exists jsoncpp && echo "✓ JsonCpp installed"

echo "All dependencies installed successfully!"
echo "Run ./build.sh to compile the captive portal."
