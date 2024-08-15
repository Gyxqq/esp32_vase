#ifndef __INIT_WIFI__
#define __INIT_WIFI__
#include "esp_flash.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/ip_addr.h"
char *ssid_;
char *password_;
static int wifi_connected = 0;
void init_wifi(void)
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
    ssid_ = ssid0;
    password_ = password0;

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(sta_netif, "esp32");

    esp_netif_dns_info_t dns_info;
    IP4_ADDR(&dns_info.ip.u_addr.ip4, 8, 8, 8, 8); // Google 的公共 DNS 服务器 8.8.8.8
    dns_info.ip.type = IPADDR_TYPE_V4;
    esp_netif_set_dns_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), ESP_NETIF_DNS_MAIN, &dns_info);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK)
    {
        printf("esp_wifi_init failed: %d\n", ret);
        return;
    }
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };
    strcpy((char *)wifi_config.sta.ssid, ssid0);
    strcpy((char *)wifi_config.sta.password, password0);

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    esp_err_t err0 = esp_wifi_start();
    if (err0 == ESP_OK)
    {
        printf("wifi start success\n");
    }
    else
    {
        printf("wifi start failed\n");
        printf("Error: %s\n", esp_err_to_name(err0));
    }

    esp_err_t err = esp_wifi_connect();
    if (err == ESP_OK)
    {
        printf("wifi connect success\n");
    }
    else
    {
        printf("wifi connect failed\n");
        printf("Error: %s\n", esp_err_to_name(err));
    }
}
// int wifi_swich()
// {
//     esp_wifi_stop();
//     wifi_config_t wifi_config = {
//         .sta = {
//             .ssid = "",
//             .password = "",
//             .threshold.authmode = WIFI_AUTH_WPA2_PSK,
//             .pmf_cfg = {
//                 .capable = true,
//                 .required = false,
//             },
//         },
//     };
//     if (wifi_connected == 0)
//     {
//         strcpy((char *)wifi_config.sta.ssid, "TP-LINK_2.4GHz_1A4A");
//         strcpy((char *)wifi_config.sta.password, "12345678");
//         wifi_connected = 1;
//     }
//     strcpy((char *)wifi_config.sta.ssid, ssid_);
//     strcpy((char *)wifi_config.sta.password, password_);

//     esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
//     esp_wifi_start();
// }
#endif