#include "nvs_flash.h"
#include "nvs_helper.h"
#include "esp_log.h"

static const char *TAG = "NVS_EXAMPLE";

// Salvar Wi-Fi
void save_wifi_credentials(const char *ssid, const char *password) {
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("wifi", NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(nvs_set_str(handle, "ssid", ssid));
    ESP_ERROR_CHECK(nvs_set_str(handle, "password", password));
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
}

// Ler Wi-Fi
bool load_wifi_credentials(char *ssid, size_t ssid_len, char *password, size_t pwd_len) {
    nvs_handle_t handle;
    if (nvs_open("wifi", NVS_READONLY, &handle) != ESP_OK) return false;

    if (nvs_get_str(handle, "ssid", ssid, &ssid_len) != ESP_OK ||
        nvs_get_str(handle, "password", password, &pwd_len) != ESP_OK) {
        nvs_close(handle);
        return false;
    }
    nvs_close(handle);
    return true;
}

// Apagar Wi-Fi
void erase_wifi_credentials() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir NVS: %s", esp_err_to_name(err));
        return;
    }

    // Apaga as chaves "ssid" e "password"
    nvs_erase_key(handle, "ssid");
    nvs_erase_key(handle, "password");

    // Confirma as alterações
    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "Credenciais Wi-Fi apagadas com sucesso.");
}


// Salvar sensor_token
void save_sensor_token(const char *token)
{
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("sensor", NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(nvs_set_str(handle, "token", token));
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
    ESP_LOGI(TAG, "Sensor token salvo");
}

// Ler sensor_token
bool load_sensor_token(char *token, size_t token_len)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open("sensor", NVS_READONLY, &handle);
    if (err != ESP_OK) {ESP_LOGW(TAG, "Token não encontrado");return false;}

    if (nvs_get_str(handle, "token", token, &token_len) != ESP_OK) {
        ESP_LOGW(TAG, "Erro ao ler token: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    nvs_close(handle);
    return true;
}

// Apagar Wi-Fi
void erase_sensor_token() 
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("sensor", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir NVS: %s", esp_err_to_name(err));
        return;
    }
    // Apaga o "token"
    nvs_erase_key(handle, "token");

    // Confirma as alterações
    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "Sensor_Token apagado com sucesso.");
}
