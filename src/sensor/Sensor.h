#ifndef SENSOR_H
#define SENSOR_H

#include <cmath>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#ifdef ARDUINO
#include <Arduino.h>
#else
unsigned long millis();
#endif

namespace Sensor {
    enum class SensorStatus : uint8_t {
        Uninitialized,
        Online,
        InitFailed,
        ReadFailing,
    };

    inline const char *sensorStatusLabel(SensorStatus s) {
        switch (s) {
            case SensorStatus::Uninitialized: return "uninitialized";
            case SensorStatus::Online: return "online";
            case SensorStatus::InitFailed: return "init_failed";
            case SensorStatus::ReadFailing: return "read_failing";
        }
        return "unknown";
    }

    enum class MeasurementType : uint8_t {
        Temperature,
        RelativeHumidity,
        DewPoint,
        Pressure,
        SeaLevelPressure,
        CO2,
        Illuminance,
        Particles03,
        Particles05,
        Particles10,
        Particles25,
        Particles50,
        Particles100,
        PM10Concentration,
        PM25Concentration,
        PM100Concentration,
        VocIndex,
        Rssi,
        Channel,
        System,
        FreeHeap,
        Uptime,
        Time
    };

    inline const char *measurementTypeLabel(MeasurementType t) {
        switch (t) {
            case MeasurementType::Temperature: return "temperature";
            case MeasurementType::RelativeHumidity: return "relative humidity";
            case MeasurementType::DewPoint: return "dew point";
            case MeasurementType::Pressure: return "pressure";
            case MeasurementType::SeaLevelPressure: return "sea level pressure";
            case MeasurementType::CO2: return "CO2";
            case MeasurementType::Illuminance: return "illuminance";
            case MeasurementType::Particles03: return "particles_03";
            case MeasurementType::Particles05: return "particles_05";
            case MeasurementType::Particles10: return "particles_10";
            case MeasurementType::Particles25: return "particles_25";
            case MeasurementType::Particles50: return "particles_50";
            case MeasurementType::Particles100: return "particles_100";
            case MeasurementType::PM10Concentration: return "part_con_10";
            case MeasurementType::PM25Concentration: return "part_con_25";
            case MeasurementType::PM100Concentration: return "part_con_100";
            case MeasurementType::VocIndex: return "VOC index";
            case MeasurementType::Rssi: return "rssi";
            case MeasurementType::Channel: return "channel";
            case MeasurementType::System: return "system";
            case MeasurementType::FreeHeap: return "free_heap";
            case MeasurementType::Uptime: return "uptime";
            case MeasurementType::Time: return "time";
        }
        return "unknown";
    }

    inline const char *measurementTypeUnit(MeasurementType t) {
        switch (t) {
            case MeasurementType::Temperature: return "°C";
            case MeasurementType::RelativeHumidity: return "%";
            case MeasurementType::DewPoint: return "°C";
            case MeasurementType::Pressure: return "hPa";
            case MeasurementType::SeaLevelPressure: return "hPa";
            case MeasurementType::CO2: return "ppm";
            case MeasurementType::Illuminance: return "lux";
            case MeasurementType::Particles03: return "#";
            case MeasurementType::Particles05: return "#";
            case MeasurementType::Particles10: return "#";
            case MeasurementType::Particles25: return "#";
            case MeasurementType::Particles50: return "#";
            case MeasurementType::Particles100: return "#";
            case MeasurementType::PM10Concentration: return "ug/m^3";
            case MeasurementType::PM25Concentration: return "ug/m^3";
            case MeasurementType::PM100Concentration: return "ug/m^3";
            case MeasurementType::VocIndex: return "";
            case MeasurementType::Rssi: return "dBm";
            case MeasurementType::Channel: return "";
            case MeasurementType::System: return "°C";
            case MeasurementType::FreeHeap: return "kB";
            case MeasurementType::Uptime: return "s";
            case MeasurementType::Time: return "ms";
        }
        return "";
    }

