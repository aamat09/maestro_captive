#include "utils/WiFiUtils.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <regex>

std::vector<WiFiScanResult> WiFiUtils::scanNetworks() {
    std::vector<WiFiScanResult> results;
    
    // Use nmcli to scan for networks, filter out empty SSIDs
    FILE* pipe = popen("nmcli -t -f SSID,SIGNAL,SECURITY dev wifi | grep -v '^:' | grep -v '^$'", "r");
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
    std::string command = "nmcli dev wifi connect '" + ssid + "' password '" + password + "'";
    int result = system(command.c_str());
    return result == 0;
}

bool WiFiUtils::isConnected() {
    int result = system("nmcli -t -f STATE general | grep -q connected");
    return result == 0;
}

std::string WiFiUtils::getCurrentSSID() {
    FILE* pipe = popen("nmcli -t -f NAME connection show --active | head -1", "r");
    if (!pipe) return "";
    
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string ssid(buffer);
        if (!ssid.empty() && ssid.back() == '\n') {
            ssid.pop_back();
        }
        pclose(pipe);
        return ssid;
    }
    
    pclose(pipe);
    return "";
}
