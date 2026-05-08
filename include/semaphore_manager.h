#ifndef SEMAPHORE_MANAGER_H
#define SEMAPHORE_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Handle do semáforo (exportado)
extern SemaphoreHandle_t gotip;
extern SemaphoreHandle_t statrt_readings; // handle global

// Inicializa semáforo
void semaphore_init(void);

#endif // SEMAPHORE_MANAGER_H