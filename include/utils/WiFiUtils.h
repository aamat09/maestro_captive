#pragma once
#include <vector>
#include <string>

struct WiFiScanResult {
    std::string ssid;
    int signal_strength;
    std::string security;
};

class WiFiUtils {
public:
    static std::vector<WiFiScanResult> scanNetworks();
    static bool connectToNetwork(const std::string& ssid, const std::string& password);
    static bool isConnected();
    static std::string getCurrentSSID();
};
