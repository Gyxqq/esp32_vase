#ifndef SEEECH_RECOGNITION
#define SEEECH_RECOGNITION
#include <esp_log.h>
#include <driver/i2s.h>
#include <driver/i2s_common.h>
int init_i2s()
{
    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX, // 设置i2s工作模式，根据需求设置
        .sample_rate = 115200,                               // 设置I2S 采样率，根据音频确定采样率
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,        // 设置采样位数
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,        // 设置I2S 通道格式（分离左右声道）
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,   // 设置I2S 通讯格式
        .tx_desc_auto_clear = true,                          // I2S 自动清除tx描述符
#if SOC_I2S_SUPPORTS_TDM
        .total_chan = 2,
        .chan_mask = I2S_TDM_ACTIVE_CH0 | I2S_TDM_ACTIVE_CH1,
        .left_align = false,
        .big_edin = false,
        .bit_order_msb = false,
        .skip_msk = false,
#endif
        .dma_desc_num = 8,                        // I2S DMA 用于接收/发送数据的描述符总数
        .dma_frame_num = 64,                      // 一次性采样的帧数。这里的 frame 表示一个 WS 周期内所有通道的总数据
        .use_apll = false,                        // I2S 使用 APLL 作为主要 I2S 时钟，使其能够获得准确的时钟
        .mclk_multiple = 256,                     // I2S 主时钟（MCLK）与采样率的倍数，有256（默认） 128 384倍
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2, // 用于分配中断的标志
    };
    if (i2s_driver_install(0, &i2s_cfg, 0, NULL) != ESP_OK)
    {
        ESP_LOGE("I2S", "install i2s driver failed");
        return ESP_FAIL;
    }

    i2s_pin_config_t i2s_pin_cfg = {
        .mck_io_num = GPIO_NUM_3,
        .bck_io_num = GPIO_NUM_21,
        .ws_io_num = GPIO_NUM_17,
        // .data_out_num =GPIO_NUM_3,
        .data_in_num = GPIO_NUM_2};
    if (i2s_set_pin(0, &i2s_pin_cfg) != ESP_OK)
    {
        ESP_LOGE("I2S", "set i2s pin failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void i2s_read_task(void *arg)
{
    size_t bytes_read;
    int16_t *data = (int16_t *)malloc(1024 * sizeof(int16_t));
    while (1)
    {
        i2s_read(0, data, 1024 * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        for (int i = 0; i < 1024; i++)
        {
            ESP_EARLY_LOGI("I2S", "%d ", data[i]);
        }
        ESP_EARLY_LOGI("I2S", "\n");
    }
    free(data);
}

#endif