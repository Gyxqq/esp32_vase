#ifndef __HTTP_FUNC_C__
#define __HTTP_FUNC_C__
#include "esp_http_client.h"
#include "esp_log.h"
#include "static_value.c"
#include "esp_tls.h"
// HTTP事件处理器
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI("HTTP", "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI("HTTP", "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_DATA, len=%d", (int)evt->data_len);
        // if (!esp_http_client_is_chunked_response(evt->client))
        // {
        //     // 复制数据到缓冲区
        //     memcpy(http_data_buffer, evt->data, evt->data_len);
        //     http_data_len = evt->data_len;
        // }
        // else
        // {
        //     // 复制数据到缓冲区
        //     memcpy(http_data_buffer + http_data_len, evt->data, evt->data_len);
        //     http_data_len += evt->data_len;
        // }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI("HTTP", "HTTP_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
    return ESP_OK;
}
esp_err_t http_get_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI("HTTP", "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI("HTTP", "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_DATA, len=%d", (int)evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // 复制数据到缓冲区
            memcpy(http_data_buffer, evt->data, evt->data_len);
            http_data_len = evt->data_len;
        }
        else
        {
            // 复制数据到缓冲区
            memcpy(http_data_buffer + http_data_len, evt->data, evt->data_len);
            http_data_len += evt->data_len;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI("HTTP", "HTTP_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
    return ESP_OK;
}
esp_err_t init_http_get_client(char *url)
{
    // 初始化HTTP客户端
    http_data_buffer = (char *)malloc(8192);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_get_event_handler,
        .method = HTTP_METHOD_GET,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI("HTTP", "HTTP GET Status = %d, content_length = %d",
                 (int)esp_http_client_get_status_code(client),
                 (int)esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE("HTTP", "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}
esp_err_t init_http_post_client(char *url, char *post_data, int data_len)
{
    // 初始化HTTP客户端
    http_data_buffer = (char *)malloc(8192);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_open(client, data_len);
    if (err != ESP_OK)
    {
        ESP_LOGE("HTTP", "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    }
    else
    {
        err = esp_http_client_write(client, post_data, data_len);
        if (err == -1)
        {
            ESP_LOGE("HTTP", "Failed to write data to HTTP connection: %s", esp_err_to_name(err));
            esp_http_client_cleanup(client);
        }
        else
        {
            err = esp_http_client_fetch_headers(client);
            if (err == 0)
            {
                ESP_LOGE("HTTP", "Failed to fetch headers: %s", esp_err_to_name(err));
                esp_http_client_cleanup(client);
            }
            else
            {

                ESP_LOGI("HTTP", "HTTP POST Status = %d, content_length = %d",
                         (int)esp_http_client_get_status_code(client),
                         (int)esp_http_client_get_content_length(client));
                err = esp_http_client_read(client, http_data_buffer, esp_http_client_get_content_length(client));
                http_data_len = err;
                if (err == -1)
                {
                    ESP_LOGE("HTTP", "Failed to read data from HTTP connection: %s", esp_err_to_name(err));
                    esp_http_client_cleanup(client);
                }
                else
                {
                    ESP_LOGI("HTTP", "HTTP POST read data from HTTP connection: %s", http_data_buffer);
                    esp_http_client_cleanup(client);
                }
            }
        }
    }

    return err;
}

int destroy_http_client()
{
    free(http_data_buffer);
    http_data_len = 0;
    return 0;
}

#endif