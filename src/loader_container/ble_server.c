#include <string.h>
#include <stdio.h>
#include "esp_log.h"

// Headers de inicialização do NimBLE
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

// Headers do Core do Host NimBLE
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_gatt.h"

// Serviços padrão
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char* TAG = "BLE_LOADER";

// UUIDs Únicos para o teu serviço de atualização (128-bit)
static const ble_uuid128_t gatt_service_ota_uuid =
    BLE_UUID128_INIT(0x00, 0x00, 0x42, 0xfc, 0x44, 0x55, 0xaf, 0x89, 0xec, 0x47, 0x1a, 0xde, 0x8e, 0x33, 0x43, 0x1d);

static const ble_uuid128_t gatt_char_dados_uuid =
    BLE_UUID128_INIT(0x01, 0x00, 0x42, 0xfc, 0x44, 0x55, 0xaf, 0x89, 0xec, 0x47, 0x1a, 0xde, 0x8e, 0x33, 0x43, 0x1d);

static int ota_handle_packet(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int ota_ble_gap_event(struct ble_gap_event *event, void *arg);

static const struct ble_gatt_chr_def ota_characteristics[] = {
    {
        .uuid = &gatt_char_dados_uuid.u,
        .access_cb = ota_handle_packet,
        .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
    },
    {0} 
};

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_service_ota_uuid.u,
        .characteristics = ota_characteristics,
    },
    {0} 
};

static int ota_handle_packet(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        struct os_mbuf *om = ctxt->om;
        
        while (om != NULL) {
            uint8_t *buffer_dados = om->om_data;  
            uint16_t tamanho_bloco = om->om_len; 
            
            printf("Recebidos %d bytes via BLE\n", tamanho_bloco);
            om = SLIST_NEXT(om, om_next); 
        }
        return 0; 
    }
    return BLE_ATT_ERR_REQ_NOT_SUPPORTED; 
}

// O OBRIGATÓRIO: Gerenciador de eventos GAP (Conexão, desconexão, etc.)
static int ota_ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "Dispositivo Conectado! Status: %d", event->connect.status);
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Dispositivo Desconectado! Reiniciando Advertisement...");
            // Se desconectar, precisamos voltar a anunciar
            struct ble_gap_adv_params adv_params;
            memset(&adv_params, 0, sizeof(adv_params));
            adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
            adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
            ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, ota_ble_gap_event, NULL);
            break;
        default:
            break;
    }
    return 0;
}

// Callback disparado quando o Host do BLE está sincronizado
static void ota_ble_on_sync(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    
    // Configura o nome do dispositivo no GAP corporativo
    ble_svc_gap_device_name_set("Pulseira_Loader");

    memset(&fields, 0, sizeof(fields));
    const char *nome_dispositivo = "Pulseira_Loader";
    fields.name = (uint8_t *)nome_dispositivo;
    fields.name_len = strlen(nome_dispositivo);
    fields.name_is_complete = 1;
    
    ble_gap_adv_set_fields(&fields);
    
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; 
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; 
    
    // CORREÇÃO: Passado 'ota_ble_gap_event' em vez de NULL
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, ota_ble_gap_event, NULL);
    ESP_LOGI(TAG, "BLE Anunciando com sucesso...");
}

// Função executada pela Task do NimBLE
void ota_ble_host_task(void *param) {
    ESP_LOGI(TAG, "NimBLE Host Task Iniciada.");
    nimble_port_run(); // Esta função bloqueia a task e roda o loop do BLE
    nimble_port_freertos_deinit();
}

void inicializar_ble_completo(void) 
{
    nimble_port_init();
    
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    // Registra os teus serviços customizados no NimBLE
    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) { ESP_LOGE(TAG, "Erro gatts_count: %d", rc); }
    
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) { ESP_LOGE(TAG, "Erro gatts_add: %d", rc); }
    
    // Define o callback de sincronização
    ble_hs_cfg.sync_cb = ota_ble_on_sync;
    
    // Cria a Task do FreeRTOS da forma correta exigida pelo NimBLE no ESP32
    xTaskCreate(ota_ble_host_task, "nimble_host_task", 4096, NULL, 5, NULL);
}