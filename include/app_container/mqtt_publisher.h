#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H
typedef enum {
    EVENT_0 = 0, 
    EVENT_1 = 1,
    EVENT_2 = 2,
    EVENT_3 = 3,
    EVENT_4 = 4,
    EVENT_5 = 5,
    EVENT_6 = 6,
    EVENT_7 = 7,
    EVENT_8 = 8,
    EVENT_9 = 9
} mqtt_acoes_t;
void mqtt_publisher_task(void *pvParameters);
#endif