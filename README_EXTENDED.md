# `README_EXTENDED.md` - Technical and Development Guide

This document serves as a technical companion to the main `README.md`. Its purpose is to detail the software architecture, specific hardware configurations, and library modifications required to compile and develop the Huawei SUN2000 inverter monitor using the LilyGO T-Display-S3 board.

---

## 1. Motivation and Architectural Decisions

The development of this project was driven by specific needs that dictated the final architecture of the solution:

* **Real-Time Requirement:** Standard Huawei applications update data every 5 minutes. To reactively monitor a household's consumption (e.g., turning an appliance on/off and seeing the immediate impact), a 3-second polling cycle was necessary.
* **Direct Connection to the Inverter's Access Point (AP):** Due to recent Huawei firmware updates (such as SPC155 and higher), the Modbus TCP port (502/6607) was blocked on the local home Wi-Fi network (via dongle connection). The only reliable and robust way to read data locally without complex intermediate hardware is to connect the ESP32 directly to the closed, internal Wi-Fi network broadcast by the inverter itself (SSID `SUN2000-...`).
* **Lack of Internet and Time Management:** Because the ESP32 is connected to the inverter's AP, it is physically isolated and has no internet access. Consequently, querying NTP (Network Time Protocol) servers to fetch the real time is impossible. For this reason, the clock in the bottom-left corner of the screen does not display the time of day, but rather the device's **Uptime** (Hours:Minutes:Seconds since it was powered on), calculated mathematically using the `millis()` function.

---

## 2. Hardware Requirements and Pinout

The code is written specifically for the **LilyGO T-Display-S3** board characteristics.

**Internal Pin Mapping (GPIOs):**

* `PIN_BTN_LEFT (0)`: Left physical button (used to wake from Deep Sleep and increase screen brightness).
* `PIN_BTN_RIGHT (14)`: Right physical button (used to decrease screen brightness and enter Deep Sleep).
* `PIN_BACKLIGHT (38)`: PWM control for the LCD screen brightness.
* `PIN_SCREEN_POWER (15)`: Main power supply control for the LCD (crucial for turning it off entirely during Deep Sleep).
* `PIN_BATTERY (4)`: Analog pin (ADC) connected to the voltage divider for battery reading.

---

## 3. Development Environment Setup

To compile the code without errors, the **Arduino IDE** must be configured with the following strict parameters after installing the *ESP32 by Espressif Systems* package:

* **Board:** `ESP32S3 Dev Module`
* **USB CDC On Boot:** `Enabled` (Required to ensure data output on the Serial Monitor)
* **PSRAM:** `OPI PSRAM` *(Mandatory to allocate LVGL graphic buffers)*
* **Flash Size:** `16MB (128Mb)`
* **Partition Scheme:** `16M Flash (3MB APP/9.9MB FATFS)`

---

## 4. Dependencies and Libraries

The following libraries must be installed in the Arduino IDE:

1. **`WiFi`** (Native to the ESP32 package)
2. **`TFT_eSPI`** (by Bodmer) - Handles low-level communication with the display.
3. **`lvgl`** (Light and Versatile Graphics Library) - Handles graphic rendering and UI (version 8.x is recommended).
4. **`ModbusIP_ESP8266`** (by Alexander Emelianov) - Used to establish Modbus TCP communication with the inverter.

---

## 5. In-Depth Library Configurations (Crucial)

The code will fail to compile, or the screen will output "static noise," if the libraries are not modified at their root.

### 5.1. `TFT_eSPI` (`User_Setup.h` File)

You must replace the original `User_Setup.h` file in the library with the specific file provided by LilyGO for the T-Display-S3. This defines the **ST7789** controller, the correct resolution (`170x320`), and the proper SPI pins.

### 5.2. `lvgl` (`lv_conf.h` File)

The `lv_conf.h` configuration file must be placed in the Arduino IDE `libraries` folder (next to the `lvgl` library folder). Mandatory settings:

* Enable the project's primary font by changing `#define LV_FONT_MONTSERRAT_14 0` to `1`.
* Ensure the Color Depth matches the screen's capabilities (standard `16 bit` RGB565).

---

## 6. Source Code Parameters to Configure

Before compiling, you must adjust the variables in the `sketch_apr28a.ino` file:

* **Inverter Credentials:**
```cpp
const char* ssid = "SUN2000-[YOUR_SERIAL_NUMBER]";
const char* password = "Changeme"; // Huawei's default factory password

```



```
* **Battery Calibration:**
  In the main file, the `checkBattery()` method uses static limits that may vary slightly depending on the manufacturing of the T-Display-S3 resistors. If the battery never hits 100%, you should adjust the ceilings (e.g., change `vMax = 4.2;` to `vMax = 4.05;` by observing the values returned in the Serial Monitor).

---

## 7. Communication Structure and Modbus Registers (Huawei)

The `HuaweiReader.h` file handles reading the Holding registers. The following data architecture was implemented (based on Huawei's Modbus documentation):

| Modbus Address | Data Type | Description | Code Processing |
| :--- | :--- | :--- | :--- |
| **32064** | 32-bit (INT) | Input Power (Panels) | Direct reading in Watts. |
| **32080** | 32-bit (INT) | Active Power (Inverter Output)| Direct reading in Watts. |
| **37113** | 32-bit (INT) | Grid Power (Smart Meter)| Read with sign: Positive (Exporting), Negative (Importing). |
| **32089** | 16-bit (UINT)| Inverter Status | Raw code (Standby, Grid-Tied, etc). |
| **32087** | 16-bit (INT) | Internal Temperature | Divided by 10 to convert to ºC. |
| **32114 / 32226** | 32-bit (UINT)| Daily Energy | Divided by 100 to convert to kWh. (Register 32226 serves as a fallback if 32114 returns 0). |

**Household Consumption Logic:**
Since the inverter does not provide the house's consumption directly via Modbus, the value is calculated mathematically:
`House Consumption = Active Power (32080) - Grid Power (37113)`

---

## 8. Compilation and Common Troubleshooting

* **Screen with static noise / Frozen on boot:**
  Verify if the `my_disp_flush` callback function is correctly mapped in the `setup()` (`disp_drv.flush_cb = my_disp_flush;`). If LVGL cannot push the color buffer to the screen through this bridge, the image will fail. Also, confirm the `User_Setup.h` configuration in `TFT_eSPI`.
* **Device stuck on Modbus reading (Continuous asterisks `*` in the Serial Monitor):**
  This means the ESP32 connected to the Wi-Fi, but the inverter refused TCP port 6607 or did not respond to the `Unit ID`. Check the physical distance to the inverter (the AP signal is notoriously weak) or test with an alternative port (some Huawei firmwares use port 502).
* **Screen does not turn off completely during Deep Sleep:**
  Ensure you are actively cutting power by passing `digitalWrite(PIN_SCREEN_POWER, LOW);` right before executing the `esp_deep_sleep_start()` command.

```