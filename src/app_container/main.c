#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "wifi_provider.h"
#include "buffer_manager.h"
#include "semaphore_manager.h"
#include "ota_helper.h"
#include "config.h"
#include "cJSON.h"
#include "i2cdev.h"
#include "app_container/mqtt_publisher.h"
#include "app_container/ble_server.h"
#include "app_container/mpu6050.h"

static const char *TAG = "MAIN_APP";
#define ADDR MPU6050_I2C_ADDRESS_LOW
void app_main() {
    ESP_LOGI(TAG, "Inicializando sistema...");
    vTaskDelay(pdMS_TO_TICKS(5000)); 
    //ota_helper_switch_and_reboot(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    
    // Inicializa NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    // Inicializa I2C
    esp_err_t err = i2cdev_init();
    if (err != ESP_OK) {
        ESP_LOGE("MAIN_APP", "Falha ao inicializar o i2cdev!");
        return;
    }
    init_ble();
    //init build in led giopins
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    
    ESP_ERROR_CHECK(ret);
    semaphore_init();
    buffer_init();
    wifi_provider_init();

    vTaskDelay(pdMS_TO_TICKS(1000)); 
    //initial wifi configuration
    wifi_sta_config_t std = {
        .ssid = STA_WIFI_SSID,
        .password = STA_WIFI_PASSWORD
    };

    BaseType_t sta_ok = xQueueSend(sta_credenticial, &std, pdMS_TO_TICKS(100));
    
   /**/
    xTaskCreate(ble_host_task, 
        "nimble_host_task", 
        4096, 
        NULL, 
        5, 
        NULL
    );
    
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
    );
    xTaskCreate(mpu6050task, 
        "mpu6050_task", 
        4096, 
        NULL, 
        5, 
        NULL
    );
    /**/
    while(true)
    {
        vTaskDelay(pdMS_TO_TICKS(3000)); // Evita que a task termine
        //ble_notificar_queda(); // Exemplo: Notificar queda a cada 1 segundo (apenas para teste)
    }
}