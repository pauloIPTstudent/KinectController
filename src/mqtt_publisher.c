#include "mqtt_client.h"
#include "esp_log.h"
#include "semaphore_manager.h"

#define MQTT_BROKER_URI "mqtt://192.168.1.233"
#define MQTT_TOPIC     "game/control"


esp_mqtt_client_handle_t client;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI("MQTT", "Conectado ao broker");
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW("MQTT", "Desconectado");
            break;

        default:
            break;
    }
}


void mqtt_publisher_task(void *pvParameters) 
{
    xSemaphoreTake(gotip, portMAX_DELAY);
    xSemaphoreGive(gotip);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    vTaskDelete(NULL);
}