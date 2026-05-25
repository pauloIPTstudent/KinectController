#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "cJSON.h"
#include "wifi_provider.h"
#include "buffer_manager.h"
#include "wifi.h"

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

static const char* TAG = "BLE_MAIN";

// --- VARIÁVEIS GLOBAIS ---
static uint8_t ble_addr_type;
static uint16_t gatt_status_chr_val_handle = 0; 
static uint16_t atual_conn_handle = 0xFFFF; // NOVO: Armazena o ID da conexão ativa (0xFFFF = desconectado)

// Proclamação da função de eventos GAP para o compilador não reclamar da ordem
static int ble_gap_event(struct ble_gap_event *event, void *arg);

// --- 1. DEFINIÇÃO DOS UUIDs ---
static const ble_uuid128_t gatt_set_credentials_service_uuid =
    BLE_UUID128_INIT(0x00, 0x00, 0x42, 0xfc, 0x44, 0x55, 0xaf, 0x89, 0xec, 0x47, 0x1a, 0xde, 0x8e, 0x33, 0x43, 0x1d);

static const ble_uuid128_t chr_credentials_uuid =
    BLE_UUID128_INIT(0x01, 0x00, 0x42, 0xfc, 0x44, 0x55, 0xaf, 0x89, 0xec, 0x47, 0x1a, 0xde, 0x8e, 0x33, 0x43, 0x1d);

static const ble_uuid128_t chr_status_uuid =
    BLE_UUID128_INIT(0x02, 0x00, 0x42, 0xfc, 0x44, 0x55, 0xaf, 0x89, 0xec, 0x47, 0x1a, 0xde, 0x8e, 0x33, 0x43, 0x1d);

// NOVO: UUID para a característica que vai receber o comando do telemóvel
static const ble_uuid128_t chr_trigger_uuid =
    BLE_UUID128_INIT(0x03, 0x00, 0x42, 0xfc, 0x44, 0x55, 0xaf, 0x89, 0xec, 0x47, 0x1a, 0xde, 0x8e, 0x33, 0x43, 0x1d);


// --- 2. CALLBACKS DE LEITURA E ESCRITA (GATT) ---
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ble_uuid_cmp(ctxt->chr->uuid, &chr_credentials_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            char recebido[256] = {0}; // Aumentei para 128 para garantir que cabe o JSON inteiro confortavelmente
            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            
            if (len < sizeof(recebido)) {
                ble_hs_mbuf_to_flat(ctxt->om, recebido, len, NULL);
                ESP_LOGI(TAG, "JSON Recebido: %s", recebido);

                // --- Início do Parse do JSON ---
                cJSON *json = cJSON_Parse(recebido);
                if (json == NULL) {
                    ESP_LOGE(TAG, "Erro: JSON inválido!");
                    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN; // Retorna erro pro app BLE
                }

                cJSON *ssid_item = cJSON_GetObjectItemCaseSensitive(json, "ssid");
                cJSON *pass_item = cJSON_GetObjectItemCaseSensitive(json, "password");

                if (cJSON_IsString(ssid_item) && cJSON_IsString(pass_item)) {
                    ESP_LOGI(TAG, "SSID Extraído: %s", ssid_item->valuestring);
                    ESP_LOGI(TAG, "PASS Extraído: [PROTEGIDO]");
                    wifi_sta_config_t std;
                    memset(&std, 0, sizeof(wifi_sta_config_t));
                    strncpy((char *)std.ssid, ssid_item->valuestring, sizeof(std.ssid) - 1);
                    strncpy((char *)std.password, pass_item->valuestring, sizeof(std.password) - 1);

                    BaseType_t sta_ok = xQueueSend(sta_credenticial, &std, pdMS_TO_TICKS(100));
                } else {
                    ESP_LOGE(TAG, "JSON não contém 'ssid' ou 'password' no formato correto.");
                }
                cJSON_Delete(json); // IMPORTANTE: Sempre limpe a memória do cJSON
            }
            return 0;
        }
    }
    
    // NOVA LÓGICA: Detetar quando o telemóvel escreve na característica Trigger
    if (ble_uuid_cmp(ctxt->chr->uuid, &chr_trigger_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            char comando[64] = {0};
            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            
            if (len < sizeof(comando)) {
                ble_hs_mbuf_to_flat(ctxt->om, comando, len, NULL);
                
                // Imprime algo no terminal do ESP32!
                printf("\n========================================\n");
                printf("💥 NOTIFICAÇÃO RECEBIDA DO TELEMÓVEL! 💥\n");
                printf("Conteúdo do comando: %s\n", comando);
                printf("========================================\n\n");
                
                ESP_LOGI(TAG, "Evento disparado com sucesso via BLE.");
            }
            return 0;
        }
    }
    return BLE_ATT_ERR_UNLIKELY;
}

// --- 3. TABELA DE SERVIÇOS E CARACTERÍSTICAS ---
static const struct ble_gatt_chr_def ota_characteristics[] = {
    {
        .uuid = &chr_credentials_uuid.u,
        .access_cb = gatt_svr_chr_access,
        .flags = BLE_GATT_CHR_F_WRITE,
    },
    {
        .uuid = &chr_status_uuid.u,
        .access_cb = gatt_svr_chr_access,
        .val_handle = &gatt_status_chr_val_handle, 
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
    },{
        // Nova característica adicionada à tabela
        .uuid = &chr_trigger_uuid.u,
        .access_cb = gatt_svr_chr_access,
        .flags = BLE_GATT_CHR_F_WRITE, // Permite que o telemóvel envie dados
    },
    {0} 
};

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_set_credentials_service_uuid.u,
        .characteristics = ota_characteristics,
    },
    {0} 
};








