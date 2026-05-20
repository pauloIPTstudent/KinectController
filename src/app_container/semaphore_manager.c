#include "semaphore_manager.h"
#include "esp_log.h"

SemaphoreHandle_t gotip; // handle global
SemaphoreHandle_t statrt_readings; // handle global

void semaphore_init(void)
{
    gotip = xSemaphoreCreateBinary(); // cria semáforo binário
    statrt_readings = xSemaphoreCreateBinary(); // cria semáforo binário

    if (gotip == NULL)
    {
        ESP_LOGE("SEMAPHORE", "Falha ao criar semáforo");
    }
    else
    {
        xSemaphoreGive(gotip); // inicializa como disponível
    }
    if (statrt_readings == NULL)
    {
        ESP_LOGE("SEMAPHORE", "Falha ao criar semáforo");
    }
    else
    {
        xSemaphoreGive(statrt_readings); // inicializa como disponível
    }
}