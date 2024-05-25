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
#include "lvgl_helpers.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "img.c"

SemaphoreHandle_t xGuiSemaphore;
static void lv_tick_task(void *arg)
{
    lv_tick_inc(portTICK_PERIOD_MS);
}

// LVGL任务
void guiTask(void *pvParameter)
{
    (void)pvParameter;
    while (1)
    {
        // 延迟10ms
        vTaskDelay(pdMS_TO_TICKS(10));

        // 尝试获取信号量并调用lvgl相关函数
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
}

// 初始化LVGL任务和相关资源
void initGuiTask()
{
    // 创建信号量
    xGuiSemaphore = xSemaphoreCreateMutex();

    // 定义定时器参数
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};

    // 创建定时器句柄
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, portTICK_PERIOD_MS * 1000));

    // 创建GUI任务
    xTaskCreate(guiTask, "guiTask", 4096, NULL, 5, NULL);
}
void app_main(void)
{
    xGuiSemaphore = xSemaphoreCreateMutex();
    printf("Hello world!\n");
    lv_init();
    lvgl_driver_init();
    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), (1 << 3));
    assert(buf1 != NULL);
    memset(buf1, 0, DISP_BUF_SIZE * sizeof(lv_color_t));
    lv_disp_buf_t disp_buf;
    lv_disp_buf_init(&disp_buf, buf1, NULL, DISP_BUF_SIZE);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    lv_obj_t *scr = lv_disp_get_scr_act(NULL);
    assert(scr != NULL);
    lv_obj_t *label = lv_label_create(scr, NULL);
    lv_label_set_text(label, "Hello World!");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
    initGuiTask();
    vTaskDelay(pdMS_TO_TICKS(5000));
    lv_obj_del(label);
    lv_obj_t *img = lv_img_create(scr, NULL);
    lv_img_set_src(img, &gImage_111);
    //颜色模式为rgb565

    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);
    //显示图片
    
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
