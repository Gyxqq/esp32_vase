#ifndef GUI_TASK_C
#define GUI_TASK_C
#include "lvgl_helpers.h"
#include "../QRCode/qrcode.c"
#include "esp_wifi.h"
#include "../static_value.c"
int screen_manager(int screen_index, int opt);
int screen_sensor_init();
int screen_sensor_update(float temp, float humi);
int screen_sensor_destroy();
struct screen_sensor
{
    lv_obj_t *sensor_screen;
    lv_obj_t *temp_label;
    lv_obj_t *humi_label;
    lv_obj_t *wifi_label;
} screen_sensor;
static void lv_tick_task(void *arg)
{
    lv_tick_inc(portTICK_PERIOD_MS);
}

// LVGL任务
void guiTask(void *pvParameter)
{
    (void)pvParameter;

    UBaseType_t uxHighWaterMark;
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    printf("Watermark: %d\n", uxHighWaterMark);

    while (1)
    {
        // 延迟10ms

        vTaskDelay(pdMS_TO_TICKS(30));

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
    // 尝试获取信号量并调用lvgl相关函数
    if (pdTRUE == xSemaphoreTake(xGuiSemaphore,pdMS_TO_TICKS(1000)))
    {

        // 定义定时器参数
        const esp_timer_create_args_t periodic_timer_args = {
            .callback = &lv_tick_task,
            .name = "periodic_gui"};

        // 创建定时器句柄
        esp_timer_handle_t periodic_timer;
        ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, portTICK_PERIOD_MS * 1000));

        // 创建GUI任务
        xTaskCreate(guiTask, "guiTask", 2048, NULL, 5, NULL);
        xSemaphoreGive(xGuiSemaphore);
    }
}
void init_dsp()
{

    ;
}

void show_qrcode()
{

    printf("Hello world!\n");

    lv_obj_t *scr = lv_disp_get_scr_act(NULL);
    if (scr == NULL)
    {
        // 创建一个新的屏幕
        scr = lv_obj_create(NULL, NULL);
    }

    assert(scr != NULL);
    lv_obj_t *label = lv_label_create(scr, NULL);
    lv_label_set_text(label, "Hello World!");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_del(label);
    printf("del label\n");
    QRCode *qrcode = heap_caps_malloc(sizeof(QRCode), (1 << 3));
    uint8_t *qrcodeData = heap_caps_malloc(qrcode_getBufferSize(10), (1 << 3));
    if (qrcode == NULL || qrcodeData == NULL)
    {
        printf("malloc failed\n");
        return;
    }
    // chipinfo获取mac地址
    printf("malloc qrcodeData\n");
    uint8_t mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    printf("MAC %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    mac[5] += 2;
    // 生成二维码
    char macStr[22];

    sprintf(macStr, "MAC %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    qrcode_initText(qrcode, qrcodeData, 10, ECC_HIGH, macStr);
    // 按像素绘制二维码
    assert(qrcode != NULL);
    printf("size: %d\n", qrcode->size);
    lv_obj_t *canvas = lv_canvas_create(scr, NULL);
    uint8_t color_depth = lv_img_cf_get_px_size(LV_IMG_CF_TRUE_COLOR);
    printf("color_depth: %d\n", color_depth);
    // 创建缓冲区
    lv_color_t *buf = heap_caps_malloc(qrcode->size * qrcode->size * color_depth / 8, (1 << 3));
    assert(buf != NULL);
    // 8位到16位rgb565
    for (int x = 0; x < qrcode->size; x++)
    {
        for (int y = 0; y < qrcode->size; y++)
        {
            if (qrcode_getModule(qrcode, x, y))
            {
                buf[x * qrcode->size + y] = LV_COLOR_BLACK;
            }
            else
            {
                buf[x * qrcode->size + y] = LV_COLOR_WHITE;
            }
        }
    }
    // 释放二维码数据
    free(qrcodeData);

    // 设置画布
    // 设置画布位置

    lv_canvas_set_buffer(canvas, buf, qrcode->size, qrcode->size, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, NULL, LV_ALIGN_CENTER, 0, 20);
    // 释放缓冲区
    free(buf);
    free(qrcode);
    // 创建标签
    lv_obj_t *scan_qrcode_label = lv_label_create(scr, NULL);
    lv_label_set_text(scan_qrcode_label, "    Scan QR Code\n  connect to WiFi");
    lv_obj_align(scan_qrcode_label, canvas, LV_ALIGN_IN_TOP_MID, -25, -35);
    // 设置字体大小
    lv_obj_set_style_local_text_font(scan_qrcode_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_theme_get_font_title());
    lv_obj_t *ble_icon = lv_label_create(scr, NULL);
    lv_label_set_text(ble_icon, LV_SYMBOL_BLUETOOTH);
    // 放右上角
    lv_obj_align(ble_icon, NULL, LV_ALIGN_IN_TOP_RIGHT, -5, 5);
}
int screen_manager(int screen_index, int opt) // 0: init 1:destroy
{

    if (screen_index == SENSOR_SCREEN)
    {
        // 尝试获取信号量并调用lvgl相关函数
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(1)))
        {

            if (opt == 0)
            {
                printf("sensor screen\n");
                lv_obj_t *scr = lv_disp_get_scr_act(NULL);
                if (scr != NULL)
                {
                    printf("del scr\n");
                    lv_obj_del(scr);
                }
                screen_sensor.sensor_screen = lv_obj_create(NULL, NULL);
                lv_disp_load_scr(screen_sensor.sensor_screen);
                assert(screen_sensor.sensor_screen != NULL);
                xSemaphoreGive(xGuiSemaphore);
                return screen_sensor_init();
            }
            else if (opt == 1)
            {
                xSemaphoreGive(xGuiSemaphore);
                return screen_sensor_destroy();
            }
            xSemaphoreGive(xGuiSemaphore);
        }
    }
    return -1;
}

