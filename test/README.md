# Maestro Captive Portal - Test Directory

This directory contains all testing infrastructure for the Maestro Captive Portal application.

## Directory Structure

### `unit/`
Unit tests for individual components:
- WiFi utility functions
- Service controllers
- Configuration management
- Hardware security modules

### `temporary/`
Temporary test programs and validation scripts:
- WiFi scanning validation programs
- Network connection tests
- API endpoint testing
- Performance benchmarks

**Note**: All temporary files and compiled test binaries are created in `temporary/`
and should be cleaned up after use to keep the project root clean.

## Testing Guidelines

1. **All test files** should be placed in appropriate subdirectories
2. **Temporary binaries** should be compiled in `temporary/` and removed after testing
3. **Unit tests** should follow naming convention: `test_<component>.cpp`
4. **No test files** should be placed in the project root directory

## WiFi Testing Results

✅ **WiFi Scanning**: Successfully tested and validated
- Detected 7+ nearby networks
- Proper parsing of SSID, signal strength, and security
- Ready for captive portal integration

## Usage

```bash
# Compile and run temporary test
cd test/temporary/
g++ -o test_program test_program.cpp
./test_program
rm test_program  # Clean up after testing
```