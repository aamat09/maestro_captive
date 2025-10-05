# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Development Commands

### Building the Application

**Standard build:**
```bash
cd /home/maestro/maestro_captive
./scripts/build.sh
```

**Clean build from scratch:**
```bash
rm -rf build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

**Workaround for faulty disk (linker crashes):**
If you encounter linker segfaults during build, use manual linking with `-O0`:
```bash
cd build
cmake ..
make  # This will fail at linking step

# Manual link command:
/usr/bin/c++ -O0 CMakeFiles/maestro-captive.dir/src/*.o CMakeFiles/maestro-captive.dir/src/*/*.o \
    -o maestro-captive \
    /usr/local/lib/libdrogon.a \
    /usr/local/lib/libtrantor.a \
    -lssl -lcrypto -lpthread -ldl -lz -lbrotlienc -lbrotlidec -lbrotlicommon -luuid -ljsoncpp
```

### Deployment

**Deploy web files after changes:**
```bash
cp -r /home/maestro/maestro_captive/web/* /opt/maestro/captive/web/
systemctl restart maestro-captive
```

**Note:** Drogon caches static files, so service restart is required after updating CSS/JS/HTML.

**Full installation:**
```bash
sudo ./scripts/install.sh
```

### Service Management

```bash
# Restart captive portal
systemctl restart maestro-captive

# Check status
systemctl status maestro-captive

# View logs
journalctl -u maestro-captive.service -f

# Check WiFi scanning
journalctl -u maestro-captive.service -f | grep -i scan
```

### Testing WiFi Scanning

```bash
# Manual WiFi scan test
iw dev wlo2 scan | grep SSID

# Check virtual interface creation
iw dev

# Test full scan with hostapd restart
systemctl stop hostapd
iw dev wlo2 scan
systemctl start hostapd
```

## Architecture Overview

### WiFi Scanning Strategy

The system uses a **dual-mode WiFi scanning approach** to work around the limitation that virtual interfaces can only scan on the hotspot's channel (typically channel 6):

1. **Quick Scan (Default)**: Uses virtual interface `wlo2_scan` created via `iw dev wlo2 interface add wlo2_scan type station`. This is fast but only scans the hotspot's channel. Returns cached results from startup scan.

2. **Full Rescan (Manual)**: Stops hostapd service, scans all WiFi channels, then restarts hotapd. This briefly interrupts the hotspot but finds networks on all channels.

**Startup Behavior:**
- On service start, a background thread performs a full scan (all channels) after 2-second delay
- Results are cached in-memory with thread-safe mutex protection
- UI automatically loads cached networks when user clicks "Get Started"
- "Full Rescan" button available if user's network isn't found in quick scan

**Implementation:**
- `WiFiUtils::scanNetworks(bool full_scan)` - Routes between quick/full scan
- `WiFiUtils::scanWithVirtualInterface()` - Quick scan, single channel
- `WiFiUtils::fullScanWithInterruption()` - Full scan, all channels, restarts hotapd
- `WiFiManagerService::scanNetworks()` - Service layer with caching
- Startup scan: `src/main.cpp:78-85` (background thread)

### Network Stack

The system has **replaced NetworkManager with hostapd + dnsmasq + wpa_supplicant** for more reliable WiFi management:

- **hostapd**: Creates WiFi hotspot (Maestro-Setup SSID)
- **dnsmasq**: DHCP/DNS server for captive portal clients
- **wpa_supplicant**: WiFi client for connecting to home networks
- **iptables**: Firewall rules for captive portal redirection

**Important:** The `NETWORK_INTERFACE` config must match the actual wireless interface name (e.g., wlo2, wlan0, wlp2s0). Use `ip link show` to find it.

### Path Configuration

The application uses **relative paths** from the working directory. The systemd service sets `WorkingDirectory=/opt/maestro/captive`, which means all paths in code are relative to this directory:

- `config/maestro.conf` → `/opt/maestro/captive/config/maestro.conf`
- `web/templates/index.html` → `/opt/maestro/captive/web/templates/index.html`
- `web/static/` → `/opt/maestro/captive/web/static/`
- `logs/` → `/opt/maestro/captive/logs/`

**Never use absolute paths or parent-relative paths (`../`) in the code.** Always use paths relative to the working directory.

### Service Architecture

**Singleton Services:**
- `WiFiManagerService`: Manages WiFi scanning, connection, caching
- `HomeAssistantService`: Manages Home Assistant Docker containers
- `HardwareSecurityService`: Hardware licensing (extensible)
- `ConfigManager`: Configuration file loading
- `Logger`: Application logging

**Controllers (Drogon HTTP handlers):**
- `CaptivePortalController`: Serves main UI (`/`)
- `WiFiController`: WiFi API endpoints (`/api/wifi/*`)
- `ServiceController`: Service management endpoints (`/api/services/*`)

**Utilities:**
- `WiFiUtils`: Low-level WiFi operations (iw, nmcli commands)
- `ConfigManager`: Config file parsing
- `Logger`: File and console logging

### Thread Safety

The WiFi network cache uses mutex protection:
```cpp
std::vector<WiFiNetwork> cachedNetworks;  // Shared state
std::mutex cacheMutex;                    // Protects cache
```

The startup scan runs in a detached thread to avoid blocking the main web server.

### Frontend Architecture

- **Single-page wizard**: 5 steps (welcome, WiFi scan, connection, services, complete)
- **Auto-scan on "Get Started"**: `nextStep()` automatically calls `scanWiFi()` when entering step 1
- **Rescan button placement**: Uses `flex-basis: 100%` to force full width below WiFi list in flexbox container
- **Static file caching**: Drogon caches static files for 1 day, requires service restart after changes

## Common Development Pitfalls

### 1. Hidden SSID Filtering
Hidden SSIDs containing `\x00` bytes must be filtered in **both** scan methods:
- `WiFiUtils::scanWithVirtualInterface()`
- `WiFiUtils::fullScanWithInterruption()`

Filter condition: `ssid.find("\\x00") == std::string::npos`

### 2. Web File Deployment
After changing files in `web/`, you **must**:
1. Copy to `/opt/maestro/captive/web/`
2. Restart service: `systemctl restart maestro-captive`

Browser cache can also cause issues - clear it or use incognito mode for testing.

### 3. Firewall Configuration
The iptables rules must allow access from **both interfaces**:
- `wlo2` (WiFi hotspot)
- `enp2s0` (Ethernet, for debugging)

Rules stored in `/etc/iptables/rules.v4`.

### 4. Flexbox Layout Issues
Parent containers with `display: flex` can cause child elements to appear beside (not below) each other. Use `flex-basis: 100%` and `width: 100%` to force full-width wrapping.

### 5. Startup Scan Not Cached
The startup scan thread must populate the cache, not just perform the scan. Check that `WiFiManagerService::scanNetworks(true)` is called (not `WiFiUtils::scanNetworks()`).

## Code Patterns

### Adding a new API endpoint

1. Add method to appropriate controller header (`include/controllers/`)
2. Implement in controller source (`src/controllers/`)
3. Use Drogon macros for routing:
```cpp
METHOD_LIST_BEGIN
ADD_METHOD_TO(WiFiController::scan, "/api/wifi/scan", Get);
METHOD_LIST_END
```

### Adding a new service

1. Create singleton pattern:
```cpp
class MyService {
public:
    static MyService& getInstance();
    bool initialize();
private:
    MyService() = default;
    bool initialized = false;
};
```

2. Initialize in `main.cpp` before `app().run()`

### Working with WiFi utilities

All WiFi operations go through `WiFiUtils` static methods. Use `#if PLATFORM_LINUX` guards for Linux-specific code.

## System Dependencies

- **Drogon Framework**: C++ web framework (must be built from source)
- **hostapd**: WiFi hotspot daemon
- **dnsmasq**: DHCP/DNS server
- **wpa_supplicant**: WiFi client
- **iw**: Wireless configuration tool
- **iptables**: Firewall
- **Docker & Docker Compose**: For Home Assistant

## Configuration Files

- `/opt/maestro/captive/config/maestro.conf`: Main config
- `/opt/maestro/config/dnsmasq.conf`: DHCP/DNS settings
- `/etc/systemd/system/maestro-captive.service`: Service definition
- `/etc/iptables/rules.v4`: Firewall rules

**Critical:** Always update `NETWORK_INTERFACE` in `maestro.conf` to match the actual wireless interface on the target system.
