#include <sstream>

#include "Constants.h"

#ifdef ARDUINO
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#endif

#include "Network.h"
#include "Config.h"
#include "DeviceId.h"
#include "OTAUpdater.h"
#include "WebServerManager.h"

#ifdef ARDUINO
#include <esp_pm.h>
#include <esp_task_wdt.h>
#endif

#ifdef ARDUINO
#include <set>
#endif

Network::Network(Config::ConfigManager &config, SensorController &sensorController, Task::SensorMonitor &sensorMonitor)
    : config(config), sensorController(sensorController), sensorMonitor(sensorMonitor), mode(NetworkMode::NONE), webServer(nullptr), statusLed(nullptr), lastMqttPublish(0)
#ifdef ARDUINO
      , ntpClient(wifiUdp)
#endif
{
}

String Network::generateHostname() {
#ifdef ARDUINO
    String deviceId = DeviceId::getDeviceId();
    String hostname = Constants::HOSTNAME_PREFIX + deviceId;
    hostname.toLowerCase();
    return hostname;
#else
    return Constants::PROJECT_NAME;
#endif
}

void Network::configureMDNS() {
#ifdef ARDUINO
    String hostname = generateHostname();

    if (MDNS.begin(hostname.c_str())) {
        Serial.print("mDNS responder started: ");
        Serial.print(hostname);
        Serial.println(".local");

        Config::DeviceConfig deviceConfig = config.loadDeviceConfig();
        bool deviceNameNotEmpty = deviceConfig.device_name[0] != '\0';
        bool deviceNameIsNotDeviceId = strcmp(deviceConfig.device_name, deviceConfig.device_id) != 0;
        bool hasCustomName = (deviceNameNotEmpty && deviceNameIsNotDeviceId);

        if (hasCustomName) {
            mdnsInstanceName = Constants::INSTANCE_NAME_PREFIX + String(deviceConfig.device_name);
        } else {
            mdnsInstanceName = Constants::INSTANCE_NAME_PREFIX + String(deviceConfig.device_id);
        }

        Serial.printf("mDNS instance name: '%s'\r\n", mdnsInstanceName.c_str());
        MDNS.setInstanceName(mdnsInstanceName.c_str());

        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("Error starting mDNS responder!");
    }
#endif
}

void Network::setStatusLedState(LedState state) {
    if (statusLed) {
        statusLed->setState(state);
    }
}

LedState Network::getStatusLedState() const {
    if (statusLed) {
        return statusLed->getState();
    }
    return LedState::OFF;
}

void Network::startAP() {
#ifdef ARDUINO
    mode = NetworkMode::AP;

    // Get device ID for AP SSID
    String ap_ssid = Constants::AP_SSID_PREFIX + DeviceId::getDeviceId();

    Serial.print("Starting Access Point: ");
    Serial.println(ap_ssid.c_str());

    // Start open AP (no password)
    WiFi.softAP(ap_ssid.c_str());

    IPAddress ip_address = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(ip_address);

    configureMDNS();

    // Start captive portal (redirects all DNS to this device)
    captivePortal.begin();

#endif
}

