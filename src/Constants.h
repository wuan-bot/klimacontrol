#ifndef KLIMACONTROL_CONSTANTS_H
#define KLIMACONTROL_CONSTANTS_H

/**
 * Project-wide constants
 */
namespace Constants {
    // Project name
    constexpr const char* PROJECT_NAME = "klima";
    
    // mDNS/hostname prefix
    constexpr const char* HOSTNAME_PREFIX = "klima-";
    
    // AP mode SSID prefix
    constexpr const char* AP_SSID_PREFIX = "Klima ";
    
    // mDNS instance name prefix
    constexpr const char* INSTANCE_NAME_PREFIX = "Klima ";
    
    // NVS namespace
    constexpr const char* NVS_NAMESPACE = "klima";
    
    // GitHub repository name
    constexpr const char* GITHUB_REPO = "klima";
    
    // Default WiFi TX power (wifi_power_t raw value, default 13 dBm)
    constexpr uint8_t DEFAULT_WIFI_POWER = 68;
};

#endif //KLIMACONTROL_CONSTANTS_H