    struct TypeSpan {
        const MeasurementType *data;
        uint8_t count;
    };

    using Value = std::variant<float, int32_t>;

    struct Measurement {
        MeasurementType type;
        Value value;
        const char *sensor; // e.g., "SHT4x", "BME680"
        bool calculated; // true for derived values (dew point, sea-level pressure)
    };

    // Magnus formula dew point from temperature (°C) and relative humidity (%)
    inline float calcDewPoint(float temperature, float humidity) {
        constexpr float a = 17.625f;
        constexpr float b = 243.04f;
        float gamma = a * temperature / (b + temperature) + logf(humidity / 100.0f);
        return b * gamma / (a - gamma);
    }

    // Hypsometric formula: sea-level pressure from station pressure, temperature (°C), and elevation (m)
    inline float calcSeaLevelPressure(float pressure, float temperatureCelsius, float elevation) {
        float temperatureK = temperatureCelsius + 273.15f;
        return pressure * powf(temperatureK / (temperatureK + 0.0065f * elevation), -5.255f);
    }

    struct ReadConfig {
        float elevation = 0.0f; // meters above sea level
    };

    inline const Measurement *findMeasurement(const std::vector<Measurement> &measurements, MeasurementType type) {
        for (const auto &m: measurements) {
            if (m.type == type) return &m;
        }
        return nullptr;
    }

    struct SensorReading {
        std::vector<Measurement> measurements;
        uint32_t timestamp;
        bool valid;

        SensorReading() : timestamp(0), valid(false) {
        }
    };

    /**
     * Base sensor interface
     */
    class Sensor {
    protected:
        SensorStatus sensorStatus = SensorStatus::Uninitialized;
        uint32_t lastInitAttempt = 0;
        uint8_t consecutiveReadFailures = 0;
        static constexpr uint8_t READ_FAILURE_THRESHOLD = 10;

    public:
        virtual ~Sensor() = default;

        virtual bool begin() = 0;

        /**
         * Wraps begin() with status tracking
         */
        bool tryBegin() {
            lastInitAttempt = millis();
            if (begin()) {
                sensorStatus = SensorStatus::Online;
                consecutiveReadFailures = 0;
                return true;
            }
            sensorStatus = SensorStatus::InitFailed;
            return false;
        }

        /**
         * Record whether the last read() produced valid data
         */
        void recordReadResult(bool valid) {
            if (valid) {
                consecutiveReadFailures = 0;
                if (sensorStatus == SensorStatus::ReadFailing) {
                    sensorStatus = SensorStatus::Online;
                }
            } else if (sensorStatus == SensorStatus::Online) {
                consecutiveReadFailures++;
                if (consecutiveReadFailures >= READ_FAILURE_THRESHOLD) {
                    sensorStatus = SensorStatus::ReadFailing;
                }
            }
        }

        virtual SensorReading read() = 0;

        virtual SensorReading read(const ReadConfig &config, const std::vector<Measurement> &prior) {
            (void) config;
            (void) prior;
            return read();
        }

        virtual const char *getType() const = 0;

        virtual bool isConnected() = 0;

        [[nodiscard]] SensorStatus getStatus() const { return sensorStatus; }
        [[nodiscard]] uint32_t getLastInitAttempt() const { return lastInitAttempt; }
        [[nodiscard]] uint8_t getConsecutiveReadFailures() const { return consecutiveReadFailures; }

        [[nodiscard]] virtual TypeSpan provides() const { return {nullptr, 0}; }
        [[nodiscard]] virtual TypeSpan requires() const { return {nullptr, 0}; }
        [[nodiscard]] virtual size_t measurementCount() const { return provides().count; }
    };

    /**
     * Sensor factory interface for creating sensor instances
     */
    class SensorFactory {
    public:
        virtual ~SensorFactory() = default;

        virtual std::unique_ptr<Sensor> createSensor() = 0;
    };
} // namespace Sensor

#endif // SENSOR_H
