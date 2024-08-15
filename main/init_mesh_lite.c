#ifndef INIT_MESH_LITE
#define INIT_MESH_LITE
#include <inttypes.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include <sys/socket.h>

#include "esp_bridge.h"
#include "esp_mesh_lite.h"
#include "init_wifi.c"
#define CONFIG_BRIDGE_SOFTAP_SSID "ESP32_mesh"
#define CONFIG_BRIDGE_SOFTAP_PASSWORD "12345678"
#define PAYLOAD_LEN (1456)
int count_mesh = 0;
static void print_system_info_timercb(TimerHandle_t timer)
{
    uint8_t primary = 0;
    uint8_t sta_mac[6] = {0};
    wifi_ap_record_t ap_info = {0};
    wifi_second_chan_t second = 0;
    wifi_sta_list_t wifi_sta_list = {0x0};

    esp_wifi_sta_get_ap_info(&ap_info);
    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    esp_wifi_get_channel(&primary, &second);

    ESP_LOGI("mesh", "System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR ", parent rssi: %d, free heap: %" PRIu32 "", primary,
             esp_mesh_lite_get_level(), MAC2STR(sta_mac), MAC2STR(ap_info.bssid),
             (ap_info.rssi != 0 ? ap_info.rssi : -120), esp_get_free_heap_size());
    // #if CONFIG_MESH_LITE_MAXIMUM_NODE_NUMBER
    ESP_LOGI("mesh", "child node number: %d", esp_mesh_lite_get_child_node_number());
    // #endif /* MESH_LITE_NODE_INFO_REPORT */
    for (int i = 0; i < wifi_sta_list.num; i++)
    {
        ESP_LOGI("mesh", "Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }
}
static void ip_event_sta_got_ip_handler(void *arg, esp_event_base_t event_base,
                                        int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI("mesh", "IP_EVENT_STA_GOT_IP");
    ESP_LOGI("mesh", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
}

static void wifi_init(void)
{
    // nvs_flash_init();
    esp_err_t ret0 = nvs_flash_init();

    if (ret0 == ESP_ERR_NVS_NO_FREE_PAGES || ret0 == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // 如果NVS分区被占满，或者NVS库版本更高，需要执行全擦除
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret0 = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret0);
    nvs_handle nvs;
    ret0 = nvs_open("storage", NVS_READWRITE, &nvs);
    if (ret0 == ESP_OK)
    {
        printf("nvs open success\n");
    }
    else
    {
        printf("nvs open failed\n");
    }
    size_t len = 0;
    ret0 = nvs_get_str(nvs, "ssid", NULL, &len);
    if (ret0 == ESP_OK)
    {
        printf("ssid len: %d\n", len);
    }
    else
    {
        printf("ssid len failed\n");
        return;
    }
    char *ssid0 = (char *)malloc(len);
    ret0 = nvs_get_str(nvs, "ssid", ssid0, &len);
    if (ret0 == ESP_OK)
    {
        printf("ssid: %s\n", ssid0);
    }
    else
    {
        printf("ssid failed\n");
        return;
    }
    ret0 = nvs_get_str(nvs, "password", NULL, &len);
    if (ret0 == ESP_OK)
    {
        printf("password len: %d\n", len);
    }
    else
    {
        printf("password len failed\n");
        return;
    }
    char *password0 = (char *)malloc(len);
    ret0 = nvs_get_str(nvs, "password", password0, &len);
    if (ret0 == ESP_OK)
    {
        printf("password: %s\n", password0);
    }
    else
    {
        printf("password failed\n");
        return;
    }
    nvs_close(nvs);
    // Station
    wifi_config_t wifi_config;
    strncpy((char *)wifi_config.sta.ssid, ssid0, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    strncpy((char *)wifi_config.sta.password, password0, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';

    esp_bridge_wifi_set_config(WIFI_IF_STA, &wifi_config);

    // Softap
    snprintf((char *)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s", CONFIG_BRIDGE_SOFTAP_SSID);
    strlcpy((char *)wifi_config.ap.password, CONFIG_BRIDGE_SOFTAP_PASSWORD, sizeof(wifi_config.ap.password));
    esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_config);
}

void app_wifi_set_softap_info(void)
{
    char softap_ssid[32];
    uint8_t softap_mac[6];
    esp_wifi_get_mac(WIFI_IF_AP, softap_mac);
    memset(softap_ssid, 0x0, sizeof(softap_ssid));

#ifdef CONFIG_BRIDGE_SOFTAP_SSID_END_WITH_THE_MAC
    snprintf(softap_ssid, sizeof(softap_ssid), "%.25s_%02x%02x%02x", CONFIG_BRIDGE_SOFTAP_SSID, softap_mac[3], softap_mac[4], softap_mac[5]);
#else
    snprintf(softap_ssid, sizeof(softap_ssid), "%.32s", CONFIG_BRIDGE_SOFTAP_SSID);
#endif
    esp_mesh_lite_set_softap_ssid_to_nvs(softap_ssid);
    esp_mesh_lite_set_softap_psw_to_nvs(CONFIG_BRIDGE_SOFTAP_PASSWORD);
    esp_mesh_lite_set_softap_info(softap_ssid, CONFIG_BRIDGE_SOFTAP_PASSWORD);
}
int init_mesh_lite()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_bridge_create_all_netif();
    wifi_init();
    esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
    mesh_lite_config.max_connect_number = 30;
    mesh_lite_config.max_node_number = 30;
    mesh_lite_config.max_router_number = 2;
    mesh_lite_config.softap_ssid = "ESP32_mesh";
    mesh_lite_config.softap_password = "12345678";
    mesh_lite_config.join_mesh_ignore_router_status = true;
    mesh_lite_config.join_mesh_without_configured_wifi = true;
    esp_mesh_lite_init(&mesh_lite_config);
    app_wifi_set_softap_info();
    esp_mesh_lite_start();
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_sta_got_ip_handler, NULL, NULL));
    TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_PERIOD_MS,
                                       true, NULL, print_system_info_timercb);
    xTimerStart(timer, 0);
    return 0;
}
int wifi_swich()
{
    mesh_lite_sta_config_t config;
    if (count_mesh % 2 == 0)
    {
        esp_err_t ret0 = nvs_flash_init();

        if (ret0 == ESP_ERR_NVS_NO_FREE_PAGES || ret0 == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            // 如果NVS分区被占满，或者NVS库版本更高，需要执行全擦除
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret0 = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret0);
        nvs_handle nvs;
        ret0 = nvs_open("storage", NVS_READWRITE, &nvs);
        if (ret0 == ESP_OK)
        {
            printf("nvs open success\n");
        }
        else
        {
            printf("nvs open failed\n");
        }
        size_t len = 0;
        ret0 = nvs_get_str(nvs, "ssid", NULL, &len);
        if (ret0 == ESP_OK)
        {
            printf("ssid len: %d\n", len);
        }
        else
        {
            printf("ssid len failed\n");
            return -1 ;
        }
        char *ssid0 = (char *)malloc(len);
        ret0 = nvs_get_str(nvs, "ssid", ssid0, &len);
        if (ret0 == ESP_OK)
        {
            printf("ssid: %s\n", ssid0);
        }
        else
        {
            printf("ssid failed\n");
            return -1 ;
        }
        size_t len1 = 0;
        ret0 = nvs_get_str(nvs, "password", NULL, &len1);
        if (ret0 == ESP_OK)
        {
            printf("password len: %d\n", len1);
        }
        else
        {
            printf("password len failed\n");
            return -1 ;
        }
        char *password0 = (char *)malloc(len1);
        ret0 = nvs_get_str(nvs, "password", password0, &len1);
        if (ret0 == ESP_OK)
        {
            printf("password: %s\n", password0);
        }
        else
        {
            printf("password failed\n");
            return -1 ;
        }
        nvs_close(nvs);

        strncpy((char *)config.ssid, ssid0,len);
        strncpy((char *)config.password, password0, len1);
    }
    else
    {
        strncpy((char *)config.ssid, "ESP32_mesh", sizeof(config.ssid) - 1);
        strncpy((char *)config.password, "12345678", sizeof(config.password) - 1);
    }
    config.bssid_set = 0;
    count_mesh++;
    esp_mesh_lite_set_router_config(&config);
    esp_mesh_lite_connect();
    return 0;
}

#endif
