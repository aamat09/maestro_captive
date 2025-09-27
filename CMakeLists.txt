cmake_minimum_required(VERSION 3.10)
project(maestro-captive)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

pkg_check_modules(LIBNL REQUIRED libnl-3.0 libnl-genl-3.0)

# Include directories
include_directories(include)
include_directories(${LIBNL_INCLUDE_DIRS})

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
)

# Create executable
add_executable(maestro-captive ${SOURCES})

# Link libraries
target_link_libraries(maestro-captive 
    Drogon::Drogon
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBNL_LIBRARIES}
)

# Compiler flags
target_compile_options(maestro-captive PRIVATE ${LIBNL_CFLAGS_OTHER})

# Install target
install(TARGETS maestro-captive DESTINATION bin)
