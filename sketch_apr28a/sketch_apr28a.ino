#include <WiFi.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <driver/rtc_io.h>
#include "models.h"
#include "huaweiReader.h"
#include "ui.h"
#include "splashscreen.h"

// --- NETWORK SETTINGS ---
const char* ssid = "SUN2000-TA2270347731";
const char* password = "Changeme";
// Inverter IP configuration
IPAddress inverterIP(192, 168, 200, 1); 
const long port = 6607;

// --- APP SETTINGS ---
bool readOptionaInverterParameters = false;
int currentBrightness = 45; // Default screen brightness level

// --- DEVICE SETTINGS ---
const int PIN_BTN_LEFT = 0;   
const int PIN_BTN_RIGHT = 14;   
const int PIN_BACKLIGHT = 38;
const int PIN_BATTERY = 4;
const int PIN_SCREEN_POWER = 15;

// --- DEVICE DISPLAY SETTINGS ---
static const uint16_t screenWidth  = 170;
static const uint16_t screenHeight = 320;
// Buffer needed for LVGL rendering
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * 10 ];

// Display driver instance
TFT_eSPI tft = TFT_eSPI();

// UI and Data classes pointers
SplashScreen* splash;
DisplayUI* ui;
// Initialize the reader with IP and Port
HuaweiReader reader(inverterIP, 0, port);
// Object to hold our fetched data
InverterData inverterData;

// Timer variables
unsigned long lastCheck = 0;
unsigned long lastTickMillis = 0;

// Button state memory
bool lastStateLeft = HIGH;
bool lastStateRight = HIGH;

// Function: Deep sleep to save battery/energy
void enterDeepSleep() {
    DEBUG_PRINTLN("Preparing to enter Deep Sleep...");
    
    // Turn off screen backlight
    analogWrite(PIN_BACKLIGHT, 0);
    
    // Send sleep command to the TFT display
    tft.writecommand(0x10); 
    delay(120);

    // Cut power to the screen completely
    digitalWrite(PIN_SCREEN_POWER, LOW); 

    // Enable internal pullup for the wakeup button
    rtc_gpio_pullup_en(GPIO_NUM_0);
    
    // Configure left button (PIN_BTN_LEFT) as the wakeup source
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BTN_LEFT, 0);
    
    DEBUG_PRINTLN("Entering sleep mode");
    // Halt execution and enter sleep
    esp_deep_sleep_start();
}

// Function: Read and calculate battery status
BatteryStatus checkBattery() {
    BatteryStatus status;
    
    // Read voltage with higher precision (float) and convert millivolts to volts
    float voltage = (analogReadMilliVolts(PIN_BATTERY) * 2.0) / 1000.0;
    
    // Detect if USB cable is plugged in based on voltage threshold
    status.isCharging = (voltage >= 4.25);

    // Minimum operating voltage
    float vMin = 3.3;
    float vMax;
    
    if (status.isCharging) {
        vMax = 4.75; // Compensated ceiling limit for the specific charging cable
    } else {
        vMax = 4.05; // Standard battery ceiling limit
    }

    // Pure percentage calculation (before safety limits)
    float rawPercentage = ((voltage - vMin) / (vMax - vMin)) * 100;
    
    // Apply UI safety boundaries (0 to 100)
    status.percent = (int)rawPercentage;
    if (status.percent > 100) status.percent = 100;
    if (status.percent < 0) status.percent = 0;
    
    // --- DETAILED SERIAL MONITOR PRINTOUT ---
    DEBUG_PRINTLN("\n=== BATTERY STATUS ===");
    DEBUG_PRINTF("1. Actual Voltage Read: %.3f V\n", voltage);
    DEBUG_PRINTF("2. USB Cable Detected : %s\n", status.isCharging ? "YES" : "NO");
    DEBUG_PRINTF("3. Scale Applied      : %.2fV (Min) -> %.2fV (Max)\n", vMin, vMax);
    DEBUG_PRINTF("4. Raw Percentage     : %.1f %%\n", rawPercentage);
    DEBUG_PRINTF("5. Final UI Value     : %d %%\n", status.percent);
    DEBUG_PRINTLN("=========================");
    
    return status;
}

// Function: Callback to flush the rendered image to the display
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1); // Calculate width
    uint32_t h = (area->y2 - area->y1 + 1); // Calculate height
    
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h); // Set area to draw
    tft.pushColors((uint16_t *)&color_p->full, w * h, true); // Push color array
    tft.endWrite();
    
    // Tell LVGL that flushing is done
    lv_disp_flush_ready(disp_drv);
}

