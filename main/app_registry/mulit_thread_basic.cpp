#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "MultiThread";

void task_hello(void *pvParameters)
{
    int counter = 0;
    while(1) {
        ESP_LOGI(TAG, "[Core %d] Hello #%d", xPortGetCoreID(), counter++);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void task_stats(void *pvParameters)
{
    while(1) {
        ESP_LOGI(TAG, "[Core %d] Free heap: %lu KB", 
                 xPortGetCoreID(),
                 esp_get_free_heap_size() / 1024);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_multi_thread(void)
{
    ESP_LOGI(TAG, "Multi-threading App Starting...");
    
    // Create tasks on both cores
    xTaskCreatePinnedToCore(task_hello, "Hello", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_stats, "Stats", 4096, NULL, 5, NULL, 1);
    
    ESP_LOGI(TAG, "Tasks created on both cores!");
    
    // Keep running
    while(1) {
        vTaskDelay(portMAX_DELAY);
    }
}