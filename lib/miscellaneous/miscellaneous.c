#include "miscellaneous.h"
void get_device_mac(char *mac_str)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    sprintf(mac_str,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2],
        mac[3], mac[4], mac[5]);
}

void get_flash_size(uint32_t *flash_size)
{
    esp_flash_get_chip_size(NULL, flash_size); // NULL uses default SPI flash
    *flash_size =  *flash_size / (1024 * 1024); // Convert to MB
}