#ifndef UI_H
#define UI_H

#include <lvgl.h>
#include <WiFi.h>
#include <time.h> 
#include "models.h"

// --- TEXT CONFIGURATION VARIABLES ---
const char* TXT_HEADER_EXCESS  = "EXPORT";
const char* TXT_HEADER_DEFICIT = "IMPORT";
const char* TXT_HEADER_NIGHT   = "NIGHT";
const char* TXT_UNIT           = "W";

// Icon size definition (256 = 100%)
#define ICON_ZOOM 256 

// Image declarations - Original names kept to avoid breaking C file mappings
LV_IMG_DECLARE(painelPreto);  
LV_IMG_DECLARE(painelBranco); 
LV_IMG_DECLARE(casaPreta);
LV_IMG_DECLARE(casaBranca);
LV_IMG_DECLARE(torrePreta);
LV_IMG_DECLARE(torreBranca);

class DisplayUI {
private:
    lv_obj_t * mainScreen;     // Main UI screen object
    lv_obj_t * header;         // Header container
    lv_obj_t * headerLabel;    // Header text label
    
    lv_obj_t * labelProd;      // Label for solar production value
    lv_obj_t * labelCons;      // Label for house consumption value
    lv_obj_t * labelGridFlow;  // Label for grid flow value
    
    lv_obj_t * imgPanel;       // Image object for the solar panel
    lv_obj_t * imgHouse;       // Image object for the house
    lv_obj_t * imgGrid;        // Image object for the grid tower
    
    // Footer elements
    lv_obj_t * labelTime;      // Label for current time
    lv_obj_t * labelBattery;   // Label for battery percentage

    // Flow Arrows
    lv_obj_t * arrowP_H;       // Arrow: Panel -> House
    lv_obj_t * arrowG_H;       // Arrow: Grid -> House
    lv_obj_t * arrowH_G;       // Arrow: House -> Grid

    // String buffers for text formatting
    char strProd[32];
    char strCons[32];
    char strFlow[32];
    char strTime[16];
    char strBatt[16];

    // Arrow coordinates moved 1px to the right (Center X axis = 34)
    lv_point_t ptsP_H[5] = { {34, 105}, {34, 135}, {29, 125}, {34, 135}, {39, 125} };  
    lv_point_t ptsH_G[5] = { {34, 195}, {34, 225}, {29, 215}, {34, 225}, {39, 215} }; 
    lv_point_t ptsG_H[5] = { {34, 225}, {34, 195}, {29, 205}, {34, 195}, {39, 205} }; 

public:
    DisplayUI() {}             // Default constructor

    void initialize() {
        // Create the main screen object
        mainScreen = lv_obj_create(NULL);
        // Disable scrollbars on the main screen
        lv_obj_set_scrollbar_mode(mainScreen, LV_SCROLLBAR_MODE_OFF);
        // Set the default background color to white
        lv_obj_set_style_bg_color(mainScreen, lv_color_white(), 0);
        // Load the main screen
        lv_scr_load(mainScreen);

        // Style setup for numeric values
        static lv_style_t style_val;
        lv_style_init(&style_val);
        // Set font size to 24 for numbers
        lv_style_set_text_font(&style_val, &lv_font_montserrat_24);

        // Header configuration
        header = lv_obj_create(mainScreen);
        // Set header size (width, height)
        lv_obj_set_size(header, 170, 35);
        // Align header to top middle
        lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
        // Remove border radius
        lv_obj_set_style_radius(header, 0, 0);
        // Remove border width
        lv_obj_set_style_border_width(header, 0, 0);
        // Disable scrollbar for header
        lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);
        
        // Header text label setup
        headerLabel = lv_label_create(header);
        // Set header font size
        lv_obj_set_style_text_font(headerLabel, &lv_font_montserrat_24, 0);
        // Set header text color to white
        lv_obj_set_style_text_color(headerLabel, lv_color_white(), 0);
        // Center the label inside the header container
        lv_obj_center(headerLabel);

        // 1. SOLAR PANEL (Top position)
        imgPanel = lv_img_create(mainScreen);
        lv_img_set_zoom(imgPanel, ICON_ZOOM); // Set zoom level
        lv_obj_set_pos(imgPanel, 15, 50);     // Set position (X, Y)

        // Production label setup
        labelProd = lv_label_create(mainScreen);
        lv_obj_add_style(labelProd, &style_val, 0); // Apply numeric style
        lv_obj_set_style_text_align(labelProd, LV_TEXT_ALIGN_RIGHT, 0); // Right-align text
        lv_obj_align(labelProd, LV_ALIGN_TOP_RIGHT, -15, 60); // Position label

