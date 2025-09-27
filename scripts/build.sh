#!/bin/bash
# Maestro Captive Portal - Build Script

set -e

echo "Building Maestro Captive Portal..."

# Create build directory
if [ -d "build" ]; then
    echo "Cleaning previous build..."
    rm -rf build
fi

mkdir build
cd build

# Configure with CMake
echo "Configuring build..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/maestro/captive

# Build the project
echo "Compiling..."
make -j$(nproc)

echo "Build completed successfully!"
echo "Run 'sudo ./install.sh' to install the captive portal."
