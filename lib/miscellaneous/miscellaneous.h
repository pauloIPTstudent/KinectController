#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H
#include "esp_flash.h"
#include "esp_mac.h"

void get_device_mac(char *mac_str);
void get_flash_size(uint32_t *flash_size);

#endif