        // 2. HOUSE (Middle position)
        imgHouse = lv_img_create(mainScreen);
        lv_img_set_zoom(imgHouse, ICON_ZOOM);
        lv_obj_set_pos(imgHouse, 15, 140);

        // Consumption label setup
        labelCons = lv_label_create(mainScreen);
        lv_obj_add_style(labelCons, &style_val, 0);
        lv_obj_set_style_text_align(labelCons, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(labelCons, LV_ALIGN_TOP_RIGHT, -15, 150);

        // 3. GRID / TOWER (Bottom position)
        imgGrid = lv_img_create(mainScreen);
        lv_img_set_zoom(imgGrid, ICON_ZOOM);
        lv_obj_set_pos(imgGrid, 15, 230);

        // Grid flow label setup
        labelGridFlow = lv_label_create(mainScreen);
        lv_obj_add_style(labelGridFlow, &style_val, 0);
        lv_obj_set_style_text_align(labelGridFlow, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(labelGridFlow, LV_ALIGN_TOP_RIGHT, -15, 240);

        // ARROWS SETUP
        // Panel to House arrow
        arrowP_H = lv_line_create(mainScreen);
        lv_line_set_points(arrowP_H, ptsP_H, 5); // Apply coordinates
        lv_obj_set_style_line_width(arrowP_H, 4, 0); // Set line thickness
        lv_obj_set_style_line_rounded(arrowP_H, true, 0); // Round line edges

        // Grid to House arrow
        arrowG_H = lv_line_create(mainScreen);
        lv_line_set_points(arrowG_H, ptsG_H, 5);
        lv_obj_set_style_line_width(arrowG_H, 4, 0);
        lv_obj_set_style_line_rounded(arrowG_H, true, 0);

        // House to Grid arrow
        arrowH_G = lv_line_create(mainScreen);
        lv_line_set_points(arrowH_G, ptsH_G, 5);
        lv_obj_set_style_line_width(arrowH_G, 4, 0);
        lv_obj_set_style_line_rounded(arrowH_G, true, 0);

        // Footer container setup
        lv_obj_t * footer = lv_obj_create(mainScreen);
        lv_obj_set_size(footer, 170, 25);
        lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(footer, lv_color_hex(0xEEEEEE), 0); // Light gray background
        lv_obj_set_style_radius(footer, 0, 0);
        lv_obj_set_style_border_width(footer, 0, 0);
        lv_obj_set_scrollbar_mode(footer, LV_SCROLLBAR_MODE_OFF);
        
        // Time label setup
        labelTime = lv_label_create(footer);
        lv_obj_set_style_text_font(labelTime, &lv_font_montserrat_14, 0); 
        lv_obj_align(labelTime, LV_ALIGN_LEFT_MID, 5, 0); // Align to the left of footer

        // Battery label setup
        labelBattery = lv_label_create(footer);
        lv_obj_set_style_text_font(labelBattery, &lv_font_montserrat_14, 0); 
        lv_obj_align(labelBattery, LV_ALIGN_RIGHT_MID, -5, 0); // Align to the right of footer

        // Initialize with default zeroed data
        InverterData initData = {0, 0, 0, 0, 0, 0, 0, false, 0, false};
        update(initData);
    }

    void update(InverterData data) {
        // Determine if it is night time (Active power below 10W)
        bool isNight = (data.activePower < 10); 
        // Get absolute value of grid power for display
        int32_t gridPowerAbs = abs(data.gridPower);

        // Format strings using the TXT_UNIT variable
        sprintf(strProd, "%d %s", data.activePower, TXT_UNIT);
        sprintf(strCons, "%d %s", data.houseConsumption, TXT_UNIT);
        sprintf(strFlow, "%d %s", gridPowerAbs, TXT_UNIT);

        // Hide all arrows by default
        lv_obj_add_flag(arrowP_H, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(arrowG_H, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(arrowH_G, LV_OBJ_FLAG_HIDDEN);

        if (isNight) {
            // NIGHT THEME
            // Set screen and header backgrounds to black
            lv_obj_set_style_bg_color(mainScreen, lv_color_black(), 0);
            lv_obj_set_style_bg_color(header, lv_color_black(), 0);
            // Update header text
            lv_label_set_text(headerLabel, TXT_HEADER_NIGHT);
            
            // Swap icons to white versions for contrast
            lv_img_set_src(imgPanel, &painelBranco);
            lv_img_set_src(imgHouse, &casaBranca);
            lv_img_set_src(imgGrid, &torreBranca);
            
            // Apply mockup colors for text
            lv_obj_set_style_text_color(labelProd, lv_color_hex(0x666666), 0);     // Gray
            lv_obj_set_style_text_color(labelCons, lv_color_hex(0xFFB300), 0);     // Orange/Yellow
            lv_obj_set_style_text_color(labelGridFlow, lv_color_hex(0xFFB300), 0); // Orange/Yellow
            
            // Display active connections during the night
            lv_obj_clear_flag(arrowP_H, LV_OBJ_FLAG_HIDDEN); // Show Panel->House arrow
            lv_obj_set_style_line_color(arrowP_H, lv_color_hex(0x666666), 0); // Set to inactive gray
            
            lv_obj_clear_flag(arrowG_H, LV_OBJ_FLAG_HIDDEN); // Show Grid->House arrow (importing)
            lv_obj_set_style_line_color(arrowG_H, lv_color_white(), 0); // Set to white for grid import
            
        } else {
            // DAY THEME
            // Set screen background to white
            lv_obj_set_style_bg_color(mainScreen, lv_color_white(), 0);
            // Swap icons to black versions
            lv_img_set_src(imgPanel, &painelPreto);
            lv_img_set_src(imgHouse, &casaPreta);
            lv_img_set_src(imgGrid, &torrePreta);
            
            // Solar panel is producing, show arrow
            lv_obj_clear_flag(arrowP_H, LV_OBJ_FLAG_HIDDEN);

            if (data.gridPower < 0) { 
                // DEFICIT (Importing from grid)
                // Set header to red
                lv_obj_set_style_bg_color(header, lv_color_hex(0xCC0000), 0); 
                lv_label_set_text(headerLabel, TXT_HEADER_DEFICIT); 
                
                // Colorize text labels
                lv_obj_set_style_text_color(labelProd, lv_color_black(), 0);
                lv_obj_set_style_text_color(labelCons, lv_color_hex(0xCC0000), 0);
                lv_obj_set_style_text_color(labelGridFlow, lv_color_hex(0xCC0000), 0);
                
                // Colorize arrows to red
                lv_obj_set_style_line_color(arrowP_H, lv_color_hex(0xCC0000), 0);
                lv_obj_clear_flag(arrowG_H, LV_OBJ_FLAG_HIDDEN); // Show Grid->House arrow
                lv_obj_set_style_line_color(arrowG_H, lv_color_hex(0xCC0000), 0);
                
            } else { 
                // EXCESS (Exporting to grid)
                // Set header to green
                lv_obj_set_style_bg_color(header, lv_color_hex(0x008000), 0); 
                lv_label_set_text(headerLabel, TXT_HEADER_EXCESS);
                
                // Colorize text labels
                lv_obj_set_style_text_color(labelProd, lv_color_hex(0x008000), 0);
                lv_obj_set_style_text_color(labelCons, lv_color_black(), 0);
                lv_obj_set_style_text_color(labelGridFlow, lv_color_hex(0x008000), 0);
                
                // Colorize arrows to green
                lv_obj_set_style_line_color(arrowP_H, lv_color_hex(0x008000), 0);
                lv_obj_clear_flag(arrowH_G, LV_OBJ_FLAG_HIDDEN); // Show House->Grid arrow
                lv_obj_set_style_line_color(arrowH_G, lv_color_hex(0x008000), 0);
            }
        }

        // Update the actual text on the screen elements
        lv_label_set_text(labelProd, strProd);
        lv_label_set_text(labelCons, strCons);
        lv_label_set_text(labelGridFlow, strFlow);

        // Clock and Footer updates
        unsigned long sec = millis() / 1000;      // Get seconds since boot
        unsigned long h = (sec / 3600) % 24;      // Calculate hours
        unsigned long m = (sec / 60) % 60;        // Calculate minutes
        unsigned long s = sec % 60;               // Calculate seconds
        // Format time string
        sprintf(strTime, "%02lu:%02lu:%02lu", h, m, s);
        // Apply time string to label
        lv_label_set_text(labelTime, strTime);

        // Battery formatting
        if (data.isCharging) {
            if (data.batteryPercent < 100) {
                // Orange if charging but not full
                lv_obj_set_style_text_color(labelBattery, lv_color_hex(0xFFA500), 0); 
            } else {
                // Green if fully charged
                lv_obj_set_style_text_color(labelBattery, lv_color_hex(0x008000), 0); 
            }
        } else {
            // Red if not charging (running on battery)
            lv_obj_set_style_text_color(labelBattery, lv_color_hex(0xFF0000), 0); 
        }

        // Format battery percentage string
        sprintf(strBatt, "%d%%", data.batteryPercent);
        // Apply battery string to label
        lv_label_set_text(labelBattery, strBatt);
    }
};
#endif