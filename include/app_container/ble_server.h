#ifndef BLE_SERVER_H
#define BLE_SERVER_H
void init_ble(void);
void ble_host_task(void *param);
void ble_notificar_queda(void); 
#endif