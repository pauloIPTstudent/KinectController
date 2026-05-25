#include "mqtt_client.h"
#include "esp_log.h"
#include "semaphore_manager.h"
#include "app_container/mqtt_publisher.h"
#include "esp_crt_bundle.h"

#define MQTT_BROKER_URI "mqtts://752c1a993df64a28b80430f7f0948d2f.s1.eu.hivemq.cloud:8883" 
#define MQTT_PORT       8883
#define MQTT_USER       "KinectV"
#define MQTT_PASS       "Qwe12345"


esp_mqtt_client_handle_t client;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client_local = event->client; // Copia o client para usar nas funções internas

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI("MQTT", "Conectado ao broker");
            
            // 1. INSCRIÇÃO NO TÓPICO: Faz o subscribe assim que conecta. 
            // O parâmetro '1' no final é o QoS (Quality of Service)
            int msg_id = esp_mqtt_client_subscribe(client_local, "status", 1);
            ESP_LOGI("MQTT", "Inscrição enviada com sucesso no tópico 'status', msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW("MQTT", "Desconectado");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI("MQTT", "Confirmação de inscrição recebida, msg_id=%d", event->msg_id);
            break;

        // 2. RECEBIMENTO DE DADOS: Esse evento roda toda vez que chega mensagem nova
        case MQTT_EVENT_DATA:
            ESP_LOGI("MQTT", "Mensagem recebida no tópico %.*s", event->topic_len, event->topic);
            
            // Segurança: Garante que recebemos pelo menos 1 byte de dado
            if (event->data_len > 0) {
                char caractere_recebido = event->data[0];

                // Verifica se o caractere está realmente entre '0' e '9'
                if (caractere_recebido >= '0' && caractere_recebido <= '9') {
                    
                    // Truque em C: Subtrair '0' do caractere numérico dá o valor inteiro exato.
                    // Exemplo: '3' (ASCII 51) - '0' (ASCII 48) = 3 inteiro.
                    mqtt_acoes_t event = (mqtt_acoes_t)(caractere_recebido - '0');

                    // Agora o switch decide o que disparar baseado no seu Enum
                    switch (event) {
                        case EVENT_0:
                            ESP_LOGI("EVENT", "Comando recebido: EVENT_0");
                            // Chame sua função aqui, ex: parar_motores();
                            break;
                        default:
                            ESP_LOGW("EVENT", "Evento não implementado.");
                            break;
                    }
                } else {
                    ESP_LOGW("MQTT", "Caractere inválido recebido: %c (Esperado de 0 a 9)", caractere_recebido);
                }
            }
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