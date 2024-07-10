#ifndef STATIC_VALUE_C
#define STATIC_VALUE_C
#define SENSOR_SCREEN 0
#define WEAHTER_SCREEN 1
#include "mqtt_client.h"
esp_mqtt_client_handle_t client;
char * uuid =NULL;
SemaphoreHandle_t xGuiSemaphore;
static int gui=0;
#endif    