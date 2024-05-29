#include "lvgl_helpers.h"
#include "../QRCode/qrcode.c"
#include "esp_wifi.h"
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
}
void init_dsp()
{
    xGuiSemaphore = xSemaphoreCreateMutex();
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
    initGuiTask();
    printf("GUI Task has been created\n");
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
    uint8_t *qrcodeData = heap_caps_malloc(qrcode_getBufferSize(15), (1 << 3));
    assert(qrcodeData != NULL);
    // chipinfo获取mac地址
    printf("malloc qrcodeData\n");
    uint8_t mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    printf("MAC %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    mac[5] += 2;
    // 生成二维码
    char macStr[22];

    UBaseType_t uxHighWaterMark;
    /* Inspect our own high water mark on entering the task. */
    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    printf("Watermark: %d\n", uxHighWaterMark);

    sprintf(macStr, "MAC %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    qrcode_initText(qrcode, qrcodeData, 15, ECC_HIGH, macStr);
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
    // 创建标签
    lv_obj_t *scan_qrcode_label = lv_label_create(scr, NULL);
    lv_label_set_text(scan_qrcode_label, "Scan QR Code\n connect to WiFi");
    lv_obj_align(scan_qrcode_label, canvas, LV_ALIGN_IN_TOP_MID, 0, -35);
}