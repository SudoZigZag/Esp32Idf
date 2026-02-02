#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "app_registry.h"

extern "C" void app_main(void)
{
    // Common initialization (all apps need this)
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║        ESP32-S3 Modular Application           ║\n");
    printf("╚════════════════════════════════════════════════╝\n\n");
    
    // Initialize NVS (most apps need this)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    
    // Show available apps
    printf("Available applications:\n");
    for (int i = 0; i < app_count; i++) {
        printf("  [%d] %-15s - %s %s\n", 
               i, 
               app_registry[i].name, 
               app_registry[i].description,
               (i == ACTIVE_APP) ? "← ACTIVE" : "");
    }
    printf("\n");
    
    // Validate and run active app
    if (ACTIVE_APP >= 0 && ACTIVE_APP < app_count) {
        printf("Starting: %s\n", app_registry[ACTIVE_APP].name);
        printf("════════════════════════════════════════════════\n\n");
        
        // Call the registered app function
        app_registry[ACTIVE_APP].function();
    } else {
        printf("ERROR: Invalid ACTIVE_APP index %d\n", ACTIVE_APP);
        printf("Valid range: 0-%d\n", app_count - 1);
    }
    
    // This line should never be reached (apps run forever)
    printf("WARNING: App returned unexpectedly!\n");
}