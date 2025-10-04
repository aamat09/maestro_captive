#include "utils/WiFiUtils.h"
#include "utils/ConfigManager.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <regex>
#include <unistd.h>

#ifdef __linux__
#define PLATFORM_LINUX 1
#else
#define PLATFORM_LINUX 0
#endif

std::vector<WiFiScanResult> WiFiUtils::scanNetworks() {
    std::vector<WiFiScanResult> results;

    // Return empty results on non-Linux platforms
    #if !PLATFORM_LINUX
    return results;
    #endif

    // Get network interface from config
    auto& config = ConfigManager::getInstance();
    std::string interface = config.get("NETWORK_INTERFACE", "wlan0");
    std::string scan_interface = interface + "_scan";

    // Create virtual interface for scanning if it doesn't exist
    bool created_vif = false;
    if (system(("ip link show " + scan_interface + " >/dev/null 2>&1").c_str()) != 0) {
        // Virtual interface doesn't exist, create it
        if (system(("iw dev " + interface + " interface add " + scan_interface + " type managed").c_str()) == 0) {
            created_vif = true;
            system(("ip link set " + scan_interface + " up").c_str());
        }
    } else {
        // Virtual interface exists, just bring it up
        system(("ip link set " + scan_interface + " up").c_str());
    }

    // Use the scan interface if available, otherwise fall back to main interface
    std::string scan_dev = (created_vif || system(("ip link show " + scan_interface + " >/dev/null 2>&1").c_str()) == 0)
                          ? scan_interface : interface;

    // Trigger scan using iw
    system(("iw dev " + scan_dev + " scan > /dev/null 2>&1").c_str());

    // Get scan results using iw
    std::string command = "iw dev " + scan_dev + " scan 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        if (created_vif) {
            system(("iw dev " + scan_interface + " del 2>/dev/null").c_str());
        }
        return results;
    }

    char buffer[512];
    std::string current_ssid;
    int current_signal = 0;
    std::string current_security;

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);

        // Check for SSID
        if (line.find("SSID: ") != std::string::npos) {
            size_t pos = line.find("SSID: ") + 6;
            current_ssid = line.substr(pos);
            if (!current_ssid.empty() && current_ssid.back() == '\n') {
                current_ssid.pop_back();
            }
        }

        // Check for signal strength
        else if (line.find("signal: ") != std::string::npos) {
            size_t pos = line.find("signal: ") + 8;
            std::string signal_str = line.substr(pos);
            // Extract number (format: "-XX.00 dBm")
            current_signal = std::stoi(signal_str);
        }

        // Check for security (WPA/WPA2/RSN)
        else if (line.find("WPA:") != std::string::npos || line.find("RSN:") != std::string::npos) {
            current_security = "WPA";
        }

        // BSS end marker - save current network
        else if (line.find("BSS ") == 0 && !current_ssid.empty()) {
            WiFiScanResult result;
            result.ssid = current_ssid;
            result.signal_strength = std::min(100, std::max(0, (current_signal + 100) * 2));
            result.security = current_security;
            results.push_back(result);

            // Reset for next network
            current_ssid.clear();
            current_signal = 0;
            current_security.clear();
        }
    }

    // Add last network if exists
    if (!current_ssid.empty()) {
        WiFiScanResult result;
        result.ssid = current_ssid;
        result.signal_strength = std::min(100, std::max(0, (current_signal + 100) * 2));
        result.security = current_security;
        results.push_back(result);
    }

    pclose(pipe);

    // Clean up virtual interface if we created it
    if (created_vif) {
        system(("iw dev " + scan_interface + " del 2>/dev/null").c_str());
    }

    return results;
}

bool WiFiUtils::connectToNetwork(const std::string& ssid, const std::string& password) {
    #if !PLATFORM_LINUX
    return false;
    #endif

    auto& config = ConfigManager::getInstance();
    std::string interface = config.get("NETWORK_INTERFACE", "wlan0");

    // Stop hotspot services
    system("systemctl stop maestro-dnsmasq maestro-hostapd 2>/dev/null");

    // Bring interface down and up
    system(("ip link set " + interface + " down").c_str());
    system(("ip addr flush dev " + interface).c_str());
    system(("ip link set " + interface + " up").c_str());

    // Add network to wpa_supplicant
    std::string add_network_cmd = "wpa_cli -i " + interface + " add_network 2>/dev/null";
    FILE* pipe = popen(add_network_cmd.c_str(), "r");
    if (!pipe) return false;

    char buffer[128];
    int network_id = -1;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        network_id = std::stoi(buffer);
    }
    pclose(pipe);

    if (network_id < 0) return false;

    // Configure network
    std::string set_ssid = "wpa_cli -i " + interface + " set_network " + std::to_string(network_id) + " ssid '\"" + ssid + "\"'";
    std::string set_psk = "wpa_cli -i " + interface + " set_network " + std::to_string(network_id) + " psk '\"" + password + "\"'";
    std::string enable_network = "wpa_cli -i " + interface + " enable_network " + std::to_string(network_id);
    std::string save_config = "wpa_cli -i " + interface + " save_config";

    system(set_ssid.c_str());
    system(set_psk.c_str());
    system(enable_network.c_str());
    system(save_config.c_str());

    // Request DHCP
    system(("dhclient " + interface + " 2>/dev/null &").c_str());

    return true;
}

bool WiFiUtils::isConnected() {
    #if !PLATFORM_LINUX
    return false;
    #endif

    auto& config = ConfigManager::getInstance();
    std::string interface = config.get("NETWORK_INTERFACE", "wlan0");

    // Check if wpa_supplicant shows connected state
    std::string command = "wpa_cli -i " + interface + " status 2>/dev/null | grep -q 'wpa_state=COMPLETED'";
    int result = system(command.c_str());
    return result == 0;
}

std::string WiFiUtils::getCurrentSSID() {
    #if !PLATFORM_LINUX
    return "";
    #endif

    // Get network interface from config
    auto& config = ConfigManager::getInstance();
    std::string interface = config.get("NETWORK_INTERFACE", "wlan0");

    // Get current SSID from wpa_supplicant status
    std::string command = "wpa_cli -i " + interface + " status 2>/dev/null | grep '^ssid=' | cut -d= -f2";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string ssid(buffer);
        if (!ssid.empty() && ssid.back() == '\n') {
            ssid.pop_back();
        }
        pclose(pipe);

        // Return empty if empty or not in completed state
        if (ssid.empty()) {
            return "";
        }

        // Verify we're actually connected
        if (!isConnected()) {
            return "";
        }

        return ssid;
    }

    pclose(pipe);
    return "";
}
