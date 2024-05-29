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

#include "mqtt_client.h"
#include "init_ble.c"
#include "GUI_TASK/show_qrcode.c"

void app_main(void)
{
    printf("Hello world!\n");
    init_wifi();
    init_ble();
    init_dsp();
    show_qrcode();
    vTaskDelay(pdMS_TO_TICKS(5000));
    deinit_ble();
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
    }
    printf("Restarting now.\n");
    fflush(stdout);

    esp_restart();
}
