#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "http_server.h"
#include "buffer_manager.h"
#include "wifi.h"
#include "cJSON.h"


static const char *TAG = "HTTP_SERVER_PROVIDER";

/* ---------------------------
   Endpoint: POST /wifi
---------------------------*/
static esp_err_t wifi_post_handler(httpd_req_t *req)
{
    char content[256];
    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret <= 0) return ESP_FAIL;

    content[ret] = '\0';
    ESP_LOGI(TAG, "Payload recebido: %s", content);

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "JSON invalido");
        return ESP_FAIL;
    }

    cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON *password = cJSON_GetObjectItem(json, "password");
    cJSON *token = cJSON_GetObjectItem(json, "token");

    if (!cJSON_IsString(ssid) || !cJSON_IsString(password)|| !cJSON_IsString(token)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Campos invalidos");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SSID: %s", ssid->valuestring);
    ESP_LOGI(TAG, "PASSWORD: %s", password->valuestring);
    ESP_LOGI(TAG, "TOKEN: %s", token->valuestring);

    char token_str[64] = {0};
    strncpy(token_str, token->valuestring, 64 - 1);
    wifi_sta_config_t new_sta = {0};

    strncpy((char*)new_sta.ssid, ssid->valuestring, sizeof(new_sta.ssid));
    strncpy((char*)new_sta.password, password->valuestring, sizeof(new_sta.password));

    BaseType_t sta_ok = xQueueSend(sta_credenticial, &new_sta, pdMS_TO_TICKS(100));
    BaseType_t token_ok = xQueueSend(token_buffer, token_str, pdMS_TO_TICKS(100));

    if (sta_ok != pdTRUE || token_ok != pdTRUE) {
        ESP_LOGW(TAG, "Fila cheia: sta_ok=%d, token_ok=%d. Credenciais descartadas.", sta_ok, token_ok);
    } else {
        // Aqui você pode chamar sua função para salvar e conectar
        // save_wifi_credentials(ssid->valuestring, password->valuestring);
        // mandar semaphoro para wifi_stop ap e wifi_start_sta(...);
        // colocar o user token no buffer
        ESP_LOGI(TAG, "Credenciais enviadas para wifi_provider");
    }

    

    const char *resp = "{\"result\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    cJSON_Delete(json);
    return ESP_OK;
}

/* ---------------------------
   Lista de endpoints
---------------------------*/
static httpd_uri_t endpoints[] = {
    {
        .uri       = "/wifi",
        .method    = HTTP_POST,
        .handler   = wifi_post_handler,
        .user_ctx  = NULL
    }
};
void http_server_provider_init(){
    http_server_endpoints_t server_endpoints = {
        .endpoints = endpoints,
        .count = sizeof(endpoints) / sizeof(endpoints[0])
    };

    httpd_handle_t server = http_server_start(&server_endpoints);
    if (!server) ESP_LOGE(TAG, "Erro ao iniciar servidor HTTP");//reiniciar
}
void http_server_provider_task(void *pvParameters){
    while (1) {
        // caso em modo AP (semaforo)
            //httpd_handle_t server = http_server_start(&server_endpoints);
            //if (!server) ESP_LOGE(TAG, "Erro ao iniciar servidor HTTP");//reiniciar 
        // caso saia do modo AP (semaforo)
            // desliga o servidor http

        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}