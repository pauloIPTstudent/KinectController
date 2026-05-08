#ifndef NVS_HELPER_H
#define NVS_HELPER_H
#include "esp_log.h"

bool load_wifi_credentials(char *ssid, size_t ssid_len, char *password, size_t pwd_len);
void save_wifi_credentials(const char *ssid, const char *password);
void erase_wifi_credentials();

void save_sensor_token(const char *token);
bool load_sensor_token(char *token, size_t token_len);
void erase_sensor_token() ;

#endif