void setup() {
    // Start serial communication at 115200 baud rate
    Serial.begin(115200);
    
    // Screen power pin setup
    pinMode(PIN_SCREEN_POWER, OUTPUT);
    digitalWrite(PIN_SCREEN_POWER, HIGH); // Turn on screen power
    
    // Check if we just woke up from deep sleep
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        DEBUG_PRINTLN("Waking up from deep sleep");
    }

    // Button and Backlight pins setup
    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
    pinMode(PIN_BACKLIGHT, OUTPUT);
    
    // Initialize TFT
    tft.init();
    tft.setRotation(0); // Set portrait orientation

    // Apply default brightness
    analogWrite(PIN_BACKLIGHT, currentBrightness); 

    // Initialize LVGL library
    lv_init();
    // Initialize the drawing buffer
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    // Initialize the display driver for LVGL
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush; // Link our flush callback
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Create and initialize the Splash Screen
    splash = new SplashScreen();
    splash->initialize();
    lv_timer_handler(); // Run LVGL tasks

    DEBUG_PRINTLN("Starting Wi-Fi connection...");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    // Try to connect up to 20 times (10 seconds)
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        lv_tick_inc(500); // Inform LVGL of time passed
        lv_timer_handler(); // Update UI
        attempts++;
    }

    // If connected successfully
    if (WiFi.status() == WL_CONNECTED) {
        splash->setWifiReady(); // Update splash icon
        unsigned long wifiWait = millis();
        // Wait 500ms to show the user the success icon
        while (millis() - wifiWait < 500) {
            lv_tick_inc(10);
            lv_timer_handler();
            delay(10);
        }
    }

    // Initialize the Modbus reader
    reader.initialize();
    // Attempt first reading
    if (reader.readAll(inverterData, readOptionaInverterParameters)) {
        splash->setInverterReady(); // Update splash icon
        unsigned long inverterWait = millis();
        // Wait 1000ms to show the user the success icon
        while (millis() - inverterWait < 1000) {
            lv_tick_inc(10);
            lv_timer_handler();
            delay(10);
        }
    }

    // Create and initialize the Main UI
    ui = new DisplayUI(); 
    ui->initialize(); 
    
    // Clean up splash screen resources
    splash->clear(); 
    delete splash;
}

void loop() {
    // Keep LVGL timing updated
    lv_tick_inc(millis() - lastTickMillis);
    lastTickMillis = millis();
    // Handle UI tasks
    lv_timer_handler();
    
    // --- READ CURRENT BUTTON STATES ---
    bool stateLeft = digitalRead(PIN_BTN_LEFT);
    bool stateRight = digitalRead(PIN_BTN_RIGHT);
    
    // --- LEFT BUTTON CONTROL (INCREASE BRIGHTNESS) ---
    // Act only if pressed (LOW) AND previously unpressed (lastState == HIGH)
    if (stateLeft == LOW && lastStateLeft == HIGH) {
        currentBrightness += 15;
        // Cap maximum brightness at 255
        if (currentBrightness > 255) currentBrightness = 255;
        
        analogWrite(PIN_BACKLIGHT, currentBrightness);
        DEBUG_PRINTF("LEFT Click! Brightness: %d\n", currentBrightness);
        
        // Mechanical debounce
        delay(50);
    }
    // Update memory state
    lastStateLeft = stateLeft;

    // --- RIGHT BUTTON CONTROL (DECREASE BRIGHTNESS / DEEP SLEEP) ---
    if (stateRight == LOW && lastStateRight == HIGH) {
        currentBrightness -= 15;
        
        // If brightness drops to 0 or below, enter sleep mode
        if (currentBrightness <= 0) {
            currentBrightness = 0;
            analogWrite(PIN_BACKLIGHT, 0);
            enterDeepSleep(); 
        } else {
            analogWrite(PIN_BACKLIGHT, currentBrightness);
            DEBUG_PRINTF("RIGHT Click! Brightness: %d\n", currentBrightness);
        }
        
        // Mechanical debounce
        delay(50);
    }
    // Update memory state
    lastStateRight = stateRight;

    // --- DATA REFRESH CYCLE (Every 5 seconds) ---
    if (millis() - lastCheck > 4000) {
        lastCheck = millis();
        
        // Check battery status
        BatteryStatus statbat = checkBattery();
        inverterData.batteryPercent = statbat.percent;
        inverterData.isCharging = statbat.isCharging;
        
        // Read inverter data and update UI if successful
        if (reader.readAll(inverterData, readOptionaInverterParameters)) {
            ui->update(inverterData);
        }
    }
    
    // Small delay to prevent watchdog issues
    delay(5);
}