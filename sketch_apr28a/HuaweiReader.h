#ifndef HUAWEI_READER_H
#define HUAWEI_READER_H

#include <WiFi.h>
#include <ModbusIP_ESP8266.h>
#include "models.h"

class HuaweiReader {
  private:
    ModbusIP _mb;              // Modbus client instance
    IPAddress _inverterIP;     // IP address of the inverter
    uint16_t _unitId;          // Modbus Unit ID
    uint16_t _port;            // Communication port

    // Helper function to read 32 bits (2 registers) - Equivalent to C# Read32
    uint32_t read32(uint16_t reg) {
        uint16_t res[2] = {0, 0}; // Array to hold the 2 registers
        // Execute Modbus read operation
        uint16_t transId = _mb.readHreg(_inverterIP, reg, res, 2, nullptr, _unitId);
        // If transaction fails to start, return 0
        if (transId == 0) return 0;
        
        unsigned long start = millis();
        // Wait for transaction to complete
        while (_mb.isTransaction(transId)) {
            _mb.task(); // Process Modbus tasks
            // Timeout after 1000ms
            if (millis() - start > 1000) return 0;
            yield();    // Yield to avoid watchdog resets
        }
        // Combine the two 16-bit registers into one 32-bit value
        return (res[0] << 16) | res[1];
    }

    // Helper function to read 16 bits (1 register) - Equivalent to C# Read16
    uint16_t read16(uint16_t reg) {
        uint16_t res = 0; // Variable to hold the result
        // Execute Modbus read operation
        uint16_t transId = _mb.readHreg(_inverterIP, reg, &res, 1, nullptr, _unitId);
        // If transaction fails to start, return 0
        if (transId == 0) return 0;

        unsigned long start = millis();
        // Wait for transaction to complete
        while (_mb.isTransaction(transId)) {
            _mb.task(); // Process Modbus tasks
            // Timeout after 1000ms
            if (millis() - start > 1000) return 0;
            yield();    // Yield to avoid watchdog resets
        }
        return res;
    }

    // Logic to obtain the status as text (GetInverterStatusText)
    const char* getStatusText(uint16_t code) {
        switch(code) {
            case 0: return "Standby";
            case 512: return "Operating";
            case 768: return "Fault";
            case 2: return "Starting";
            default: return "Unknown";
        }
    }

  public:
    // Constructor updated to accept 3 arguments
    HuaweiReader(IPAddress ip, uint16_t unitId = 1, uint16_t port = 502) 
      : _inverterIP(ip), _unitId(unitId), _port(port) {}

    // Initializes the Modbus client
    void initialize() { _mb.client(); }

    // Reads all necessary data and updates the InverterData structure
    bool readAll(InverterData &data, bool readOptionalParams) {
        
        DEBUG_PRINTLN("\n--- Reading Huawei Inverter ---");
        
        // Connect to the inverter if not already connected
        if (!_mb.isConnected(_inverterIP)) {
            if (!_mb.connect(_inverterIP, _port)) return false;
        }

        if (readOptionalParams) {
            // Inverter Status (Register 32089)
            data.statusRaw = read16(32089);
            delay(200);

            //Solar Panels Power (Register 32064)
            data.inputPower = read32(32064);
            delay(200);  

            //Temperature (Register 32087)
            int16_t rawTemp = (int16_t)read16(32087);
            // Divide by 10 to get the correct decimal value
            data.temperature = rawTemp / 10.0f;
            delay(200);

            //Daily Energy (Try 32114, if 0 try 32226)
            uint32_t rawDaily = read32(32114);
            if (rawDaily == 0) 
                rawDaily = read32(32226);

            //Divide by 100 to get kWh
            data.dailyEnergy = rawDaily / 100.0f; 
        }   

        //Active AC Power (Register 32080)
        data.activePower = (int32_t)read32(32080);
        delay(200);

        //Grid Power (Register 37113) - Important: use int32_t for negative values
        data.gridPower = (int32_t)read32(37113);
        delay(200);

        //House Consumption Calculation (C# Formula)
        data.houseConsumption = data.activePower - data.gridPower;

        //Output results to serial monitor using debug macros
        printToSerial(data, readOptionalParams);

        return true;
    }

    // Prints the gathered data to the serial monitor for debugging
    void printToSerial(InverterData &d, bool readOptionalParams) {
        // Determine grid direction as a string
        const char* gridDir = (d.gridPower > 0) ? "Exporting" : (d.gridPower < 0 ? "Importing" : "Neutral");
        
        DEBUG_PRINTLN("-----------------------------------");
        if (readOptionalParams)
            DEBUG_PRINTF("Input Power       : %u W\n", d.inputPower);
        DEBUG_PRINTF("Active Power      : %d W\n", d.activePower);
        DEBUG_PRINTF("Grid Power        : %d W  -> (%s)\n", d.gridPower, gridDir);
        DEBUG_PRINTF("House Consumption : %d W\n", d.houseConsumption);
        if (readOptionalParams){
            DEBUG_PRINTF("Status            : %u - %s\n", d.statusRaw, getStatusText(d.statusRaw));
            DEBUG_PRINTLN("-----------------------------------");
            DEBUG_PRINTF("Daily Energy      : %.2f kWh\n", d.dailyEnergy);
            DEBUG_PRINTF("Temperature       : %.1f ºC\n", d.temperature);
        }
        DEBUG_PRINTLN("-----------------------------------\n");
    }
};

#endif