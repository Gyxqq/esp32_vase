#ifndef SEEECH_RECOGNITION
#define SEEECH_RECOGNITION
#include <esp_log.h>
#include <driver/i2s.h>
#include <driver/i2s_common.h>
#include <stdint.h>
#include <math.h>

void process_audio(uint8_t *pcm_data, int num_samples, int sr, int16_t *mel_spectrum);

#define N_FFT 256
#define HOP_LENGTH (N_FFT / 2)
#define N_MELS 40
#define AUDIO_SIZE 8000
#define FIXED_POINT_SCALE 32768 // 用于定点数运算的缩放因子
#define MEL_POINTS (N_MELS + 2)
#define M_PI 3.14159265358979323846

// 定点数缩放与还原
#define MULT_FIXED(a, b) ((int32_t)((int64_t)(a) * (b) / FIXED_POINT_SCALE))
#define FLOAT_TO_FIXED(x) ((int32_t)((x) * FIXED_POINT_SCALE))
#define FIXED_TO_FLOAT(x) ((float)(x) / FIXED_POINT_SCALE)
int init_i2s()
{
    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX, // 设置i2s工作模式，根据需求设置
        .sample_rate = 8000,                                 // 设置I2S 采样率，根据音频确定采样率
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,         // 设置采样位数
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,         // 设置I2S 通道格式（分离左右声道）
        .communication_format =I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,   // 设置I2S 通讯格式
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
        // int16_t *mfcc = NULL;
        // process_audio(data, 8000, 8000, mfcc);
        for (int i = 0; i < 8000; i++)
        {
            printf("%lu ", data[i]>>8);
        }
    }
    free(data);
}

// 梅尔频率与赫兹之间的转换（使用定点数运算）
int32_t hz_to_mel_fixed(int32_t hz)
{
    return FLOAT_TO_FIXED(2595 * log10f(1 + FIXED_TO_FLOAT(hz) / 700.0f));
}

int32_t mel_to_hz_fixed(int32_t mel)
{
    return FLOAT_TO_FIXED(700 * (powf(10, FIXED_TO_FLOAT(mel) / 2595.0f) - 1));
}

// 转换8位PCM到16位
int16_t convert_8bit_to_16bit(uint8_t sample)
{
    return (int16_t)(sample - 128) << 8;
}

// 生成梅尔滤波器
void generate_mel_filters(int16_t mel_filters[N_MELS][N_FFT / 2], int sr)
{
    int32_t mel_points[MEL_POINTS];
    int32_t mel_min = hz_to_mel_fixed(FLOAT_TO_FIXED(0));
    int32_t mel_max = hz_to_mel_fixed(FLOAT_TO_FIXED(sr / 2));

    // 计算梅尔刻度上的边界点
    // 计算梅尔刻度上的边界点
    for (int i = 0; i < MEL_POINTS; i++)
    {
        mel_points[i] = mel_min + i * (mel_max - mel_min) / (N_MELS + 1);
    }

    // 计算每个滤波器的系数
    for (int m = 0; m < N_MELS; m++)
    {
        for (int k = 0; k < N_FFT / 2; k++)
        {
            int32_t freq = FLOAT_TO_FIXED(k * sr / (double)N_FFT);
            int32_t mel_freq = hz_to_mel_fixed(freq);
            int32_t mel_coeff = 0;
           
            // 计算滤波器系数
            if (mel_freq < mel_points[m] || mel_freq > mel_points[m + 2])
            {
                mel_coeff = 0;
            }
            else if (mel_freq < mel_points[m + 1])
            {
                mel_coeff = (mel_freq - mel_points[m]) / (mel_points[m + 1] - mel_points[m]);
            }
            else
            {
                mel_coeff = (mel_points[m + 2] - mel_freq) / (mel_points[m + 2] - mel_points[m + 1]);
            }
             printf("m = %d, k = %d, freq = %ld, mel_coeff = %ld\n", m, k, freq, mel_coeff);
            // 将系数存储到滤波器组中
            mel_filters[m][k] = mel_coeff;
        }
    }
}

