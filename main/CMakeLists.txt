idf_component_register(
    SRCS "main.cpp"
         "app_registry/wifi_basic.cpp"
         "app_registry/mulit_thread_basic.cpp"
         "app_registry/http_server.cpp"
    INCLUDE_DIRS "."
    REQUIRES "mdns"
             "esp_wifi"
             "nvs_flash"
             "esp_netif"
             "esp_event"
)