#ifndef SENSOR_H
#define SENSOR_H

#include <cmath>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

namespace Sensor {
    enum class MeasurementType : uint8_t {
        Temperature,
        RelativeHumidity,
        DewPoint,
        Pressure,
        SeaLevelPressure,
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
            case MeasurementType::VocIndex: return "voc index";
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
    public:
        virtual ~Sensor() = default;

        virtual bool begin() = 0;

        virtual SensorReading read() = 0;

        virtual SensorReading read(const std::vector<Measurement> &prior) {
            (void) prior;
            return read();
        }

        virtual const char *getType() const = 0;

        virtual bool isConnected() = 0;

        virtual TypeSpan provides() const { return {nullptr, 0}; }
        virtual TypeSpan requires() const { return {nullptr, 0}; }
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
