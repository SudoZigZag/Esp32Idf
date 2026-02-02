#ifndef APP_REGISTRY_H
#define APP_REGISTRY_H

// Function signature for all apps
typedef void (*app_function_t)(void);

// Registry structure
struct AppEntry {
    const char* name;
    const char* description;
    app_function_t function;
};

// Declare all your apps here
void app_wifi_basic(void);
void app_multi_thread(void);
void app_http_server(void);
// Add more as you create them...

// Registry table - add your apps here
static const AppEntry app_registry[] = {
    {"wifi_basic",      "WiFi + mDNS + Stats",           app_wifi_basic},
    {"multi_thread",    "Multi-core threading demo",     app_multi_thread},
    {"http_server",     "Simple HTTP web server",        app_http_server},
    // Add more entries as you create new apps...
};

static const int app_count = sizeof(app_registry) / sizeof(AppEntry);

// ========================================
// CHANGE THIS LINE TO SWITCH APPS!
// ========================================
#define ACTIVE_APP 0  // 0=wifi_basic, 1=multi_thread, 2=http_server, etc.

#endif // APP_REGISTRY_H