#ifndef SEEECH_RECOGNITION
#define SEEECH_RECOGNITION
#include <esp_log.h>
#include <driver/i2s.h>
#include <driver/i2s_common.h>
#include <stdint.h>
#include <math.h>
#include <esp_mn_models.h>
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_process_sdkconfig.h"
// #include "esp_board_init.h"
// #include "speech_commands_action.h"
#include "model_path.h"
void process_audio(uint8_t *pcm_data, int num_samples, int sr, int16_t *mel_spectrum);
void init_model();
int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;
static volatile int task_flag = 0;
srmodel_list_t *models = NULL;
static int play_voice = -2;
int init_i2s()
{
    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,                   // 设置i2s工作模式，根据需求设置
        .sample_rate = 16000,                                                  // 设置I2S 采样率，根据音频确定采样率
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                          // 设置采样位数
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,                           // 设置I2S 通道格式（分离左右声道）
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB, // 设置I2S 通讯格式
        .tx_desc_auto_clear = true,                                            // I2S 自动清除tx描述符
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
        // .mck_io_num = GPIO_NUM_3,
        // .bck_io_num = GPIO_NUM_21,
        .bck_io_num = GPIO_NUM_3,
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
    uint32_t *data = (uint32_t *)malloc(8000 * sizeof(uint32_t));
    while (1)
    {
        i2s_read(0, data, 8000 * sizeof(uint32_t), &bytes_read, portMAX_DELAY);
    }
    free(data);
}
void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int feed_channel = 1;
    assert(nch <= feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (task_flag)
    {
        // esp_get_feed_data(false, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);
        i2s_read(0, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel, NULL, portMAX_DELAY);
        ESP_EARLY_LOGI("SR", "I2S READ DATA");
        afe_handle->feed(afe_data, i2s_buff);
    }
    if (i2s_buff)
    {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}
void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE);
    printf("multinet:%s\n", mn_name);
    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    model_iface_data_t *model_data = multinet->create(mn_name, 6000);
    esp_mn_commands_update_from_sdkconfig(multinet, model_data); // Add speech commands from sdkconfig
    int mu_chunksize = multinet->get_samp_chunksize(model_data);
    assert(mu_chunksize == afe_chunksize);

    // print active speech commands
    multinet->print_active_speech_commands(model_data);
    printf("------------detect start------------\n");
    // FILE *fp = fopen("/sdcard/out1", "w");
    // if (fp == NULL) printf("can not open file\n");
    while (task_flag)
    {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL)
        {
            printf("fetch error!\n");
            break;
        }

        if (detect_flag == 1)
        {
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

            if (mn_state == ESP_MN_STATE_DETECTING)
            {
                continue;
            }

            if (mn_state == ESP_MN_STATE_DETECTED)
            {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                for (int i = 0; i < mn_result->num; i++)
                {
                    printf("TOP %d, command_id: %d, phrase_id: %d, string:%s prob: %f\n",
                           i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->string, mn_result->prob[i]);
                }
                printf("\n-----------listening-----------\n");
            }

            if (mn_state == ESP_MN_STATE_TIMEOUT)
            {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                printf("timeout, string:%s\n", mn_result->string);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = 0;
                printf("\n-----------awaits to be waken up-----------\n");
                continue;
            }
        }
    }
    if (model_data)
    {
        multinet->destroy(model_data);
        model_data = NULL;
    }
    printf("detect exit\n");
    vTaskDelete(NULL);
}

void init_model()
{
    models = esp_srmodel_init("model");
    if (models == NULL)
    {
        ESP_LOGE("SR", "Failed to init model");
        return;
    }
    ESP_LOGI("SR", "Model init success");
    afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;
    if (afe_handle == NULL)
    {
        ESP_LOGE("SR", "Failed to init afe");
        return;
    }
    ESP_LOGI("SR", "AFE init success");

    afe_config_t config = AFE_CONFIG_DEFAULT();
    config.pcm_config.sample_rate = 16000;
    config.pcm_config.mic_num = 1;
    config.pcm_config.ref_num = 1;
    config.pcm_config.total_ch_num = 2;
    config.wakenet_mode = DET_MODE_90;
    config.afe_ns_mode = NS_MODE_SSP;
    config.wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    ;
    afe_data = afe_handle->create_from_config(&config);
    task_flag = 1;
    xTaskCreatePinnedToCore(&detect_Task, "detect", 16 * 1024, (void *)afe_data, 5, NULL, 1);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 16 * 1024, (void *)afe_data, 5, NULL, 0);
}

#endif