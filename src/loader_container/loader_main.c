#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"  // Header que define os Semáforos e UBaseType_t
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_ota_ops.h"
#include "ble_server.h"

static const char *TAG = "MAIN_LOADER";

void passar_comando_para_controller(void) {
    ESP_LOGI(TAG, "A procurar a partição do Controller (ota_0)...");
    
    // Procura explicitamente a ota_0 (onde está o teu Controller)
    const esp_partition_t *controller_part = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, 
        ESP_PARTITION_SUBTYPE_APP_OTA_0, 
        NULL
    );

    if (controller_part != NULL) {
        // Altera o registo no otadata para apontar para a ota_0
        esp_err_t err = esp_ota_set_boot_partition(controller_part);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Sucesso! Próximo arranque definido para o Controller (ota_0).");
            ESP_LOGI(TAG, "A reiniciar o ESP32...");
            vTaskDelay(pdMS_TO_TICKS(500)); 
            esp_restart();
        } else {
            ESP_LOGE(TAG, "Erro ao gravar no otadata: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Erro: Partição ota_0 não foi encontrada na tabela!");
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Inicializando sistema...");
    ESP_LOGI(TAG, "Contagem regressiva");
    // Inicializa NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    inicializar_ble_completo();
    //while(1) {
        ESP_LOGI(TAG, "Log from Loader");
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    //}
    
    // Chama a função para alternar o boot
    //passar_comando_para_controller();
}