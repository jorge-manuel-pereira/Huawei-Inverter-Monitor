#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <lvgl.h>

// Declaration of the new icons (C arrays) - Kept original names as requested
LV_IMG_DECLARE(wifiCinza);
LV_IMG_DECLARE(wifiVerde);
LV_IMG_DECLARE(roldanaCinza);
LV_IMG_DECLARE(roldanaVerde);

class SplashScreen {
private:
    lv_obj_t * screen;      // Pointer to the splash screen object
    lv_obj_t * imgWifi;     // Pointer to the Wi-Fi icon image object
    lv_obj_t * imgGear;     // Pointer to the gear (inverter) icon image object

public:
    SplashScreen() {}       // Default constructor

    void initialize() {
        // Create a dedicated screen for the Splash view
        screen = lv_obj_create(NULL);
        // Set the background color of the screen to black
        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
        // Load the created screen
        lv_scr_load(screen);

        // Wi-Fi Icon (Top Center)
        imgWifi = lv_img_create(screen);
        // Set the source of the image to the gray Wi-Fi icon
        lv_img_set_src(imgWifi, &wifiCinza);
        // Align the Wi-Fi icon to the center, offset by -50 pixels on the Y axis
        lv_obj_align(imgWifi, LV_ALIGN_CENTER, 0, -50);

        // Gear Icon (Bottom Center)
        imgGear = lv_img_create(screen);
        // Set the source of the image to the gray gear icon
        lv_img_set_src(imgGear, &roldanaCinza);
        // Align the gear icon to the center, offset by 50 pixels on the Y axis
        lv_obj_align(imgGear, LV_ALIGN_CENTER, 0, 50);
    }

    void setWifiReady() {
        // Change the Wi-Fi image source to the green icon
        lv_img_set_src(imgWifi, &wifiVerde);
    }

    void setInverterReady() {
        // Change the gear image source to the green icon
        lv_img_set_src(imgGear, &roldanaVerde);
    }
    
    void clear() {
        // Delete the splash screen object from memory when done
        lv_obj_del(screen);
    }
};

#endif