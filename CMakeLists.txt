cmake_minimum_required(VERSION 3.10)
project(maestro-captive)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/controllers/CaptivePortalController.cpp
    src/controllers/WiFiController.cpp
    src/controllers/ServiceController.cpp
    src/services/WiFiManagerService.cpp
    src/services/HomeAssistantService.cpp
    src/services/HardwareSecurityService.cpp
    src/utils/WiFiUtils.cpp
    src/utils/ConfigManager.cpp
    src/utils/Logger.cpp
)

# Create executable
add_executable(maestro-captive ${SOURCES})

# Link libraries
target_link_libraries(maestro-captive
    Drogon::Drogon
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Install target
install(TARGETS maestro-captive DESTINATION bin)
