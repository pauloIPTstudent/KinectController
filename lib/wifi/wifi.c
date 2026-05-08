#include "wifi.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_mac.h"
#include <arpa/inet.h>   
static const char *TAG = "wifi";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif = NULL;

static wifi_event_callbacks_t wifi_callbacks = {0};

//
void wifi_register_callbacks(wifi_event_callbacks_t *callbacks)
{
    if (!callbacks) return;

    wifi_callbacks.sta_connected = callbacks->sta_connected;
    wifi_callbacks.sta_disconnected = callbacks->sta_disconnected;
    wifi_callbacks.got_ip = callbacks->got_ip;
    wifi_callbacks.ap_sta_connected = callbacks->ap_sta_connected;
    wifi_callbacks.ap_sta_disconnected = callbacks->ap_sta_disconnected;
}
// Função que registra os eventos
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA iniciado, tentando conectar...");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "Desconectado do Wi-Fi, tentando reconectar...");
                esp_wifi_connect();
                xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
                xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
                if (wifi_callbacks.sta_disconnected) wifi_callbacks.sta_disconnected();
                break;

            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "AP iniciado");
                break;

            case WIFI_EVENT_AP_STACONNECTED:
                if (wifi_callbacks.ap_sta_connected)
                    wifi_callbacks.ap_sta_connected((wifi_event_ap_staconnected_t *)event_data);
                ESP_LOGI(TAG, "Estação conectada ao AP");
                break;

            case WIFI_EVENT_AP_STADISCONNECTED:
                if (wifi_callbacks.ap_sta_disconnected)
                    wifi_callbacks.ap_sta_disconnected((wifi_event_ap_stadisconnected_t *)event_data);
                ESP_LOGI(TAG, "Estação desconectada do AP");
                break;

            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "STA obteve IP: " IPSTR, IP2STR(&event->ip_info.ip));
        set_sta_dns(sta_netif);
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        if (wifi_callbacks.got_ip) wifi_callbacks.got_ip(event);
    }
}

// Função para inicializar o event handler
void wifi_init_event_handler()
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,                   // Tipo de evento
        ESP_EVENT_ANY_ID,              // Qualquer ID de evento
        &wifi_event_handler,           // Função que trata o evento
        NULL,                          // Dados adicionais
        NULL));                        // Instância de evento
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,                      // Tipo de evento para IP
        IP_EVENT_STA_GOT_IP,           // Evento de recebimento de IP via STA
        &wifi_event_handler,           // Função que trata o evento
        NULL,                          // Dados adicionais
        NULL));                        // Instância de evento
}

void wifi_init()
{
    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();

    sta_netif = esp_netif_create_default_wifi_sta();
    ap_netif  = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_storage(WIFI_STORAGE_RAM);
}


void set_sta_dns(esp_netif_t *sta)
{
    if (!sta) return;

    esp_netif_dns_info_t dns;
    dns.ip.type = ESP_IPADDR_TYPE_V4;

    // Opção 1: definir IP manualmente com bytes
    dns.ip.u_addr.ip4.addr = (8 << 24) | (8 << 16) | (8 << 8) | 8; // 8.8.8.8
    dns.ip.u_addr.ip4.addr = htonl(dns.ip.u_addr.ip4.addr);          // converte para network order

    // Opção 2: ou usando inet_pton (mais legível)
    // inet_pton(AF_INET, "8.8.8.8", &dns.ip.u_addr.ip4);

    esp_err_t err = esp_netif_set_dns_info(sta, ESP_NETIF_DNS_MAIN, &dns);
    if (err != ESP_OK) {
        ESP_LOGE("DNS", "Erro ao setar DNS: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI("DNS", "DNS manual definido: 8.8.8.8");
}

void wifi_start_sta(wifi_sta_config_t *config)
{
    wifi_config_t sta_config = {0};

    strcpy((char*)sta_config.sta.ssid, (const char*)config->ssid);
    strcpy((char*)sta_config.sta.password, (const char*)config->password);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &sta_config);

    esp_wifi_start();
    esp_wifi_connect();

    ESP_LOGI(TAG, "STA started");
}

void wifi_stop_sta()
{
    ESP_ERROR_CHECK(esp_wifi_disconnect());  // Desconectar do Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_stop());        // Parar o Wi-Fi
    ESP_LOGI(TAG, "Modo STA parado");
}

void wifi_start_ap(wifi_ap_config_t *config)
{
    wifi_config_t ap_config = {0};

    strcpy((char*)ap_config.ap.ssid, (const char*)config->ssid);
    strcpy((char*)ap_config.ap.password, (const char*)config->password);

    ap_config.ap.max_connection = config->max_connection;
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);

    esp_wifi_start();

    ESP_LOGI(TAG, "AP started");
}

