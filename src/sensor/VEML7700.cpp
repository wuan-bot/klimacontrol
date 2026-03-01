#include "VEML7700.h"

namespace Sensor {

    VEML7700::VEML7700(uint8_t address) : I2CSensor(address) {
    }

    bool VEML7700::begin() {
#ifdef ARDUINO
        Serial.println("VEML7700: Initializing sensor...");

        if (!veml.begin(&wire)) {
            Serial.println("VEML7700: Failed to initialize sensor");
            return false;
        }

        veml.setGain(VEML7700_GAIN_1);
        veml.setIntegrationTime(VEML7700_IT_100MS);

        initialized = true;
        Serial.println("VEML7700: Sensor initialized successfully");
        return true;
#else
        initialized = true;
        return true;
#endif
    }

    SensorReading VEML7700::read() {
        SensorReading reading;
        reading.timestamp = millis();

        if (!initialized || !isConnected()) {
            reading.valid = false;
            return reading;
        }

#ifdef ARDUINO
        float lux = veml.readLux();

        if (lux >= 0.0f) {
            reading.measurements.push_back({MeasurementType::Illuminance, lux, getType(), false});
            reading.valid = true;
        } else {
            reading.valid = false;
            Serial.println("VEML7700: Failed to read sensor data");
        }
#else
        reading.measurements.push_back({MeasurementType::Illuminance, 250.0f, getType(), false});
        reading.valid = true;
#endif

        return reading;
    }

} // namespace Sensor
