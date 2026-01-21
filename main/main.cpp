#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/opt.h"
#include "lwip/stats.h"
#include "wifi_passwords.h"
#include "nvs_flash.h"
#include "mdns.h"

// ========================================
// Constants and Globals
// ========================================
static const char *TAG = "WiFi";

// Event group for WiFi events
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
#define MAX_RETRY_ATTEMPTS 5




// Task 1: Hello World Loop (pinned to Core 0)
void hello_world_task(void *pvParameters)
{
    int counter = 0;
    while(1) {
        ESP_LOGI(TAG, "[Core %d] [HELLO] Message #%d", 
                 xPortGetCoreID(), counter++);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

// Task 2: Stats Dumper (pinned to Core 1)
void stats_task(void *pvParameters)
{
    while(1) {
        ESP_LOGI(TAG, "[Core %d] [STATS] Dumping task list...",
                 xPortGetCoreID());

        // Dump tasks
        TaskStatus_t *task_array;
        UBaseType_t task_count = uxTaskGetNumberOfTasks();

        task_array = (TaskStatus_t*)pvPortMalloc(task_count * sizeof(TaskStatus_t));
        if (task_array != NULL) {
            task_count = uxTaskGetSystemState(task_array, task_count, NULL);

            printf("\n--- Active Tasks ---\n");
            for (UBaseType_t i = 0; i < task_count; i++) {
                printf("  %s (priority %u)\n",
                       task_array[i].pcTaskName,
                       task_array[i].uxCurrentPriority);
            }
            vPortFree(task_array);
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

// Task 3: Number Cruncher (pinned to Core 0)
void compute_task(void *pvParameters)
{
    while(1) {
        ESP_LOGI(TAG, "[Core %d] [COMPUTE] Calculating...",
                 xPortGetCoreID());

        // Simulate heavy computation
        volatile uint32_t result = 0;
        for (int i = 0; i < 10000000; i++) {
            result += i;
        }

        ESP_LOGI(TAG, "[Core %d] [COMPUTE] Result: %lu",
                 xPortGetCoreID(), result);

        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

// Task 4: Memory Monitor (pinned to Core 1)
void memory_task(void *pvParameters)
{
    while(1) {
        ESP_LOGI(TAG, "[Core %d] [MEMORY] Free heap: %lu KB",
                 xPortGetCoreID(),
                 esp_get_free_heap_size() / 1024);

        vTaskDelay(4000 / portTICK_PERIOD_MS);
    }
}

// ========================================
// Event Handler - Called when WiFi events occur
// ========================================
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // WiFi started, try to connect
        ESP_LOGI(TAG, "WiFi started, connecting to AP...");
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Disconnected - retry if we haven't exceeded max attempts
        if (s_retry_num < MAX_RETRY_ATTEMPTS) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying connection... (attempt %d/%d)", s_retry_num, MAX_RETRY_ATTEMPTS);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect to WiFi after %d attempts", MAX_RETRY_ATTEMPTS);
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Successfully got IP address!
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ========================================
// Initialize mDNS
// ========================================
void mdns_init_service(void)
{
    // Initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    
    // Set hostname
    ESP_ERROR_CHECK(mdns_hostname_set(MDNS_HOSTNAME));
    ESP_LOGI(TAG, "mDNS hostname set to: %s.local", MDNS_HOSTNAME);
    
    // Set instance name
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32-S3 Device"));
    
    // Optional: Add services so the device can be discovered
    // Add HTTP service (we'll use this later for web server)
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
    
    ESP_LOGI(TAG, "mDNS service started");
    ESP_LOGI(TAG, "You can now access this device at: http://%s.local", MDNS_HOSTNAME);
}

// ========================================
// Initialize WiFi in Station Mode
// ========================================
void wifi_init_sta(void)
{
    // Create event group
    s_wifi_event_group = xEventGroupCreate();

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    // Configure WiFi connection
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASSWORD);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    // Set mode and configuration
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished.");
    ESP_LOGI(TAG, "Connecting to SSID: %s", WIFI_SSID);

    // Wait for connection (or failure)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    // Check result
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "✓ Connected to WiFi successfully!");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "✗ Failed to connect to WiFi");
    } else {
        ESP_LOGE(TAG, "✗ Unexpected event");
    }
}

#if 0
void BlinkLed(gpio_num_t pin_num, int timeout_ms)
{
    printf("Hello! Starting LED blink example\n");
    printf("LED Pin: %d\n", pin_num);
    
    // Configure GPIO pin as output
    gpio_reset_pin(pin_num);
    gpio_set_direction(pin_num, GPIO_MODE_OUTPUT);
    
    // Blink forever
    int level = 0;
    printf("LED: %s\n", level ? "ON" : "OFF");
    gpio_set_level(pin_num, level);
    level = !level;
    vTaskDelay(pdMS_TO_TICKS(timeout_ms));
    gpio_set_level(pin_num, level);
    printf("LED: %s\n", level ? "ON" : "OFF");
}
void RgbLEDBlinker()
{
    printf("\n=================================\n");
    printf("RGB LED Control Example\n");
    printf("=================================\n");
    
    // Configure the RGB LED strip
    led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_LED_GPIO,
        .max_leds = 1,  // Just one RGB LED on the board
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,  // WS2812 uses GRB format
        .led_model = LED_MODEL_WS2812,
        .flags = {
            .invert_out = false,
        }
    };
    
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags = {
            .with_dma = false,
        }
    };
    
    // Create LED strip object
    led_strip_handle_t led_strip;
    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    
    if (err != ESP_OK) {
        printf("Failed to create LED strip: %s\n", esp_err_to_name(err));
        return;
    }
    
    printf("RGB LED initialized on GPIO %d\n", RGB_LED_GPIO);
    printf("Starting color cycle...\n\n");
    
    // Color cycle
    int brightness = 20;  // 0-255, start low to not blind yourself!
    
    while(1) {
        // Red
        printf("RED\n");
        led_strip_set_pixel(led_strip, 0, brightness, 0, 0);
        led_strip_refresh(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        // Green
        printf("GREEN\n");
        led_strip_set_pixel(led_strip, 0, 0, brightness, 0);
        led_strip_refresh(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        // Blue
        printf("BLUE\n");
        led_strip_set_pixel(led_strip, 0, 0, 0, brightness);
        led_strip_refresh(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        // Yellow (Red + Green)
        printf("YELLOW\n");
        led_strip_set_pixel(led_strip, 0, brightness, brightness, 0);
        led_strip_refresh(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        // Cyan (Green + Blue)
        printf("CYAN\n");
        led_strip_set_pixel(led_strip, 0, 0, brightness, brightness);
        led_strip_refresh(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        // Magenta (Red + Blue)
        printf("MAGENTA\n");
        led_strip_set_pixel(led_strip, 0, brightness, 0, brightness);
        led_strip_refresh(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        // White (All colors)
        printf("WHITE\n");
        led_strip_set_pixel(led_strip, 0, brightness, brightness, brightness);
        led_strip_refresh(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        // Off
        printf("OFF\n");
        led_strip_clear(led_strip);
        led_strip_refresh(led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
#endif
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
#if 0
    for(const auto& pin : TEST_PINS) {
        BlinkLed(pin, 2000);
    }
    // RgbLEDBlinker();
#endif
    // Initialize NVS (needed for WiFi)
     esp_err_t ret = nvs_flash_init();
     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
          ESP_ERROR_CHECK(nvs_flash_erase());
         ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

     // Connect to WiFi
     wifi_init_sta();

    // Initialize mDNS (only after WiFi is connected!)
    mdns_init_service();

    // Main loop - you're connected now!
    printf("\n");
    printf("=================================\n");
    printf("WiFi is connected!\n");
    printf("You can now:\n");
    printf("  - Make HTTP requests\n");
    printf("  - Run a web server\n");
    printf("  - Connect to MQTT\n");
    printf("  - Do anything network-related!\n");
    printf("=================================\n");
    printf("\n");
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║     Multi-threaded Dual-Core Application      ║\n");
    printf("╚════════════════════════════════════════════════╝\n\n");
    
    ESP_LOGI(TAG, "Creating tasks on both cores...");
    
    // Create Task 1 on Core 0
    xTaskCreatePinnedToCore(
        hello_world_task,      // Function
        "HelloWorld",          // Name (shows in task dump!)
        4096,                  // Stack size
        NULL,                  // Parameters
        5,                     // Priority
        NULL,                  // Task handle
        0);                    // Core 0
    
    // Create Task 2 on Core 1
    xTaskCreatePinnedToCore(
        stats_task,
        "StatsDumper",
        8192,
        NULL,
        5,
        NULL,
        1);                    // Core 1
    
    // Create Task 3 on Core 0
    xTaskCreatePinnedToCore(
        compute_task,
        "NumberCruncher",
        4096,
        NULL,
        5,
        NULL,
        0);                    // Core 0
    
    // Create Task 4 on Core 1
    xTaskCreatePinnedToCore(
        memory_task,
        "MemoryMonitor",
        4096,
        NULL,
        5,
        NULL,
        1);                    // Core 1
    
    ESP_LOGI(TAG, "All tasks created! App exiting main...");

}