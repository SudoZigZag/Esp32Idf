#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void)
{
    #ifdef CONFIG_IDF_TARGET_ESP32
        printf("Running on ESP32 (dual-core Xtensa)\n");
    #elif CONFIG_IDF_TARGET_ESP32S3
        printf("Running on ESP32-S3 (dual-core Xtensa with USB)\n");
    #elif CONFIG_IDF_TARGET_ESP32C3
        printf("Running on ESP32-C3 (single-core RISC-V)\n");
    #else
        printf("Unknown target\n");
    #endif
    
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}