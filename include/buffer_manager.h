#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Tipo do item do buffer
#define FREQ_HZ          18
#define WINDOW_DURATION  2
#define WINDOW_SIZE      (FREQ_HZ * WINDOW_DURATION) // 36

// A mesma estrutura de amostra que usamos na task do MPU
typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
} imu_sample_t;

typedef struct {
    imu_sample_t amostras[WINDOW_SIZE];
} reading_type;
// Handle do buffer (exportado)
extern QueueHandle_t sta_credenticial;
extern QueueHandle_t reading_buffer;
extern QueueHandle_t gpio_evt_queue;

// Inicializa buffer
void buffer_init(void);

#endif // BUFFER_MANAGER_H