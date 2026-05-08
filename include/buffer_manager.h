#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Tipo do item do buffer
typedef struct reading {
    int16_t humidity;
    int light;
    int16_t temperature;
}reading_type;

// Handle do buffer (exportado)
extern QueueHandle_t sta_credenticial;
extern QueueHandle_t token_buffer;
extern QueueHandle_t reading_buffer;
extern QueueHandle_t gpio_evt_queue;

// Inicializa buffer
void buffer_init(void);

#endif // BUFFER_MANAGER_H