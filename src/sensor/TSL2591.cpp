#include "TSL2591.h"

namespace Sensor {

    TSL2591::TSL2591(uint8_t address) : I2CSensor(address) {
    }

    bool TSL2591::begin() {
#ifdef ARDUINO
        Serial.println("TSL2591: Initializing sensor...");

        if (!tsl.begin(&wire, i2cAddress)) {
            Serial.println("TSL2591: Failed to initialize sensor");
            return false;
        }

        tsl.setGain(TSL2591_GAIN_MED);
        tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);

        initialized = true;
        Serial.println("TSL2591: Sensor initialized successfully");
        return true;
#else
        initialized = true;
        return true;
#endif
    }

    SensorReading TSL2591::read() {
        SensorReading reading;
        reading.timestamp = millis();

        if (!initialized || !isConnected()) {
            reading.valid = false;
            return reading;
        }

#ifdef ARDUINO
        uint32_t lum = tsl.getFullLuminosity();
        uint16_t ir = lum >> 16;
        uint16_t full = lum & 0xFFFF;
        float lux = tsl.calculateLux(full, ir);

        if (lux >= 0.0f) {
            reading.measurements.push_back({MeasurementType::Illuminance, lux, getType(), false});
            reading.valid = true;
        } else {
            reading.valid = false;
            Serial.println("TSL2591: Sensor saturated or error");
        }
#else
        reading.measurements.push_back({MeasurementType::Illuminance, 250.0f, getType(), false});
        reading.valid = true;
#endif

        return reading;
    }

} // namespace Sensor