int screen_sensor_init()
{ // 尝试获取信号量并调用lvgl相关函数
    lv_obj_t *bar = NULL;
    if (pdTRUE == xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(1)))
    {
        bar = lv_bar_create(screen_sensor.sensor_screen, NULL);
        lv_bar_set_anim_time(bar, 2000);
        lv_obj_set_size(bar, 80, 20);
        lv_obj_align(bar, screen_sensor.sensor_screen, LV_ALIGN_CENTER, 0, 0);
        lv_bar_set_value(bar, 100, LV_ANIM_ON);
        // 设置动画时间
        xSemaphoreGive(xGuiSemaphore);
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    if (pdTRUE == xSemaphoreTake(xGuiSemaphore,pdMS_TO_TICKS(1)))
    {
        lv_obj_del(bar);
        screen_sensor.temp_label = lv_label_create(screen_sensor.sensor_screen, NULL);
        screen_sensor.humi_label = lv_label_create(screen_sensor.sensor_screen, NULL);
        screen_sensor.wifi_label = lv_label_create(screen_sensor.sensor_screen, NULL);
        lv_obj_set_style_local_text_font(screen_sensor.temp_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_theme_get_font_title());
        lv_obj_set_style_local_text_font(screen_sensor.humi_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_theme_get_font_title());
        lv_obj_align(screen_sensor.temp_label, screen_sensor.sensor_screen, LV_ALIGN_CENTER, -30, -30);
        lv_obj_align(screen_sensor.humi_label, screen_sensor.sensor_screen, LV_ALIGN_CENTER, -30, 0);
        lv_obj_align(screen_sensor.wifi_label, screen_sensor.sensor_screen, LV_ALIGN_OUT_TOP_RIGHT, -10, 15);
        lv_label_set_text(screen_sensor.temp_label, "Temp: 0.0");
        lv_label_set_text(screen_sensor.humi_label, "Humi: 0.0");
        lv_label_set_text(screen_sensor.wifi_label, LV_SYMBOL_WIFI);
        xSemaphoreGive(xGuiSemaphore);
    }

    return 0;
}
int screen_sensor_destroy()
{
    lv_obj_del(screen_sensor.sensor_screen);
    return 0;
}
int screen_sensor_update(float temp, float humi)
{
    char temp_str[20];
    char humi_str[20];
    snprintf(temp_str, sizeof(temp_str), "Temp: %.2f", temp);
    snprintf(humi_str, sizeof(humi_str), "Humi: %.2f", humi);
    // 尝试获取信号量并调用lvgl相关函数
    if (pdTRUE == xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(1)))
    {
        lv_label_set_text(screen_sensor.temp_label, temp_str);
        lv_label_set_text(screen_sensor.humi_label, humi_str);
        xSemaphoreGive(xGuiSemaphore);
    }

    return 0;
}

#endif