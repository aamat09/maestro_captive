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

std::vector<WiFiScanResult> WiFiUtils::scanNetworks(bool full_scan) {
    #if !PLATFORM_LINUX
    return std::vector<WiFiScanResult>();
    #endif

    if (full_scan) {
        return fullScanWithInterruption();
    }

    // Try cache first, fallback to virtual interface scan
    auto results = scanFromCache();
    if (!results.empty()) {
        return results;
    }

    return scanWithVirtualInterface();
}

std::vector<WiFiScanResult> WiFiUtils::scanFromCache() {
    std::vector<WiFiScanResult> results;

    #if !PLATFORM_LINUX
    return results;
    #endif

    std::ifstream cache_file("/var/cache/maestro/wifi-scan.json");
    if (!cache_file.is_open()) {
        return results;
    }

    std::stringstream buffer;
    buffer << cache_file.rdbuf();
    std::string json_str = buffer.str();

    // Simple JSON parsing for networks array
    size_t networks_pos = json_str.find("\"networks\":[");
    if (networks_pos == std::string::npos) return results;

    size_t start = networks_pos + 11;
    size_t end = json_str.find("]", start);
    if (end == std::string::npos) return results;

    std::string networks_json = json_str.substr(start, end - start);

    // Parse each network object (simple regex-based parsing)
    std::regex network_regex(R"(\{\"ssid\":\"([^\"]*)\",\"signal\":(\d+),\"security\":\"([^\"]*)\"\})");
    std::sregex_iterator iter(networks_json.begin(), networks_json.end(), network_regex);
    std::sregex_iterator end_iter;

    for (; iter != end_iter; ++iter) {
        WiFiScanResult result;
        result.ssid = (*iter)[1];
        result.signal_strength = std::stoi((*iter)[2]);
        result.security = (*iter)[3];
        // Filter out empty and hidden SSIDs (containing \x00)
        if (!result.ssid.empty() && result.ssid.find("\\x00") == std::string::npos) {
            results.push_back(result);
        }
    }

    return results;
}

std::vector<WiFiScanResult> WiFiUtils::scanWithVirtualInterface() {
    std::vector<WiFiScanResult> results;

    #if !PLATFORM_LINUX
    return results;
    #endif

    auto& config = ConfigManager::getInstance();
    std::string interface = config.get("NETWORK_INTERFACE", "wlan0");
    std::string scan_interface = interface + "_scan";

    // Create virtual interface for scanning if it doesn't exist
    bool created_vif = false;
    if (system(("ip link show " + scan_interface + " >/dev/null 2>&1").c_str()) != 0) {
        if (system(("iw dev " + interface + " interface add " + scan_interface + " type managed").c_str()) == 0) {
            created_vif = true;
            system(("ip link set " + scan_interface + " up").c_str());
        }
    } else {
        system(("ip link set " + scan_interface + " up").c_str());
    }

    std::string scan_dev = (created_vif || system(("ip link show " + scan_interface + " >/dev/null 2>&1").c_str()) == 0)
                          ? scan_interface : interface;

    system(("iw dev " + scan_dev + " scan > /dev/null 2>&1").c_str());

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

        if (line.find("SSID: ") != std::string::npos) {
            size_t pos = line.find("SSID: ") + 6;
            current_ssid = line.substr(pos);
            if (!current_ssid.empty() && current_ssid.back() == '\n') {
                current_ssid.pop_back();
            }
        }
        else if (line.find("signal: ") != std::string::npos) {
            size_t pos = line.find("signal: ") + 8;
            std::string signal_str = line.substr(pos);
            current_signal = std::stoi(signal_str);
        }
        else if (line.find("WPA:") != std::string::npos || line.find("RSN:") != std::string::npos) {
            current_security = "WPA";
        }
        else if (line.find("BSS ") == 0 && !current_ssid.empty()) {
            WiFiScanResult result;
            result.ssid = current_ssid;
            result.signal_strength = std::min(100, std::max(0, (current_signal + 100) * 2));
            result.security = current_security;
            results.push_back(result);
            current_ssid.clear();
            current_signal = 0;
            current_security.clear();
        }
    }

    if (!current_ssid.empty()) {
        WiFiScanResult result;
        result.ssid = current_ssid;
        result.signal_strength = std::min(100, std::max(0, (current_signal + 100) * 2));
        result.security = current_security;
        results.push_back(result);
    }

    pclose(pipe);

    if (created_vif) {
        system(("iw dev " + scan_interface + " del 2>/dev/null").c_str());
    }

    return results;
}

std::vector<WiFiScanResult> WiFiUtils::fullScanWithInterruption() {
    std::vector<WiFiScanResult> results;

    #if !PLATFORM_LINUX
    return results;
    #endif

    auto& config = ConfigManager::getInstance();
    std::string interface = config.get("NETWORK_INTERFACE", "wlan0");

    // Stop hostapd services
    system("systemctl stop maestro-dnsmasq maestro-hostapd 2>/dev/null");
    usleep(500000); // Wait 0.5s for services to stop

    // Bring interface up
    system(("ip link set " + interface + " up").c_str());

    // Full channel scan
    system(("iw dev " + interface + " scan > /dev/null 2>&1").c_str());

    std::string command = "iw dev " + interface + " scan 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        system("systemctl start maestro-hostapd maestro-dnsmasq 2>/dev/null");
        return results;
    }

    char buffer[512];
    std::string current_ssid;
    int current_signal = 0;
    std::string current_security;

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);

        if (line.find("SSID: ") != std::string::npos) {
            size_t pos = line.find("SSID: ") + 6;
            current_ssid = line.substr(pos);
            if (!current_ssid.empty() && current_ssid.back() == '\n') {
                current_ssid.pop_back();
            }
        }
        else if (line.find("signal: ") != std::string::npos) {
            size_t pos = line.find("signal: ") + 8;
            std::string signal_str = line.substr(pos);
            current_signal = std::stoi(signal_str);
        }
        else if (line.find("WPA:") != std::string::npos || line.find("RSN:") != std::string::npos) {
            current_security = "WPA";
        }
        else if (line.find("BSS ") == 0 && !current_ssid.empty()) {
            WiFiScanResult result;
            result.ssid = current_ssid;
            result.signal_strength = std::min(100, std::max(0, (current_signal + 100) * 2));
            result.security = current_security;
            results.push_back(result);
            current_ssid.clear();
            current_signal = 0;
            current_security.clear();
        }
    }

    if (!current_ssid.empty()) {
        WiFiScanResult result;
        result.ssid = current_ssid;
        result.signal_strength = std::min(100, std::max(0, (current_signal + 100) * 2));
        result.security = current_security;
        results.push_back(result);
    }

    pclose(pipe);

    // Restart hostapd
    system("systemctl start maestro-hostapd maestro-dnsmasq 2>/dev/null");

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
