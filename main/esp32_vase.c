/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "../build/config/sdkconfig.h"
// #include "lvgl.h"

#include "esp_timer.h"
#include "esp_err.h"
// #include "img.c"

#include <inttypes.h>

#include "esp_chip_info.h"
#include "init_wifi.c"
#include "sensor.c"
#include "mqttcl.c"
#include "init_ble.c"
#include "GUI_TASK/show_qrcode.c"
#include "esp_sleep.h"
#include "esp_log.h"
#include "light.c"
#include "water.c"
#include "init_mesh.c"
#include "config.h"
#include "speech_recognition.c"
int load_uuid();
int read_temp_humi_lux_uploader();
int check_wifi_and_set_icon();
int gui_interupt_init();
static void gpio_isr_handler(void *arg)
{
    gui++;
    ESP_EARLY_LOGI("ISR", "GPIO[%d] intr, val: %d gui:%d", GPIO_NUM_0, gpio_get_level(GPIO_NUM_0), gui);

    if (gui % 2 == 1)
    {
        screen_manager(SENSOR_SCREEN, 1);
    }
    else
    {
        screen_manager(WEAHTER_SCREEN, 1);
    }
}
void app_main(void)
{

    init_i2c();
    xGuiSemaphore = xSemaphoreCreateMutex();
    printf("Hello world!\n");
    esp_log_level_set("*", ESP_LOG_DEBUG);
#ifdef USE_MESH
    init_mesh();
#else
    init_wifi();
#endif
    // init_ble();
    // vTaskDelay(pdMS_TO_TICKS(20000));
    lv_init();
    lvgl_driver_init();
    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), (1 << 3));
    assert(buf1 != NULL);
    // memset(buf1, 0, DISP_BUF_SIZE * sizeof(lv_color_t));
    lv_disp_buf_t disp_buf;
    lv_disp_buf_init(&disp_buf, buf1, NULL, DISP_BUF_SIZE);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);
    // init_dsp();
    initGuiTask();
    printf("GUI Task has been created\n");

    // show_qrcode();
    // vTaskDelay(pdMS_TO_TICKS(5000));
    // 检查wifi连接状态
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret != ESP_OK)
    {

        printf("wifi not connected\n");
        show_qrcode();
        init_ble();
        // esp_mesh_stop();
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            wifi_ap_record_t ap_info;
            esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
            if (ret == ESP_OK)
            {
                deinit_ble();
                lv_obj_del(qrscr);
                free(img_buf);
                vTaskDelay(pdMS_TO_TICKS(2000));
                // esp_mesh_stop();

                break;
            }
        }
    }
    init_mqtt();
    load_uuid();
    screen_manager(SENSOR_SCREEN, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    int ret1 = screen_manager(WEAHTER_SCREEN, 0);
    if (ret1 == -1)
    {
        ESP_EARLY_LOGE("main", "weather screen init failed");
    }

#ifdef USE_LIGHT
    init_light_gpio();
#endif
#ifdef USE_WATER
    init_water();
#endif
#ifdef USE_SPEECH_RECOGNITION
    init_i2s();
    ESP_EARLY_LOGI("main", "init i2s success");
    xTaskCreate(i2s_read_task, "speech_recognition_task", 8192, NULL, 10, NULL);
    ESP_EARLY_LOGI("main", "create i2s task success");
#endif
#ifdef USE_SPEECH_RECOGNITION_SR
    ESP_EARLY_LOGI("main", "init speech recognition");
    init_model();
    ESP_EARLY_LOGI("main", "init model success");
#endif

    gui_interupt_init();
    int_fast64_t counter = 0;
    while (1) // 存放一些定时任务
    {
        if (counter % 5 == 0)
        { // 获取当前剩余堆内存
            size_t freeHeapSize = xPortGetFreeHeapSize();
            // 获取自系统启动以来最小的剩余堆内存
            size_t minEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();
            // 打印内存信息
            printf("Free heap size: %d, Minimum free heap size: %d\n", freeHeapSize, minEverFreeHeapSize);
            // 获取信号量当前计数
            UBaseType_t uxSemaphoreCount = uxSemaphoreGetCount(xGuiSemaphore);
            // 打印信号量信息
            printf("Semaphore count: %d\n", uxSemaphoreCount);
        }
        if (counter % 20 == 0)
        {
#ifdef USE_SPEECH_RECOGNITION
            read_temp_humi_lux_uploader();
#endif
        }
        if (counter % 60 == 0)
        {
            check_wifi_and_set_icon();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
        counter++;
        if (counter == 0xffffffff)
        {
            counter = 0;
        }
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
int read_temp_humi_lux_uploader()
{
    struct temp_hum_data data = read_sensor();
    if (data.temp == -1)
    {
        printf("read sensor failed\n");
        return -1;
    }
    float lux = get_light();
    char *topic = "get-history";
    char *payload = malloc(200);
    if (uuid == NULL)
    {
        uuid = "123456";
    }
    assert(payload != NULL);
    sprintf(payload, "{\n\"msg\": \"SUCCESS\",\n\"plant_id\": \"%s\",\n\"temperature\": %.2f,\n\"humidity\": %.2f,\n\"luminance\": %.2f\n}", uuid, data.temp, data.hum, lux);
    esp_mqtt_client_publish(client, topic, payload, 0, 0, 0);
    free(payload);
    screen_sensor_update(data.temp, data.hum, lux);
    return 0;
}

int load_uuid()
{
    nvs_flash_init();
    esp_err_t ret0 = nvs_flash_init();

    if (ret0 == ESP_ERR_NVS_NO_FREE_PAGES || ret0 == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // 如果NVS分区被占满，或者NVS库版本更高，需要执行全擦除
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret0 = nvs_flash_init();
    }
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
    ret0 = nvs_get_str(nvs, "uuid", NULL, &len);
    if (ret0 == ESP_OK)
    {
        printf("uuid len: %d\n", len);
    }
    else
    {
        printf("uuid len failed\n");
        return -1;
    }
    uuid = (char *)malloc(len);
    ret0 = nvs_get_str(nvs, "uuid", uuid, &len);
    if (ret0 == ESP_OK)
    {
        printf("uuid: %s\n", uuid);
    }
    else
    {
        printf("uuid init failed\n");
        return -1;
    }
    nvs_close(nvs);
    return 0;
}

int check_wifi_and_set_icon()
{
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret != ESP_OK)
    {
        printf("wifi not connected\n");
        ret = esp_wifi_connect();
        if (ret != ESP_OK)
        {
            printf("wifi reconnect failed\n");

            if (pdTRUE == xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(1)))
            {
                lv_obj_set_hidden(screen_sensor.wifi_label, true);

                xSemaphoreGive(xGuiSemaphore);
            }
        }
        else
        {
            printf("wifi reconnect success\n");
            if (pdTRUE == xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(1)))
            {
                lv_obj_set_hidden(screen_sensor.wifi_label, false);

                xSemaphoreGive(xGuiSemaphore);
            }
        }

        return -1;
    }
    else
    {
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(1)))
        {
            lv_obj_set_hidden(screen_sensor.wifi_label, false);

            xSemaphoreGive(xGuiSemaphore);
        }

        return 0;
    }
    return 0;
}
int gui_interupt_init()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pull_up_en = 1; // 设置上拉
    io_conf.pull_down_en = 0;
    io_conf.pin_bit_mask = 1ULL << GPIO_NUM_0;
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);
    // gpio_set_intr_type(GPIO_NUM_0, GPIO_INTR_NEGEDGE);
    // esp_err_t ret = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL2);
    // if (ret != ESP_OK)
    // {
    //     // 处理错误
    // }
    // // 注册中断处理程序
    // gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, NULL);
    // // esp_intr_alloc(GPIO_NUM_0, ESP_INTR_FLAG_LEVEL1, gpio_isr_handler, NULL, NULL);

    esp_err_t ret = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
    if (ret != ESP_OK)
    {
        printf("Failed to install GPIO ISR service: %s\n", esp_err_to_name(ret));
        return -1;
    }
    gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, NULL);
    ESP_EARLY_LOGI("ISR", "install gpio isr service success");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_intr_dump(stderr);

    return 0;
}