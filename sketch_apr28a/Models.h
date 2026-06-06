#ifndef MODELS_H
#define MODELS_H

// --- DEBUG CONFIGURATION ---
// Change to 0 to disable all Serial output and improve performance
#define DEBUG_MODE 0

#if DEBUG_MODE
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINT(x)    Serial.print(x)
#else
  #define DEBUG_PRINTF(...)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINT(x)
#endif

// Structure to hold the inverter data
struct InverterData {
    // Raw and processed values from the inverter
    uint32_t inputPower;       // Register 32064 (W) - Power from solar panels
    int32_t  activePower;      // Register 32080 (W) - Active power generated
    int32_t  gridPower;        // Register 37113 (W) - Grid power (can be negative if importing)
    uint16_t statusRaw;        // Register 32089 - Raw status code of the inverter
    float    temperature;      // Register 32087 (ºC) - Internal temperature
    float    dailyEnergy;      // Register 32114 or 32226 (kWh) - Total energy produced today
    int32_t  houseConsumption; // Calculated consumption of the house
    
    bool isConnected;          // Flag to check if the connection is active

    int batteryPercent;        // Estimated battery percentage of the display device
    bool isCharging;           // Flag to indicate if the display device is charging
};

// 1. The new dedicated model for battery data
struct BatteryStatus {
    int percent;               // Calculated battery percentage (0-100)
    bool isCharging;           // True if USB cable is providing enough voltage
};

#endif