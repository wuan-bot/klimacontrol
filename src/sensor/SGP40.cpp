#include "SGP40.h"

namespace Sensor {

    SGP40::SGP40(uint8_t address) : I2CSensor(address) {
    }

    bool SGP40::begin() {
#ifdef ARDUINO
        Serial.println("SGP40: Initializing sensor...");

        if (!sgp.begin(&wire)) {
            Serial.println("SGP40: Failed to initialize sensor");
            return false;
        }

        initialized = true;
        Serial.println("SGP40: Sensor initialized successfully");
        return true;
#else
        initialized = true;
        return true;
#endif
    }

    SensorReading SGP40::read() {
        return read({});
    }

    SensorReading SGP40::read(const std::vector<Measurement>& prior) {
        SensorReading reading;
        reading.timestamp = millis();

        if (!initialized || !isConnected()) {
            reading.valid = false;
            return reading;
        }

        auto* tempM = findMeasurement(prior, MeasurementType::Temperature);
        auto* rhM = findMeasurement(prior, MeasurementType::RelativeHumidity);

#ifdef ARDUINO
        int32_t vocIndex = (tempM && rhM)
            ? sgp.measureVocIndex(std::get<float>(tempM->value), std::get<float>(rhM->value))
            : sgp.measureVocIndex();
        if (vocIndex >= 0) {
            reading.measurements.push_back({MeasurementType::VocIndex, vocIndex, getType(), false});
            reading.valid = true;
        } else {
            reading.valid = false;
            Serial.println("SGP40: Failed to read sensor data");
        }
#else
        reading.measurements.push_back({MeasurementType::VocIndex, (int32_t)100, getType(), false});
        reading.valid = true;
#endif

        return reading;
    }

} // namespace Sensor
