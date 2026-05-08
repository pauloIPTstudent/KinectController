#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "button_manager.h"
#include "buffer_manager.h"
#include "mqtt_client.h"
static const char *TAG = "BUTTON_TASK";
extern esp_mqtt_client_handle_t client;
// Fila interna para comunicar a Interrupção (ISR) com a Task

// Estrutura para traduzir o GPIO para o nome do botão (igual ao HTML)
typedef struct {
    gpio_num_t pin;
    char* name;
} button_map_t;

static const button_map_t buttons[] = {
    {BUTTON_LEFT,  "ArrowLeft"},
    {BUTTON_RIGHT, "ArrowRight"},
    {BUTTON_UP,    "ArrowUp"},
    {BUTTON_DOWN,  "ArrowDown"},
    {BUTTON_X,     "X"},
    {BUTTON_B,     "B"}
};

// Handler de Interrupção (ISR) - Roda na RAM para velocidade máxima
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// A Task que você vai iniciar no seu main
void button_handler_task(void* arg) {
    uint32_t io_num;
    char json_payload[64];

    ESP_LOGI(TAG, "Aguardando eventos de botão para envio MQTT...");

    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            
            // Debounce
            vTaskDelay(pdMS_TO_TICKS(20)); 

            int level = gpio_get_level(io_num);
            const char* action = (level == 0) ? "press" : "release";
            const char* btn_name = "unknown";

            for(int i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
                if(buttons[i].pin == io_num) {
                    btn_name = buttons[i].name;
                    break;
                }
            }

            // Formata o JSON igual ao que o seu HTML enviaria via Socket.io
            snprintf(json_payload, sizeof(json_payload), 
                     "{\"button\":\"%s\",\"action\":\"%s\"}", 
                     btn_name, action);

            // Verifica se o cliente MQTT está pronto antes de publicar
            if (client != NULL) {
                int msg_id = esp_mqtt_client_publish(client, "game/control", json_payload, 0, 1, 0);
                ESP_LOGD(TAG, "MQTT Sent: %s (id=%d)", json_payload, msg_id);
            } else {
                ESP_LOGW(TAG, "MQTT client não inicializado. Ignorando clique.");
            }
        }
    }
}
void init_buttons() {
    // Cria a fila para 10 eventos simultâneos
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,          // Interrompe em QUALQUER mudança (press e release)
        .pin_bit_mask = (1ULL<<BUTTON_LEFT) | (1ULL<<BUTTON_RIGHT) | 
                        (1ULL<<BUTTON_UP)   | (1ULL<<BUTTON_DOWN)  | 
                        (1ULL<<BUTTON_X)    | (1ULL<<BUTTON_B),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,                         // Ativa resistor interno (Botão liga no GND)
        .pull_down_en = 0,
    };
    gpio_config(&io_conf);

    // Instala o serviço de interrupção global
    gpio_install_isr_service(0);

    // Adiciona o handler para cada pino definido
    for(int i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
        gpio_isr_handler_add(buttons[i].pin, gpio_isr_handler, (void*) buttons[i].pin);
    }
}