void fft(int16_t *input, int16_t *output)
{
    int16_t real[N_FFT];
    int16_t imag[N_FFT];
    // 初始化实部和虚部
    for (int i = 0; i < N_FFT; i++)
    {
        real[i] = input[i];
        imag[i] = 0;
    }

    // Cooley-Tukey FFT算法
    int n = N_FFT;
    int m = log2(N_FFT);

    // 位反转排序
    for (int i = 0; i < n; ++i)
    {
        int j = 0;
        for (int k = 0; k < m; ++k)
        {
            if (i & (1 << k))
            {
                j |= (1 << (m - 1 - k));
            }
        }
        if (i < j)
        {
            int16_t temp_real = real[i];
            int16_t temp_imag = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = temp_real;
            imag[j] = temp_imag;
        }
    }

    // FFT蝶形运算
    for (int s = 1; s <= m; ++s)
    {
        int m1 = 1 << s;
        int m2 = m1 >> 1;
        int16_t w_real = FLOAT_TO_FIXED(1.0);
        int16_t w_imag = FLOAT_TO_FIXED(0.0);
        int16_t wm_real = FLOAT_TO_FIXED(cos(-2 * M_PI / m1));
        int16_t wm_imag = FLOAT_TO_FIXED(sin(-2 * M_PI / m1));

        for (int j = 0; j < m2; ++j)
        {
            for (int k = j; k < n; k += m1)
            {
                int t = k + m2;
                int16_t t_real = MULT_FIXED(w_real, real[t]) - MULT_FIXED(w_imag, imag[t]);
                int16_t t_imag = MULT_FIXED(w_real, imag[t]) + MULT_FIXED(w_imag, real[t]);

                real[t] = real[k] - t_real;
                imag[t] = imag[k] - t_imag;
                real[k] = real[k] + t_real;
                imag[k] = imag[k] + t_imag;
            }
            int16_t temp_w_real = w_real;
            w_real = MULT_FIXED(w_real, wm_real) - MULT_FIXED(w_imag, wm_imag);
            w_imag = MULT_FIXED(temp_w_real, wm_imag) + MULT_FIXED(w_imag, wm_real);
        }
    }

    // 将结果组合到输出
    for (int i = 0; i < N_FFT; i++)
    {
        output[i] = sqrt(real[i] * real[i] + imag[i] * imag[i]); // 计算幅值
    }
}

// 处理音频并计算梅尔频谱图
void process_audio(uint8_t *pcm_data, int num_samples, int sr, int16_t *mel_spectrum)
{
    // 将8位PCM数据转换为16位数据
    int16_t *pcm_data_16bit = (int16_t *)malloc(num_samples * sizeof(int16_t));
    for (int i = 0; i < num_samples; i++)
    {
        pcm_data_16bit[i] = convert_8bit_to_16bit(pcm_data[i]);
    }

    // 生成梅尔滤波器
    int16_t mel_filters[N_MELS][N_FFT / 2];
    generate_mel_filters(mel_filters, sr);

    // 执行FFT并计算梅尔频谱图
    for (int frame = 0; frame < num_samples / HOP_LENGTH; frame++)
    {
        int16_t fft_result[N_FFT / 2];
        fft(&pcm_data_16bit[frame * HOP_LENGTH], fft_result);

        // 计算梅尔频谱
        mel_spectrum = malloc(N_MELS * sizeof(int16_t));
        for (int m = 0; m < N_MELS; m++)
        {
            for (int k = 0; k < N_FFT / 2; k++)
            {
                mel_spectrum[m] += (fft_result[k] * mel_filters[m][k]) / FIXED_POINT_SCALE;
            }
        }
    }

    free(pcm_data_16bit);
}

#endif