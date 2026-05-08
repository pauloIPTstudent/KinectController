/*#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "wifi.h"

static const char *TAG = "wifi_example";


void on_sta_disconnected(void) {
    ESP_LOGI(TAG, "Callback: STA desconectada!");
}

void on_got_ip(ip_event_got_ip_t *event) {
    ESP_LOGI(TAG, "Callback: Obtive IP: " IPSTR, IP2STR(&event->ip_info.ip));
}


void main(void){

    // Inicializa NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();
    wifi_init_event_handler();

    wifi_event_callbacks_t cbs = {
        .sta_disconnected = on_sta_disconnected,
        .got_ip = on_got_ip
    };

    wifi_register_callbacks(&cbs);
    // Configuração do AP
    wifi_ap_config_t ap_config = {
        .ssid = "DAISY-XYZ",
        .password = "12345678",
        .max_connection = 1
    };

    // Inicia o Wi-Fi em modo AP
    wifi_start_ap(&ap_config);
}*/