#ifndef BME680_H
#define BME680_H

#include "Sensor.h"

#ifdef ARDUINO
#include <Adafruit_BME680.h>
#endif

namespace Sensor {

    class BME680 : public Sensor {
    private:
        uint8_t i2cAddress;

#ifdef ARDUINO
        Adafruit_BME680 bme;
#endif
        bool initialized;

    public:
        explicit BME680(uint8_t address = 0x77);

        bool begin() override;
        SensorReading read() override;
        const char* getName() const override;
        const char* getType() const override;
        bool isConnected() override;
    };

} // namespace Sensor

#endif // BME680_H
