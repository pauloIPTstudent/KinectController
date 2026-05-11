#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "wifi_provider.h"
#include "buffer_manager.h"
#include "semaphore_manager.h"
#include "mqtt_publisher.h"
#include "config.h"
#include "cJSON.h"
#include "button_manager.h"

static const char *TAG = "MAIN";

void app_main() {
    ESP_LOGI(TAG, "Inicializando sistema...");

    // Inicializa NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    semaphore_init();
    buffer_init();
    wifi_provider_init();
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    init_buttons();
    //initial wifi configuration
    wifi_sta_config_t std = {
        .ssid = STA_WIFI_SSID,
        .password = STA_WIFI_PASSWORD
    };

    BaseType_t sta_ok = xQueueSend(sta_credenticial, &std, pdMS_TO_TICKS(100));
    
    xTaskCreate(
        wifi_provider_task,
        "wifi_provider",
        8192, //4096
        NULL,
        5,
        NULL
    );
    xTaskCreate(
        mqtt_publisher_task,
        "mqtt_publisher",
        8192,
        NULL,
        5,
        NULL
    );//*/
    xTaskCreate(
        button_handler_task, 
        "btn_task", 
        4096, 
        NULL, 
        10, 
        NULL
    );
}