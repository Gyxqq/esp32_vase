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
#include "lvgl.h"
#include "lvgl_helpers.h"
#include "esp_timer.h"
#include "esp_err.h"
SemaphoreHandle_t xGuiSemaphore;
static void lv_tick_task(void *arg)
{
    (void)arg;

    lv_tick_inc(portTICK_PERIOD_MS);
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
    disp_drv.flush_cb = st7735s_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    lv_obj_t *scr = lv_disp_get_scr_act(NULL);
    assert(scr != NULL);
    lv_obj_t *label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(label, "Hello World!");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, portTICK_PERIOD_MS * 1000));

    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }

    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
