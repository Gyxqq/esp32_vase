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
void app_main(void)
{
    init_i2c();

    xGuiSemaphore = xSemaphoreCreateMutex();
    printf("Hello world!\n");
    init_wifi();
    // init_ble();
    vTaskDelay(pdMS_TO_TICKS(1000));

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
    vTaskDelay(pdMS_TO_TICKS(5000));
    // 检查wifi连接状态
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret != ESP_OK)
    {

        printf("wifi not connected\n");
        show_qrcode();
        init_ble();
    }
    init_mqtt();

    while (1)
    {
        // 获取当前剩余堆内存
        size_t freeHeapSize = xPortGetFreeHeapSize();
        // 获取自系统启动以来最小的剩余堆内存
        size_t minEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();
        // 打印内存信息
        printf("Free heap size: %d, Minimum free heap size: %d\n", freeHeapSize, minEverFreeHeapSize);
        // 延迟5秒钟
        vTaskDelay(pdMS_TO_TICKS(5000));
        // read_sensor();
    }
    printf("Restarting now.\n");
    fflush(stdout);

    esp_restart();
}
