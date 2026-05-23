#include "buffer_manager.h"
#include "esp_log.h"
#include "wifi.h"

#define BUFFER_LENGTH 10
#define JANELAS_EM_ESPERA 3
QueueHandle_t sta_credenticial; // handle global
QueueHandle_t reading_buffer; // handle global
QueueHandle_t gpio_evt_queue;

void buffer_init(void)
{
    sta_credenticial = xQueueCreate(BUFFER_LENGTH, sizeof(wifi_sta_config_t));
    reading_buffer = xQueueCreate(JANELAS_EM_ESPERA, sizeof(reading_type));
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if (sta_credenticial == NULL)
    {
        ESP_LOGE("BUFFER", "Falha ao criar buffer");
    }
    if (reading_buffer == NULL)
    {
        ESP_LOGE("BUFFER", "Falha ao criar buffer");
    }
    if (gpio_evt_queue == NULL)
    {
        ESP_LOGE("BUFFER", "Falha ao criar buffer");
    }
}