void Network::startSTA(const char *ssid, const char *password) {
#ifdef ARDUINO
    mode = NetworkMode::STA;

    // Disconnect any previous connection
    WiFi.disconnect(true);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(WIFI_PS_NONE);

    // Apply WiFi TX power from energy config
    Config::EnergyConfig energyConfig = config.loadEnergyConfig();
    WiFi.setTxPower(static_cast<wifi_power_t>(energyConfig.wifi_power));

    Serial.println("WiFi Configuration:");
    Serial.printf("  TX Power: %d (raw wifi_power_t)\r\n", WiFi.getTxPower());
    Serial.printf("  Sleep Mode: %d (0=NONE)\r\n", WiFi.getSleep());

    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi ...");
    Serial.println(ssid);

    // Wait for connection with timeout
    int attempts = 0;
    const int maxAttempts = 30; // 15 seconds

    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");

        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // Print detailed WiFi diagnostics
        Serial.println("\r\nWiFi Diagnostics:");
        Serial.printf("  SSID: %s\r\n", WiFi.SSID().c_str());
        Serial.printf("  BSSID: %s\r\n", WiFi.BSSIDstr().c_str());
        Serial.printf("  Channel: %d\r\n", WiFi.channel());
        Serial.printf("  RSSI: %d dBm\r\n", WiFi.RSSI());
        Serial.printf("  MAC: %s\r\n", WiFi.macAddress().c_str());
        Serial.printf("  Gateway: %s\r\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("  DNS: %s\r\n", WiFi.dnsIP().toString().c_str());
        Serial.printf("  TX Power: %d\r\n", WiFi.getTxPower());
        Serial.printf("  Sleep Mode: %d (0=NONE)\r\n", WiFi.getSleep());
        Serial.printf("  Auto Reconnect: %d\r\n", WiFi.getAutoReconnect());
        Serial.println();

        configureMDNS();

        String hostname = generateHostname();
        Serial.print("You can now access ");
        Serial.print(Constants::PROJECT_NAME);
        Serial.println(" at:");
        Serial.print("  http://");
        Serial.print(hostname);
        Serial.println(".local/");
        Serial.print("  or http://");
        Serial.println(WiFi.localIP());

        // Start NTP client
        ntpClient.begin();
        ntpClient.update();

        Serial.print("NTP time: ");
        Serial.println(ntpClient.getFormattedTime());

        // Initialize MQTT client
        mqttClient = std::make_unique<MqttClient>();
        Config::MqttConfig mqttConfig = config.loadMqttConfig();
        mqttClient->begin(mqttConfig);
    } else {
        Serial.println("\r\nConnection failed");
    }
#endif
}

