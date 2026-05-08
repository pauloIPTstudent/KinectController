#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include "esp_wifi.h"  // Agora isso inclui a definição correta para wifi_sta_config_t e wifi_ap_config_t
#include "esp_log.h"

typedef void (*wifi_sta_connected_cb_t)(void);
typedef void (*wifi_sta_disconnected_cb_t)(void);
typedef void (*wifi_got_ip_cb_t)(ip_event_got_ip_t *event);
typedef void (*wifi_ap_sta_connected_cb_t)(wifi_event_ap_staconnected_t *event);
typedef void (*wifi_ap_sta_disconnected_cb_t)(wifi_event_ap_stadisconnected_t *event);
typedef struct {
    wifi_sta_connected_cb_t sta_connected;
    wifi_sta_disconnected_cb_t sta_disconnected;
    wifi_got_ip_cb_t got_ip;
    wifi_ap_sta_connected_cb_t ap_sta_connected;
    wifi_ap_sta_disconnected_cb_t ap_sta_disconnected;
} wifi_event_callbacks_t;

//static wifi_event_callbacks_t wifi_callbacks = {0};
// Inicializa o Wi-Fi e o evento de monitoramento
void wifi_init();
void wifi_init_event_handler();
void wifi_register_callbacks(wifi_event_callbacks_t *callbacks);

// Inicia o modo Estação (STA || AP || STA/AP) com a configuração fornecida
void wifi_start_sta(wifi_sta_config_t *config);
void wifi_start_ap(wifi_ap_config_t *config);
void wifi_start_ap_sta(wifi_sta_config_t *sta, wifi_ap_config_t *ap);

//
void wifi_stop_ap();
void wifi_stop_sta();
void wifi_stop_ap_sta();
void wifi_disable_ap_keep_sta();

//michelaneous
bool wifi_is_connected();
void wifi_update_sta_credentials(wifi_sta_config_t *config);
void set_sta_dns(esp_netif_t *sta);
#endif // WIFI_H