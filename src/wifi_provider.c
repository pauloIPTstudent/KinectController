#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_helper.h"
#include "esp_err.h"
#include "semaphore_manager.h"
#include "wifi_provider.h"
#include "wifi.h"
#include "buffer_manager.h"

static const char *TAG = "WIFI_PROVIDER";


void on_wifi_got_ip(ip_event_got_ip_t *event) {
    wifi_disable_ap_keep_sta();
    //vTaskDelay(pdMS_TO_TICKS(200));
    xSemaphoreGive(gotip);
    ESP_LOGI(TAG, "Callback: Got IP: ");

}

void on_wifi_sta_desconected() {
    //quando perder o ap se não se conectar em 10 segundos ligar ap para ficar com sta_ap
    xSemaphoreTake(gotip, pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Callback: STA Desconected: ");
}

void on_wifi_ap_desconected(wifi_event_ap_stadisconnected_t *event){
    ESP_LOGI(TAG, "Callback: AP Desconected: ");
}

void on_wifi_ap_conected(wifi_event_ap_staconnected_t *event){
    ESP_LOGI(TAG, "Callback: AP Desconected: ");
}
void wifi_provider_init(){
    xSemaphoreTake(gotip, portMAX_DELAY);
    wifi_init();
    wifi_init_event_handler();

    wifi_event_callbacks_t cbs = {
        .got_ip = on_wifi_got_ip,
        .sta_connected = on_wifi_sta_desconected,
        .ap_sta_disconnected = on_wifi_ap_desconected,
        .ap_sta_connected = on_wifi_ap_conected
    };

    wifi_register_callbacks(&cbs);

    char ssid[32];
    char password[64];

    ESP_LOGI(TAG, "Buscando Crendencias do WiFi");
    bool has_credentials = load_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password));
    wifi_sta_config_t sta_config = {
            .ssid = "",
            .password = ""
    };
    if (has_credentials) {
        ESP_LOGI(TAG, "Credenciais encontradas: SSID=%s", ssid);
        strncpy((char*)sta_config.ssid, ssid, sizeof(sta_config.ssid));
        strncpy((char*)sta_config.password, password, sizeof(sta_config.password));
    }
    wifi_start_sta(&sta_config);
}

void wifi_provider_task(void *pvParameters){
    while (1) {
        wifi_sta_config_t new_sta;

        if (xQueueReceive(sta_credenticial, &new_sta, pdMS_TO_TICKS(100)))
        {
            ESP_LOGI(TAG, "Novas credenciais recebidas!");

            ESP_LOGI(TAG, "SSID: %s", new_sta.ssid);
            ESP_LOGI(TAG, "PASSWORD: %s", new_sta.password);

            save_wifi_credentials((const char *)new_sta.ssid, (const char *)new_sta.password);
            wifi_update_sta_credentials(&new_sta);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
