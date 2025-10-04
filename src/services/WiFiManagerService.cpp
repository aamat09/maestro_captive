#include "services/WiFiManagerService.h"
#include "utils/WiFiUtils.h"
#include <thread>
#include <chrono>

WiFiManagerService& WiFiManagerService::getInstance() {
    static WiFiManagerService instance;
    return instance;
}

bool WiFiManagerService::initialize() {
    initialized = true;
    return true;
}

std::vector<WiFiNetwork> WiFiManagerService::scanNetworks(bool full_scan) {
    // If not doing full scan and we have cached results, return them
    if (!full_scan) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (!cachedNetworks.empty()) {
            return cachedNetworks;
        }
    }

    // Perform actual scan
    auto scanResults = WiFiUtils::scanNetworks(full_scan);
    std::vector<WiFiNetwork> networks;

    for (const auto& result : scanResults) {
        WiFiNetwork network;
        network.ssid = result.ssid;
        network.signal = result.signal_strength;
        network.security = result.security;
        networks.push_back(network);
    }

    // Update cache
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        cachedNetworks = networks;
    }

    return networks;
}

bool WiFiManagerService::connectToNetwork(const std::string& ssid, const std::string& password) {
    // First disconnect from any current network
    std::string currentSSID = WiFiUtils::getCurrentSSID();
    if (!currentSSID.empty()) {
        std::string disconnectCmd = "nmcli connection down '" + currentSSID + "' 2>/dev/null";
        system(disconnectCmd.c_str());

        // Wait a moment for disconnect
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Attempt connection
    bool success = WiFiUtils::connectToNetwork(ssid, password);

    if (success) {
        // Validate connection by checking if we can get an IP and the SSID matches
        for (int i = 0; i < 15; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            auto status = getConnectionStatus();
            if (status.connected && status.ssid == ssid) {
                return true;
            }
        }
        // Connection didn't validate within timeout
        return false;
    }

    return false;
}

WiFiStatus WiFiManagerService::getConnectionStatus() {
    WiFiStatus status;
    status.connected = WiFiUtils::isConnected();
    status.ssid = WiFiUtils::getCurrentSSID();
    
    if (status.connected) {
        // Get signal strength for current connection
        FILE* pipe = popen(("nmcli -t -f SIGNAL dev wifi | grep -A1 '" + status.ssid + "' | tail -1").c_str(), "r");
        if (pipe) {
            char buffer[32];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string signal_str(buffer);
                if (!signal_str.empty() && signal_str.back() == '\n') {
                    signal_str.pop_back();
                }
                status.signal = signal_str.empty() ? 0 : std::stoi(signal_str);
            }
            pclose(pipe);
        }
    } else {
        status.signal = 0;
    }
    
    return status;
}

bool WiFiManagerService::disconnect() {
    std::string currentSSID = WiFiUtils::getCurrentSSID();
    if (currentSSID.empty()) {
        return true; // Already disconnected
    }

    std::string disconnectCmd = "nmcli connection down '" + currentSSID + "'";
    int result = system(disconnectCmd.c_str());
    return result == 0;
}

bool WiFiManagerService::validateInternetConnectivity() {
    // Test internet connectivity with multiple methods
    
    // Method 1: Ping Google DNS
    int result1 = system("ping -c 1 -W 3 8.8.8.8 >/dev/null 2>&1");
    if (result1 == 0) return true;
    
    // Method 2: Ping Cloudflare DNS
    int result2 = system("ping -c 1 -W 3 1.1.1.1 >/dev/null 2>&1");
    if (result2 == 0) return true;
    
    // Method 3: Try HTTP request to a reliable endpoint
    int result3 = system("curl -s --connect-timeout 5 --max-time 10 http://detectportal.firefox.com/success.txt | grep -q success");
    if (result3 == 0) return true;
    
    return false;
}
