#include "utils/WiFiUtils.h"
#include "utils/ConfigManager.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <regex>

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

    // Use nmcli to scan for networks on specified interface, filter out empty SSIDs
    std::string command = "nmcli -t -f SSID,SIGNAL,SECURITY dev wifi list ifname " + interface + " 2>/dev/null | grep -v '^:' | grep -v '^$'";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return results;
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        if (line.empty()) continue;
        
        // Remove newline
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        
        // Parse nmcli output format: SSID:SIGNAL:SECURITY
        size_t first_colon = line.find(':');
        size_t second_colon = line.find(':', first_colon + 1);
        
        if (first_colon != std::string::npos && second_colon != std::string::npos) {
            WiFiScanResult result;
            result.ssid = line.substr(0, first_colon);
            
            std::string signal_str = line.substr(first_colon + 1, second_colon - first_colon - 1);
            result.signal_strength = signal_str.empty() ? 0 : std::stoi(signal_str);
            
            result.security = line.substr(second_colon + 1);
            
            if (!result.ssid.empty()) {
                results.push_back(result);
            }
        }
    }
    
    pclose(pipe);
    return results;
}

bool WiFiUtils::connectToNetwork(const std::string& ssid, const std::string& password) {
    #if !PLATFORM_LINUX
    return false;
    #endif

    auto& config = ConfigManager::getInstance();
    std::string interface = config.get("NETWORK_INTERFACE", "wlan0");

    std::string command = "nmcli dev wifi connect '" + ssid + "' password '" + password + "' ifname " + interface;
    int result = system(command.c_str());
    return result == 0;
}

bool WiFiUtils::isConnected() {
    #if !PLATFORM_LINUX
    return false;
    #endif

    int result = system("nmcli -t -f STATE general | grep -q connected");
    return result == 0;
}

std::string WiFiUtils::getCurrentSSID() {
    #if !PLATFORM_LINUX
    return "";
    #endif

    // Get network interface from config
    auto& config = ConfigManager::getInstance();
    std::string interface = config.get("NETWORK_INTERFACE", "wlan0");

    // Get the active WiFi connection for the specified interface
    std::string command = "nmcli -t -f GENERAL.CONNECTION dev show " + interface + " 2>/dev/null | cut -d: -f2";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string ssid(buffer);
        if (!ssid.empty() && ssid.back() == '\n') {
            ssid.pop_back();
        }
        pclose(pipe);

        // Return empty if it's "--" (not connected) or empty
        if (ssid == "--" || ssid.empty()) {
            return "";
        }

        return ssid;
    }

    pclose(pipe);
    return "";
}
