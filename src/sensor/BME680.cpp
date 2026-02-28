#include "BME680.h"

namespace Sensor {

    BME680::BME680(uint8_t address) : I2CSensor(address) {
        bme = std::make_unique<Adafruit_BME680>(&wire);
    }

    bool BME680::begin() {
#ifdef ARDUINO
        Serial.println("BME680: Initializing sensor...");

        if (!bme->begin(i2cAddress)) {
            Serial.println("BME680: Failed to initialize sensor");
            return false;
        }

        // Configure oversampling and filter
        bme->setTemperatureOversampling(BME680_OS_8X);
        bme->setHumidityOversampling(BME680_OS_2X);
        bme->setPressureOversampling(BME680_OS_4X);
        bme->setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme->setGasHeater(320, 150); // 320°C for 150ms

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
        if (bme->performReading()) {
            float temperature = bme->temperature;
            float relative_humidity = bme->humidity;
            reading.measurements.push_back({MeasurementType::Temperature, temperature, getType(), false});
            reading.measurements.push_back({MeasurementType::RelativeHumidity, relative_humidity, getType(), false});
            reading.measurements.push_back({MeasurementType::DewPoint, calcDewPoint(temperature, relative_humidity), getType(), true});
            reading.measurements.push_back({MeasurementType::Pressure, bme->pressure / 100.0f, getType(), false});

            reading.valid = true;
        } else {
            reading.valid = false;
            Serial.println("BME680: Failed to read sensor data");
        }
#else
        float t = 23.0f;
        float rh = 50.0f;
        reading.measurements.push_back({MeasurementType::Temperature, t, getType(), false});
        reading.measurements.push_back({MeasurementType::RelativeHumidity, rh, getType(), false});
        reading.measurements.push_back({MeasurementType::Pressure, 1013.25f, getType(), false});

        reading.measurements.push_back({MeasurementType::DewPoint, calcDewPoint(t, rh), getType(), true});

        reading.valid = true;
#endif

        return reading;
    }

} // namespace Sensor