void Network::configureUsingAPMode() {
    // Start Access Point mode
    startAP();

    // Create and start config webserver for AP mode
    webServer = std::make_unique<ConfigWebServerManager>(config, *this, sensorController, sensorMonitor);
    webServer->begin();

    // Wait for configuration
    while (!config.isConfigured()) {
        captivePortal.handleClient(); // Handle DNS requests
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Serial.println("Configuration received");

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    Serial.println("Stopping captive portal");
    captivePortal.end();

    Serial.println("Scheduling restart");
    config.requestRestart(1000);

    // Stay in loop until main loop restarts us
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

[[noreturn]] void Network::task() {
#ifdef ARDUINO
    esp_task_wdt_add(NULL);

    Serial.println("Network task started");

    // Initialize status LED early (works without WiFi) - uses built-in NeoPixel
    statusLed = std::make_unique<StatusLed>(); // Built-in NeoPixel, 1 pixel
    statusLed->begin();
    statusLed->setState(LedState::STARTUP); // Indicate booting
    
    if (!config.isConfigured()) {
        Serial.println("No WiFi configuration found - starting AP mode");
        
        configureUsingAPMode();
    }
    Serial.println("Network task configured");

    // Check connection failure count — fall back to AP mode after repeated failures
    static constexpr uint8_t AP_FALLBACK_THRESHOLD = 3;
    uint8_t failures = config.getConnectionFailures();
    Serial.printf("Previous connection failures: %u\r\n", failures);

    if (failures >= AP_FALLBACK_THRESHOLD) {
        // Temporarily open AP so the user can reconfigure WiFi credentials.
        // If nobody reconfigures within the timeout, restart back into STA to
        // retry the original credentials (handles temporary WLAN outages).
        config.resetConnectionFailures();

        static constexpr unsigned long AP_FALLBACK_TIMEOUT_MS = 5UL * 60 * 1000; // 5 minutes
        Serial.printf("Too many connection failures - opening AP for %lu s\r\n",
                      AP_FALLBACK_TIMEOUT_MS / 1000);

        startAP();
        webServer = std::make_unique<ConfigWebServerManager>(config, *this, sensorController, sensorMonitor);
        webServer->begin();

        unsigned long apStart = millis();
        while (!config.isConfigured() && (millis() - apStart < AP_FALLBACK_TIMEOUT_MS)) {
            esp_task_wdt_reset();
            captivePortal.handleClient();
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        captivePortal.end();
        webServer.reset();

        if (config.isConfigured()) {
            Serial.println("New configuration received - restarting...");
        } else {
            Serial.println("AP fallback timed out - restarting to retry STA...");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP.restart();
    }

    // Load WiFi configuration
    Config::WiFiConfig wifiConfig = config.loadWiFiConfig();

    Serial.println("WiFi configured - starting STA mode");

    // Start Station mode
    startSTA(wifiConfig.ssid, wifiConfig.password);

    // Check if connection succeeded
    if (WiFi.status() != WL_CONNECTED) {
        uint8_t newFailures = config.incrementConnectionFailures();
        Serial.printf("Failed to connect (failure %u/%u) - restarting...\r\n",
                      newFailures, AP_FALLBACK_THRESHOLD);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP.restart();
    }

    // Connection successful - reset failure counter
    config.resetConnectionFailures();
    
    if (statusLed) {
        statusLed->setState(LedState::ON); // Solid on for connected state
    }

    // Create and start operational webserver for STA mode
    webServer = std::make_unique<OperationalWebServerManager>(config, *this, sensorController, sensorMonitor);
    webServer->begin();

    Serial.println("Webserver started - system ready");
    Serial.printf("Free heap: %u bytes\r\n", ESP.getFreeHeap());

    // Main loop - NTP updates and touch control
    auto lastNtpUpdate = ntpClient.getEpochTime();

    unsigned long lastCheck = millis();
    unsigned long lastSecond = millis();
    unsigned long lastDiagnostics = millis();
    unsigned long lastNtpRetry = 0; // millis() of last NTP retry when unsynced
    bool wasConnected = true;  // Track WiFi state transitions for mDNS re-advertisement
    static constexpr uint32_t MIN_FREE_HEAP_BYTES = 16384; // 16 KB
    static constexpr unsigned long DIAGNOSTICS_INTERVAL_MS = 300000; // 5 minutes
    static constexpr unsigned long NTP_UNSYNCED_RETRY_MS = 60000; // 1 minute
    while (true) {
        vTaskDelay(500 / portTICK_PERIOD_MS);

        esp_task_wdt_reset();

        unsigned long now = millis();

        if (statusLed) {
            statusLed->update();
        }

        // if (touchController) {
        //     touchController->update();
        // }

        // Low heap check - restart cleanly before an OOM crash
        // Skip during OTA: TLS buffers temporarily consume most internal SRAM
        uint32_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < MIN_FREE_HEAP_BYTES && !OTAUpdater::isUpdateInProgress()) {
            Serial.printf("CRITICAL: Low heap %u bytes - restarting...\r\n", freeHeap);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            ESP.restart();
        }

        if (now - lastSecond >= 1000) {
            lastSecond = now;

            // WiFi state tracking — auto-reconnect is handled by WiFi.setAutoReconnect(true)
            bool isConnected = WiFi.status() == WL_CONNECTED;
            if (now - lastCheck > 1100) {
                Serial.printf("WiFi status: %d delayed %lu\r\n", WiFi.status(), now - lastCheck);
            }
            lastCheck = now;

            if (!wasConnected && isConnected) {
                Serial.printf("WiFi reconnected (IP: %s)\r\n", WiFi.localIP().toString().c_str());
                configureMDNS();
            } else if (wasConnected && !isConnected) {
                Serial.println("WiFi disconnected - waiting for auto-reconnect");
            }
            wasConnected = isConnected;

            // NTP update
            uint32_t currentEpoch = ntpClient.getEpochTime();
            static constexpr uint32_t NTP_UPDATE_INTERVAL_S = 3600;

            if (currentEpoch > 0) {
                // Already synced, check for periodic update
                if (lastNtpUpdate == 0 || currentEpoch - lastNtpUpdate >= NTP_UPDATE_INTERVAL_S) {
                    bool result = ntpClient.update();
                    if (result) {
                        lastNtpUpdate = ntpClient.getEpochTime();
                        Serial.print("NTP update: ");
                        Serial.println(ntpClient.getFormattedTime());
                    } else {
                        Serial.println("NTP update failed");
                    }
                }
            } else if (now - lastNtpRetry >= NTP_UNSYNCED_RETRY_MS) {
                // NTP not yet synced - retry at most once per minute
                lastNtpRetry = now;
                if (ntpClient.update()) {
                    lastNtpUpdate = ntpClient.getEpochTime();
                    Serial.print("NTP initial sync: ");
                    Serial.println(ntpClient.getFormattedTime());
                }
            }

            // MQTT: reconnect/keepalive + publish measurements
            if (mqttClient) {
                mqttClient->loop();

                if (mqttClient->isConnected() && sensorController.isDataValid()) {
                    uint32_t intervalMs = mqttClient->getIntervalMs();

                    // Seed lastMqttPublish on first eligible cycle
                    if (lastMqttPublish == 0) lastMqttPublish = now;

                    if (intervalMs > 0 && now >= 60000 && (now - lastMqttPublish >= intervalMs)) {
                        statusLed->setState(LedState::TRANSMIT_DATA);

                        lastMqttPublish = now;
                        publishMeasurements(sensorController.getMeasurements());
                    }

                    // Update MQTT progress for green→red gradient on status LED
                    if (statusLed && intervalMs > 0) {
                        float prog = static_cast<float>(now - lastMqttPublish) / intervalMs;
                        if (prog > 1.0f) prog = 1.0f;
                        statusLed->setProgress(prog);
                        statusLed->setState(LedState::ON);
                    }
                } else if (statusLed) {
                    statusLed->setProgress(0.0f);
                }
            }

            // Periodic diagnostics: heap and task stack high-water marks
            if (now - lastDiagnostics >= DIAGNOSTICS_INTERVAL_MS) {
                lastDiagnostics = now;
                Serial.printf("Diagnostics: heap=%u bytes (min=%u), uptime=%lu s\r\n",
                              ESP.getFreeHeap(), ESP.getMinFreeHeap(), now / 1000);
                if (taskHandle) {
                    Serial.printf("  Network task stack HWM: %u bytes\r\n",
                                  uxTaskGetStackHighWaterMark(taskHandle) * sizeof(StackType_t));
                }
            }
        }
    }
#endif
}

void Network::publishMeasurements(const std::vector<Sensor::Measurement>& measurements) {
#ifdef ARDUINO
    if (!mqttClient || !mqttClient->isConnected()) return;

    uint32_t epoch = getCurrentEpoch();
    Config::MqttConfig mqttConfig = config.loadMqttConfig();

    uint32_t succeeded = 0;
    uint32_t failed = 0;

    for (const auto& m : measurements) {
        char topic[128];
        snprintf(topic, sizeof(topic), "%s/%s", mqttConfig.prefix, Sensor::measurementTypeLabel(m.type));

        char payload[256];
        if (auto* i = std::get_if<int32_t>(&m.value)) {
            snprintf(payload, sizeof(payload),
                "{\"time\":%u,\"value\":%d,\"unit\":\"%s\",\"sensor\":\"%s\",\"calculated\":%s}",
                epoch, *i, Sensor::measurementTypeUnit(m.type), m.sensor, m.calculated ? "true" : "false");
        } else {
            snprintf(payload, sizeof(payload),
                "{\"time\":%u,\"value\":%.2f,\"unit\":\"%s\",\"sensor\":\"%s\",\"calculated\":%s}",
                epoch, std::get<float>(m.value), Sensor::measurementTypeUnit(m.type), m.sensor, m.calculated ? "true" : "false");
        }

        if (mqttClient->publish(topic, payload)) {
            succeeded++;
        } else {
            failed++;
        }
    }

    mqttClient->recordPublishResult(succeeded, failed);

    if (failed > 0) {
        Serial.printf("MQTT: Published %u/%u measurements (%u failed)\r\n",
                      succeeded, succeeded + failed, failed);
    }
#endif
}

void Network::updateMqttConfig(const Config::MqttConfig& mqttConfig) {
    if (mqttClient) {
        mqttClient->setConfig(mqttConfig);
    }
}

void Network::startTask() {
    xTaskCreate(
        taskWrapper, // Task Function
        "Network", // Task Name
        18000, // Stack Size
        this, // Parameters
        1, // Priority
        &taskHandle // Task Handle
    );
}

void Network::taskWrapper(void *pvParameters) {
    Serial.println("Network: taskWrapper()");
    auto *instance = static_cast<Network *>(pvParameters);
    instance->task();
}
