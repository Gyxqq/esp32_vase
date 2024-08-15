#ifndef __init_mesh__
#define __init_mesh__
#include "mesh.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_mesh.h"
#define CONFIG_MESH_CHANNEL 2
static const uint8_t MESH_ID[6] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x76};
#define CONFIG_MESH_ROUTER_SSID "ESP32_mesh"
#define CONFIG_MESH_ROUTER_PASSWD "12345678"
#define CONFIG_MESH_AP_PASSWD "12345678"
#define CONFIG_MESH_AP_CONNECTIONS 4
#define MESH_TAG "MESH"
esp_netif_t *sta_netif;
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
void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    mesh_addr_t id = {
        0,
    };
    static uint16_t last_layer = 0;
    uint16_t mesh_layer = 0;
    mesh_addr_t mesh_parent_addr;
    static bool is_mesh_connected = false;
    switch (event_id)
    {
    case MESH_EVENT_NO_PARENT_FOUND:
    {
        mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 no_parent->scan_times);
    }
    /* TODO handler for the failure */
    break;
    /* 已连接到父节点事件 */
    case MESH_EVENT_PARENT_CONNECTED:
    {
        mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
        esp_mesh_get_id(&id);
        mesh_layer = connected->self_layer;
        memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
        
        last_layer = mesh_layer;
        /* 已连接到父节点状态指示 */
        // mesh_connected_indicator(mesh_layer);
        is_mesh_connected = true;
        /* 判断该设备是否为Mesh网络中的根节点 */
        // if (esp_mesh_is_root())
        // {
            /* 开启DHCP客户端获取IP */
        esp_netif_dhcpc_start(sta_netif);
        // }
        
    }
    break;
    /* 与父节点断开连接事件 */
    case MESH_EVENT_PARENT_DISCONNECTED:
    {
        mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 disconnected->reason);
        is_mesh_connected = false;
        // mesh_disconnected_indicator();
        mesh_layer = esp_mesh_get_layer();
    }
    break;

    default:
        break;
    }
}
int init_mesh()
{
    nvs_flash_init();
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
        return -1;
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
        return -1;
    }
    ret0 = nvs_get_str(nvs, "password", NULL, &len);
    if (ret0 == ESP_OK)
    {
        printf("password len: %d\n", len);
    }
    else
    {
        printf("password len failed\n");
        return -1;
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
        return -1;
    }
    nvs_close(nvs);

    ESP_ERROR_CHECK(esp_netif_init());

    /* 事件初始化 */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    /* Wi-Fi 初始化 */
    // sta_netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&sta_netif, NULL));
    // esp_netif_create_default_wifi_
    esp_netif_set_hostname(sta_netif, "esp32");
    esp_netif_dns_info_t dns_info;
    IP4_ADDR(&dns_info.ip.u_addr.ip4, 8, 8, 8, 8); // Google 的公共 DNS 服务器 8.8.8.8
    dns_info.ip.type = IPADDR_TYPE_V4;
    esp_netif_set_dns_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), ESP_NETIF_DNS_MAIN, &dns_info);

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    /* 注册 IP 事件处理程序 */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler1, NULL));
    // 注册 mesh 事件处理程序
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    // ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());
    /* Mesh 初始化 */
    ESP_ERROR_CHECK(esp_mesh_init());
    /* 注册 mesh 事件处理程序 */
    /* 默认启用 MESH IE 加密 */
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    /* Mesh ID */

    memcpy((uint8_t *)&cfg.mesh_id, MESH_ID, 6);
    /* 信道（需与路由器信道匹配）*/
    cfg.channel = CONFIG_MESH_CHANNEL;
    cfg.allow_channel_switch = true;
    /* 路由器 */
    cfg.router.ssid_len = strlen(ssid0);
    memcpy((uint8_t *)&cfg.router.ssid, ssid0, cfg.router.ssid_len);
    memcpy((uint8_t *)&cfg.router.password, password0, strlen(password0));
    /* Mesh softAP */
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *)&cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD,
           strlen(CONFIG_MESH_AP_PASSWD));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    ESP_ERROR_CHECK(esp_mesh_start());
    return 0;
}
#endif