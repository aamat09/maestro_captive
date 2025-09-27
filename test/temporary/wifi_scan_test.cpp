#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

struct WiFiNetwork {
    std::string ssid;
    int signal;
    std::string security;
};

std::vector<WiFiNetwork> scanNetworks() {
    std::vector<WiFiNetwork> results;
    
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
        
        // Parse: SSID:SIGNAL:SECURITY
        size_t first_colon = line.find(':');
        size_t second_colon = line.find(':', first_colon + 1);
        
        if (first_colon != std::string::npos && second_colon != std::string::npos) {
            WiFiNetwork network;
            network.ssid = line.substr(0, first_colon);
            
            std::string signal_str = line.substr(first_colon + 1, second_colon - first_colon - 1);
            network.signal = signal_str.empty() ? 0 : std::stoi(signal_str);
            
            network.security = line.substr(second_colon + 1);
            
            if (!network.ssid.empty()) {
                results.push_back(network);
            }
        }
    }
    
    pclose(pipe);
    return results;
}

int main() {
    std::cout << "=== WiFi Scanning Test ===" << std::endl;
    
    auto networks = scanNetworks();
    
    std::cout << "Found " << networks.size() << " networks:" << std::endl;
    
    for (size_t i = 0; i < networks.size() && i < 5; ++i) {
        const auto& net = networks[i];
        std::cout << (i+1) << ". SSID: '" << net.ssid 
                  << "' | Signal: " << net.signal 
                  << "% | Security: '" << net.security << "'" << std::endl;
    }
    
    return 0;
}
