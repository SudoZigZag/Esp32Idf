#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HTTP-Server";

void app_http_server(bool run_forever = true, void *arg = NULL)
{
    ESP_LOGI(TAG, "HTTP Server App - COMING SOON!");
    ESP_LOGI(TAG, "This app will serve a web page");
    
    while(run_forever) {
        ESP_LOGI(TAG, "Not implemented yet...");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}