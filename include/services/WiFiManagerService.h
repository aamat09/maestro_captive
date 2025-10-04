#pragma once

#include <vector>
#include <string>
#include <mutex>

struct WiFiNetwork {
    std::string ssid;
    int signal;
    std::string security;
};

struct WiFiStatus {
    bool connected;
    std::string ssid;
    int signal;
};

class WiFiManagerService {
public:
    static WiFiManagerService& getInstance();

    bool initialize();
    std::vector<WiFiNetwork> scanNetworks(bool full_scan = false);
    bool connectToNetwork(const std::string& ssid, const std::string& password);
    WiFiStatus getConnectionStatus();
    bool disconnect();
    bool validateInternetConnectivity();

private:
    WiFiManagerService() = default;
    bool initialized = false;
    std::vector<WiFiNetwork> cachedNetworks;
    std::mutex cacheMutex;
};
