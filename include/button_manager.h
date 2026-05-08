#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Mapeamento dos GPIOs
#define BUTTON_UP    GPIO_NUM_20
#define BUTTON_DOWN  GPIO_NUM_21
#define BUTTON_LEFT  GPIO_NUM_18
#define BUTTON_RIGHT GPIO_NUM_19
#define BUTTON_X     GPIO_NUM_22
#define BUTTON_B     GPIO_NUM_23

typedef enum {
    BUTTON_ACTION_RELEASE = 0,
    BUTTON_ACTION_PRESS = 1
} button_action_t;

typedef struct {
    int pin;
    button_action_t action;
} button_event_t;

// Inicializa os GPIOs e a Task de monitoramento
void init_buttons();
void button_handler_task(void* arg);

#endif