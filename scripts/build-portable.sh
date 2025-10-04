#!/bin/bash
# Maestro Captive Portal - Portable Build Script
# Creates a self-contained executable with all dependencies

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"
APP_NAME="maestro-captive"

echo "=== Maestro Captive Portal Portable Build ==="
echo "Project root: $PROJECT_ROOT"

# Clean previous build
echo "Cleaning previous build..."
rm -rf "$BUILD_DIR" "$DIST_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$DIST_DIR"

# Build the application
echo "Building application..."
cd "$BUILD_DIR"
cmake .. >/dev/null
make -j$(nproc) || make -j1 || {
    echo "Regular build failed, trying manual link with -O0..."
    /usr/bin/c++ -O0 CMakeFiles/$APP_NAME.dir/src/*.o CMakeFiles/$APP_NAME.dir/src/*/*.o \
        -o $APP_NAME \
        /usr/local/lib/libdrogon.a \
        /usr/local/lib/libtrantor.a \
        /usr/lib/x86_64-linux-gnu/libssl.so \
        /usr/lib/x86_64-linux-gnu/libcrypto.so \
        -lpthread -ldl \
        /usr/lib/x86_64-linux-gnu/libjsoncpp.so \
        /usr/lib/x86_64-linux-gnu/libuuid.so \
        /usr/lib/x86_64-linux-gnu/libbrotlidec.so \
        /usr/lib/x86_64-linux-gnu/libbrotlienc.so \
        /usr/lib/x86_64-linux-gnu/libbrotlicommon.so \
        /usr/lib/x86_64-linux-gnu/libz.so
}

if [ ! -f "$APP_NAME" ]; then
    echo "ERROR: Build failed - executable not found"
    exit 1
fi

echo "Build successful!"

# Create distribution structure
echo "Creating distribution package..."
mkdir -p "$DIST_DIR/$APP_NAME"
mkdir -p "$DIST_DIR/$APP_NAME/bin"
mkdir -p "$DIST_DIR/$APP_NAME/lib"
mkdir -p "$DIST_DIR/$APP_NAME/web"
mkdir -p "$DIST_DIR/$APP_NAME/config"

# Copy executable
echo "Copying executable..."
cp "$BUILD_DIR/$APP_NAME" "$DIST_DIR/$APP_NAME/bin/"

# Copy web files
echo "Copying web files..."
cp -r "$PROJECT_ROOT/web"/* "$DIST_DIR/$APP_NAME/web/"

# Copy config
echo "Copying config..."
cp "$PROJECT_ROOT/config/maestro.conf" "$DIST_DIR/$APP_NAME/config/"

# Find and copy all dynamic library dependencies
echo "Gathering dependencies..."
LIBS_TO_COPY=$(ldd "$BUILD_DIR/$APP_NAME" | grep "=>" | awk '{print $3}' | grep -v "^$")

for lib in $LIBS_TO_COPY; do
    if [ -f "$lib" ]; then
        cp -L "$lib" "$DIST_DIR/$APP_NAME/lib/"
    fi
done

# Create launcher script
echo "Creating launcher script..."
cat > "$DIST_DIR/$APP_NAME/run.sh" << 'LAUNCHER_EOF'
#!/bin/bash
# Maestro Captive Portal Launcher

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set library path to use bundled libraries
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"

# Change to script directory for relative paths
cd "$SCRIPT_DIR"

# Run the application
exec "$SCRIPT_DIR/bin/maestro-captive" "$@"
LAUNCHER_EOF

chmod +x "$DIST_DIR/$APP_NAME/run.sh"

# Create README
cat > "$DIST_DIR/$APP_NAME/README.md" << 'README_EOF'
# Maestro Captive Portal - Portable Distribution

## Contents
- `bin/maestro-captive` - Main executable
- `lib/` - Bundled dependencies
- `web/` - Web UI files
- `config/` - Configuration files
- `run.sh` - Launcher script

## Installation

1. Copy this entire directory to your target location:
   ```bash
   cp -r maestro-captive /opt/maestro/captive
   ```

2. Edit configuration:
   ```bash
   nano config/maestro.conf
   ```

3. Run directly:
   ```bash
   ./run.sh
   ```

4. Or install as systemd service:
   ```bash
   sudo cp scripts/install-service.sh /tmp/
   sudo /tmp/install-service.sh /opt/maestro/captive
   ```

## Requirements
- Linux x86_64
- systemd (for service installation)
- hostapd and dnsmasq (for WiFi hotspot)
- wpa_supplicant (for WiFi client)

## Configuration
Edit `config/maestro.conf` to set:
- NETWORK_INTERFACE (your WiFi interface, e.g., wlo2)
- HOTSPOT_SSID (hotspot name)
- HOTSPOT_PASSWORD (hotspot password)
- SERVER_PORT (web server port, default 8080)

## Running
The launcher script (`run.sh`) automatically:
- Sets LD_LIBRARY_PATH to use bundled libraries
- Changes to the correct directory
- Runs the executable

All file paths in the application are relative to the installation directory.
README_EOF

# Create service installer
cat > "$DIST_DIR/$APP_NAME/scripts/install-service.sh" << 'SERVICE_EOF'
#!/bin/bash
# Install Maestro Captive Portal as systemd service

if [ -z "$1" ]; then
    echo "Usage: $0 <installation_path>"
    echo "Example: $0 /opt/maestro/captive"
    exit 1
fi

INSTALL_PATH="$(realpath "$1")"

if [ ! -f "$INSTALL_PATH/run.sh" ]; then
    echo "ERROR: Invalid installation path. run.sh not found."
    exit 1
fi

echo "Creating systemd service for $INSTALL_PATH..."

cat > /etc/systemd/system/maestro-captive.service << EOF
[Unit]
Description=Maestro Captive Portal Service
After=network.target
Wants=network.target
Documentation=https://github.com/aamat09/maestro_captive

[Service]
Type=simple
User=root
WorkingDirectory=$INSTALL_PATH
ExecStart=$INSTALL_PATH/run.sh
Restart=on-failure
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable maestro-captive
echo "Service installed! Use: systemctl start maestro-captive"
SERVICE_EOF

chmod +x "$DIST_DIR/$APP_NAME/scripts/install-service.sh"

# Create archive
echo "Creating archive..."
cd "$DIST_DIR"
tar -czf "$APP_NAME-portable-$(uname -m).tar.gz" "$APP_NAME"

# Summary
echo ""
echo "=== Build Complete ==="
echo "Distribution: $DIST_DIR/$APP_NAME"
echo "Archive: $DIST_DIR/$APP_NAME-portable-$(uname -m).tar.gz"
echo ""
echo "Package contents:"
du -sh "$DIST_DIR/$APP_NAME"
echo ""
echo "To deploy:"
echo "  1. Extract: tar -xzf $APP_NAME-portable-$(uname -m).tar.gz"
echo "  2. Move: sudo mv $APP_NAME /opt/maestro/captive"
echo "  3. Configure: nano /opt/maestro/captive/config/maestro.conf"
echo "  4. Install service: sudo /opt/maestro/captive/scripts/install-service.sh /opt/maestro/captive"
echo "  5. Start: sudo systemctl start maestro-captive"
echo ""
echo "Libraries bundled:"
ls -1 "$DIST_DIR/$APP_NAME/lib" | wc -l
