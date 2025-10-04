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
    static std::vector<WiFiScanResult> scanNetworks(bool full_scan = false);
    static bool connectToNetwork(const std::string& ssid, const std::string& password);
    static bool isConnected();
    static std::string getCurrentSSID();
private:
    static std::vector<WiFiScanResult> scanFromCache();
    static std::vector<WiFiScanResult> scanWithVirtualInterface();
    static std::vector<WiFiScanResult> fullScanWithInterruption();
};
