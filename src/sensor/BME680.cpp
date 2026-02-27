#include "BME680.h"

#ifdef ARDUINO
#include <Wire.h>
#endif

namespace Sensor {

    BME680::BME680(uint8_t address) : i2cAddress(address), initialized(false) {
    }

    bool BME680::begin() {
#ifdef ARDUINO
        Serial.println("BME680: Initializing sensor...");

        Wire1.setPins(SDA1, SCL1);

        if (!Wire1.begin()) {
            Serial.println("BME680: Failed to initialize I2C");
            return false;
        }

        if (!bme.begin(i2cAddress, &Wire1)) {
            Serial.println("BME680: Failed to initialize sensor");
            return false;
        }

        // Configure oversampling and filter
        bme.setTemperatureOversampling(BME680_OS_8X);
        bme.setHumidityOversampling(BME680_OS_2X);
        bme.setPressureOversampling(BME680_OS_4X);
        bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme.setGasHeater(320, 150); // 320°C for 150ms

        initialized = true;
        Serial.println("BME680: Sensor initialized successfully");
        return true;
#else
        initialized = true;
        return true;
#endif
    }

    SensorReading BME680::read() {
        SensorReading reading;
        reading.timestamp = millis();

        if (!initialized || !isConnected()) {
            reading.valid = false;
            return reading;
        }

#ifdef ARDUINO
        if (bme.performReading()) {
            reading.measurements.push_back({"temperature", bme.temperature, "°C", "BME680", false});
            reading.measurements.push_back({"humidity", bme.humidity, "%", "BME680", false});
            reading.measurements.push_back({"pressure", bme.pressure / 100.0f, "hPa", "BME680", false});
            reading.valid = true;
        } else {
            reading.valid = false;
            Serial.println("BME680: Failed to read sensor data");
        }
#else
        reading.measurements.push_back({"temperature", 23.0f, "°C", "BME680", false});
        reading.measurements.push_back({"humidity", 50.0f, "%", "BME680", false});
        reading.measurements.push_back({"pressure", 1013.25f, "hPa", "BME680", false});
        reading.valid = true;
#endif

        return reading;
    }

    const char* BME680::getName() const {
        return "BME680";
    }

    const char* BME680::getType() const {
        return "Environmental";
    }

    bool BME680::isConnected() {
#ifdef ARDUINO
        if (!initialized) return false;

        Wire1.beginTransmission(i2cAddress);
        return Wire1.endTransmission() == 0;
#else
        return initialized;
#endif
    }

} // namespace Sensor