void wifi_stop_ap()
{
    ESP_ERROR_CHECK(esp_wifi_stop());        // Parar o Wi-Fi
    ESP_LOGI(TAG, "Modo AP parado");
}

void wifi_disable_ap_keep_sta()
{
    wifi_mode_t mode;

    esp_err_t err = esp_wifi_get_mode(&mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao obter modo WiFi: %s", esp_err_to_name(err));
        return;
    }

    if (mode == WIFI_MODE_APSTA) {
        err = esp_wifi_set_mode(WIFI_MODE_STA);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "AP desativado, STA mantido ativo");
        } else {
            ESP_LOGE(TAG, "Erro ao desativar AP: %s", esp_err_to_name(err));
        }
    } 
    else if (mode == WIFI_MODE_AP) {
        ESP_LOGW(TAG, "Dispositivo estava apenas em AP, WiFi não será parado automaticamente");
    } 
    else {
        ESP_LOGI(TAG, "AP já está desativado");
    }
}

void wifi_start_ap_sta(wifi_sta_config_t *sta, wifi_ap_config_t *ap)
{
    wifi_config_t sta_config = {0};
    wifi_config_t ap_config = {0};

    // Corrigido: Cast para const char* ao usar strcpy
    strcpy((char*)sta_config.sta.ssid, (const char*)sta->ssid);
    strcpy((char*)sta_config.sta.password, (const char*)sta->password);

    strcpy((char*)ap_config.ap.ssid, (const char*)ap->ssid);
    strcpy((char*)ap_config.ap.password, (const char*)ap->password);

    ap_config.ap.max_connection = ap->max_connection;
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_APSTA);

    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);

    esp_wifi_start();
    esp_wifi_connect();

    ESP_LOGI(TAG, "AP + STA started");
}

void wifi_stop_ap_sta()
{
    ESP_ERROR_CHECK(esp_wifi_stop());        // Parar o Wi-Fi
    ESP_LOGI(TAG, "Modo AP + STA parado");
}

void wifi_update_sta_credentials(wifi_sta_config_t *config)
{
    esp_err_t err;

    ESP_LOGI("wifi", "Atualizando credenciais STA");
    ////////////////////
    if (!config) return;

    ESP_LOGI(TAG, "Atualizando credenciais STA...");

    wifi_mode_t current_mode;
    err = esp_wifi_get_mode(&current_mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Não foi possível ler o modo Wi-Fi: %s", esp_err_to_name(err));
        return;
    }

    // Se estamos apenas em AP, mudar para AP+STA
    if (current_mode == WIFI_MODE_AP) {
        ESP_LOGI(TAG, "Modo atual AP apenas, mudando para AP+STA...");
        err = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Erro ao alterar para AP+STA: %s", esp_err_to_name(err));
            return;
        }
    }
    /////
    err = esp_wifi_disconnect();
    if (err != ESP_OK) {
        ESP_LOGW("wifi", "STA não estava conectado");
    }

    err = esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t *)config);
    if (err != ESP_OK) {
        ESP_LOGE("wifi", "Erro ao atualizar config: %s", esp_err_to_name(err));
        return;
    }

    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE("wifi", "Erro ao reconectar: %s", esp_err_to_name(err));
    }
}

bool wifi_is_connected()
{
    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    return bits & WIFI_CONNECTED_BIT;
}