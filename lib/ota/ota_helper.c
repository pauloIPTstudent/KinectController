#include "ota_helper.h"


esp_err_t ota_helper_switch_and_reboot(
    esp_partition_type_t target_type, 
    esp_partition_subtype_t target_subtype,
    const char *label) 
{
    const esp_partition_t *loader_part = esp_partition_find_first(
        target_type, 
        target_subtype, 
        label
    );

    if (loader_part != NULL) {
        // 2. Escreve na partição 'otadata' dizendo ao bootloader para arrancar por ela
        esp_err_t err = esp_ota_set_boot_partition(loader_part);
        if (err == ESP_OK) {
            ESP_LOGI("OTA", "Próximo arranque definido para o Loader (ota_1)!");
            // 3. Reinicia o ESP32 para aplicar
            esp_restart();
        }
    } else {
        ESP_LOGE("OTA", "Partição ota_1 (Loader) não encontrada!");
        return ESP_FAIL;
    }
    return ESP_OK;
}


void handle_ble_updated_block(uint8_t *bluetooth_data, size_t block_size) 
{
    static esp_ota_handle_t ota_handle = 0;
    static bool first_block = true;
    if (first_block) {
        const esp_partition_t *update_partition = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL
        );
        // Inicializa a partição ota_0 (limpa o espaço necessário)
        esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
        first_block = false;
    }
    // Grava o bloco de bytes que o telemóvel acabou de enviar por Bluetooth
    esp_ota_write(ota_handle, (const void *)bluetooth_data, block_size);
}