// --- 4. FUNÇÃO PARA DISPARAR NOTIFICAÇÃO ---
void ble_enviar_estado_evento(const char* novo_estado) {
    if (gatt_status_chr_val_handle == 0) {
        ESP_LOGW(TAG, "Nenhum cliente conectado para notificar.");
        return;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(novo_estado, strlen(novo_estado));
    if (om != NULL) {
        // Envia para o handle que o NimBLE registou automaticamente
        ble_gatts_notify_custom(BLE_HS_CONN_HANDLE_NONE, gatt_status_chr_val_handle, om);
        ESP_LOGI(TAG, "Notificação enviada: %s", novo_estado);
    }
}


// --- FUNÇÃO PARA DISPARAR ALERTA DE QUEDA (1 BYTE) ---

// ... seu código intermédio permanece igual ...

// --- FUNÇÃO PARA DISPARAR ALERTA DE QUEDA ATUALIZADA ---
void ble_notificar_queda(void) 
{
    // 1. Verifica se temos um handle de conexão válido salvo
    if (atual_conn_handle == 0xFFFF) {
        ESP_LOGW(TAG, "Nenhum dispositivo conectado via GAP para receber notificações.");
        return;
    }

    if (gatt_status_chr_val_handle == 0) {
        ESP_LOGW(TAG, "Handle da característica inválido.");
        return;
    }

    uint8_t payload_queda = 1;

    struct os_mbuf *om = ble_hs_mbuf_from_flat(&payload_queda, sizeof(payload_queda));
    if (om != NULL) {
        // CORREÇÃO: Passamos 'atual_conn_handle' em vez de BLE_HS_CONN_HANDLE_NONE
        int rc = ble_gatts_notify_custom(atual_conn_handle, gatt_status_chr_val_handle, om);
        if (rc == 0) {
            ESP_LOGI(TAG, "Notificação de QUEDA enviada com sucesso! [1 byte]");
        } else {
            ESP_LOGE(TAG, "Erro ao enviar notificação BLE: %d", rc);
        }
    }
}

// --- 5. ADVERTISEMENT ---
void ble_app_advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields rsp_fields; // NOVO: Campos para a resposta de scan
    int rc;

    // ==========================================
    // 1. PACOTE PRINCIPAL (Advertising Data)
    // ==========================================
    memset(&fields, 0, sizeof(fields));
    
    // Flags obrigatórias
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    // Injetar o UUID de 128-bits aqui (Filtro do Android vai detetar na hora)
    fields.uuids128 = (ble_uuid128_t *)&gatt_set_credentials_service_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) { 
        ESP_LOGE(TAG, "Erro ao definir campos principais do adv: %d", rc); 
        return; 
    }

    // ==========================================
    // 2. PACOTE SECUNDÁRIO (Scan Response Data)
    // ==========================================
    memset(&rsp_fields, 0, sizeof(rsp_fields));

    // Colocamos o Nome completo aqui sem medo de estourar o limite
    const char *name = "ESP32_Config";
    rsp_fields.name = (uint8_t *)name;
    rsp_fields.name_len = strlen(name);
    rsp_fields.name_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Erro ao definir campos do Scan Response: %d", rc);
        return;
    }

    // ==========================================
    // 3. INICIAR ANÚNCIO
    // ==========================================
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // Permite conexões
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // Modo descobrível geral

    rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
    if (rc != 0) { 
        ESP_LOGE(TAG, "Erro ao iniciar adv: %d", rc); 
    } else {
        ESP_LOGI(TAG, "Anúncio BLE iniciado com sucesso (Serviço + Scan Response)!");
    }
}







static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Dispositivo Conectado com sucesso! Conn Handle: %d", event->connect.conn_handle);
                atual_conn_handle = event->connect.conn_handle; // SALVA O HANDLE DA CONEXÃO ATIVA
            } else {
                ESP_LOGE(TAG, "Falha na conexão; reiniciando anúncio. Status: %d", event->connect.status);
                ble_app_advertise();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Dispositivo Desconectado! Limpando Handle e Reiniciando Advertisement...");
            atual_conn_handle = 0xFFFF; // LIMPA O HANDLE COM VALOR DE DESCONEXÃO
            ble_app_advertise(); 
            break;
            
        default:
            break;
    }
    return 0;
}






//--- INITIALIZAÇÃO DO BLE ---


static void ota_ble_on_sync(void) {
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_advertise(); 
}

void ble_host_task(void *param) {
    ESP_LOGI(TAG, "NimBLE Host Task Iniciada.");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void init_ble(void) 
{
    nimble_port_init();
    
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    ble_svc_gap_device_name_set("ESP32_Config");
    
    
    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) { ESP_LOGE(TAG, "Erro gatts_count: %d", rc); }
    
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) { ESP_LOGE(TAG, "Erro gatts_add: %d", rc); }
    
    ble_hs_cfg.sync_cb = ota_ble_on_sync;
}