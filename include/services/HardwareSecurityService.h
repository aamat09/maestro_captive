#pragma once

class HardwareSecurityService {
public:
    static HardwareSecurityService& getInstance();
    bool initialize();
    bool validateLicense();
    
private:
    HardwareSecurityService() = default;
};
