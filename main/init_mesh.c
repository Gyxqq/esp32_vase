#include "mesh.h"
#include "esp_wifi.h"
#define MESH_ID 13
static void ip_event_handler1(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event;
    printf("ev_handle_called.\n");
    switch (event_id)
    {
    case IP_EVENT_STA_GOT_IP:
        event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI("mesh", "IP_EVENT_STA_GOT_IP");
        ESP_LOGI("mesh", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        break;
    default:
        break;
    }

    return;
}
int init_mesh()
{
    ESP_ERROR_CHECK(esp_netif_init());

    /* 事件初始化 */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Wi-Fi 初始化 */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    /* 注册 IP 事件处理程序 */

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler1, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());
    /* Mesh 初始化 */
    ESP_ERROR_CHECK(esp_mesh_init());
    /* 注册 mesh 事件处理程序 */
    /* 默认启用 MESH IE 加密 */
mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
/* Mesh ID */
memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
/* 信道（需与路由器信道匹配）*/
cfg.channel = CONFIG_MESH_CHANNEL;
/* 路由器 */
cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
memcpy((uint8_t *) &cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
memcpy((uint8_t *) &cfg.router.password, CONFIG_MESH_ROUTER_PASSWD,
    strlen(CONFIG_MESH_ROUTER_PASSWD));
/* Mesh softAP */
cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD,
    strlen(CONFIG_MESH_AP_PASSWD));
ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
}