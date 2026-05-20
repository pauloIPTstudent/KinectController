#ifndef OTA_HELPER_H
#define OTA_HELPER_H
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_partition.h"
#include "esp_log.h"

esp_err_t ota_helper_switch_and_reboot(
    esp_partition_type_t target_type, 
    esp_partition_subtype_t target_subtype,
    const char *label);

#endif