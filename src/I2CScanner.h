#ifndef I2CSCANNER_H
#define I2CSCANNER_H

#include <cstdint>
#include <vector>

struct I2CDevice {
    uint8_t address;
    const char* knownType;  // nullptr if unknown
};

namespace I2CScanner {
    std::vector<I2CDevice> scan();
    const char* identifyAddress(uint8_t address);
}

#endif // I2CSCANNER_H
