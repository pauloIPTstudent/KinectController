#include "mqtt_client.h"
#include "esp_log.h"
#include "semaphore_manager.h"
#include "esp_crt_bundle.h"

#define MQTT_BROKER_URI "mqtts://752c1a993df64a28b80430f7f0948d2f.s1.eu.hivemq.cloud:8883" 
#define MQTT_PORT       8883
#define MQTT_USER       "KinectV"
#define MQTT_PASS       "Qwe12345"


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
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASS,

        //CERT TLS
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE("MQTT", "Falha ao inicializar o cliente (memória insuficiente ou config inválida)");
        vTaskDelete(NULL);
        return;
    }
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    vTaskDelete(NULL);
}

/*
extern esp_mqtt_client_handle_t client;
if (client != NULL) {
    int msg_id = esp_mqtt_client_publish(client, "game/control", json_payload, 0, 1, 0);
    ESP_LOGD(TAG, "MQTT Sent: %s (id=%d)", json_payload, msg_id);
} else {
    ESP_LOGW(TAG, "MQTT client não inicializado. Ignorando clique.");
}
*/