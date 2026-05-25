#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "nvs_flash.h"
#include "esp_log.h"

#define TAG "LOADER_MAIN"


// Callback de Eventos do SPP mapeado conforme o teu enum 'esp_spp_cb_event_t'


void app_main(void)
{

    // 1. Inicializa o armazenamento NVS
    while(true){
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Hello from loader...");
    }
}

/*
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

*/