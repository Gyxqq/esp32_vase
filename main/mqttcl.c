#ifndef MQTTCL_C
#define MQTTCL_C
#include "mqtt_client.h"
#include "static_value.c"
#include "init_wifi.c"
#include <nvs.h>
// #define TAG "MQTT"
#include "esp_log.h"
#include "cJSON.h"
#include "GUI_TASK/show_qrcode.c"
#include "light.c"
#include "water.c"
int command_handler(char *command);
void base_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = handler_args;
    // your code here3
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        printf("MQTT_EVENT_CONNECTED\n");
        esp_mqtt_client_subscribe(client, "send-command", 0);
        break;
    case MQTT_EVENT_DISCONNECTED:

        printf("MQTT_EVENT_DISCONNECTED\n");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        printf("MQTT_EVENT_SUBSCRIBED, msg_id=%d\n", event->msg_id);
        esp_mqtt_client_publish(client, "test", "data", 0, 0, 0);
        break;

    case MQTT_EVENT_DATA:
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        cJSON *root = cJSON_Parse(event->data);
        if (root == NULL)
        {
            ESP_EARLY_LOGE("MQTT", "json ROOT parse failed");
            break;
        }
        cJSON *uuid_json = cJSON_GetObjectItem(root, "plant_id");
        if (uuid_json == NULL)
        {
            ESP_EARLY_LOGE("MQTT", "json  parse failed");
            break;
        }
        if (strcmp(uuid_json->valuestring, uuid) != 0)
        {
            ESP_EARLY_LOGI("MQTT", "uuid not match");
            break;
        }
        cJSON *command_json = cJSON_GetObjectItem(root, "command");
        if (command_json == NULL)
        {
            ESP_EARLY_LOGE("MQTT", "json command parse failed");
            break;
        }
        command_handler(command_json->valuestring);

        break;

    default:
        break;
    }
}
void init_mqtt()
{
    asm("nop");

    nvs_handle_t nvs;
    nvs_open("storage", NVS_READWRITE, &nvs);
    size_t size = 0;
    esp_err_t err;
    err = nvs_get_str(nvs, "uuid", NULL, &size);
    if (err != ESP_OK || size == 0)
    {
        printf("uuid not found\ninit mqtt failed\n");
        return;
    }
    uuid = heap_caps_malloc(size, (1 << 3));
    err = nvs_get_str(nvs, "uuid", uuid, &size);
    if (err != ESP_OK)
    {
        printf("uuid not found\n");
        return;
    }
    nvs_close(nvs);
    printf("uuid: %s\n", uuid);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://jzitan.top:1883",
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, base_mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}
void destory_mqtt()
{
    esp_mqtt_client_destroy(client);
}
int command_handler(char *command)
{
    ESP_EARLY_LOGI("MQTT", "command: %s", command);
    if (strcmp(command, "reboot") == 0)
    {
        esp_restart();
    }
    else if (strcmp(command, "switch") == 0)
    {
        ESP_EARLY_LOGI("MQTT", "switch");

        if (gui % 2 == 1)
        {
            ESP_EARLY_LOGI("MQTT", "switch to sensor screen");
            screen_manager(SENSOR_SCREEN, 1);
        }
        else
        {
            ESP_EARLY_LOGI("MQTT", "switch to weather screen");
            screen_manager(WEAHTER_SCREEN, 1);
        }
        gui++;
    }
    else if (strcmp(command, "light-on") == 0)
    {
        ESP_EARLY_LOGI("MQTT", "LIGHT_ON");
        light_on();
    }
    else if (strcmp(command, "light-off") == 0)
    {
        ESP_EARLY_LOGI("MQTT", "LIGHT-OFF");
        light_off();
    }
    else if (strcmp("water", command) == 0)
    {
        ESP_EARLY_LOGI("MQTT", "WATER");
        xTaskCreate(water_on, "water_on", 2048, NULL, 10, NULL);
    }

    return 0;
}
#endif
