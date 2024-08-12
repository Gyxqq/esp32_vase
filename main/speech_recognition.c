#ifndef SEEECH_RECOGNITION
#define SEEECH_RECOGNITION
#include <esp_log.h>
#include <driver/i2s.h>
#include <driver/i2s_common.h>
#include <math.h>
#define PI 3.14159265358979323846
#define SAMPLE_RATE 8000           // 8kHz采样率
#define DURATION 1                 // 处理1秒的数据
#define N (SAMPLE_RATE * DURATION) // DFT长度
#define FRAME_SIZE 256             // 每帧256个样本 (32ms @ 8kHz)
#define NUM_FILTERS 26             // Mel滤波器组的数量
#define NUM_MFCC_COEFFS 13         // 提取的MFCC系数数量
void extract_mfcc(float *frame, float *mfcc);
void process_pcm_frames(int8_t *pcm_data, size_t pcm_length, float *mfcc);
typedef struct
{
    float real;
    float imag;
} Complex;
int init_i2s()
{
    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX, // 设置i2s工作模式，根据需求设置
        .sample_rate = 8000,                                 // 设置I2S 采样率，根据音频确定采样率
        .bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT,         // 设置采样位数
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,         // 设置I2S 通道格式（分离左右声道）
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
    int8_t *data = (int8_t *)malloc(1024 * sizeof(int8_t));
    while (1)
    {
        i2s_read(0, data, 1024 * sizeof(int8_t), &bytes_read, portMAX_DELAY);
        float *mfcc = malloc(sizeof(float) * 1024);
        process_pcm_frames(data, 1024, mfcc);
        // ESP_EARLY_LOGI("I2S", "read 1024");
    }
    free(data);
}

// DFT实现函数
void dft(float *x, Complex *X)
{
    for (uint8_t k = 0; k < N; k++)
    {
        float sum_real = 0.0f;
        float sum_imag = 0.0f;
        for (uint8_t n = 0; n < N; n++)
        {
            float angle = 2 * PI * k * n / N;
            sum_real += x[n] * cosf(angle);
            sum_imag -= x[n] * sinf(angle);
        }
        X[k].real = sum_real;
        X[k].imag = sum_imag;
    }
}
#define FRAME_SIZE 256  // 每帧包含256个样本 (32ms @ 8kHz)
#define OVERLAP 128     // 帧重叠部分，通常为帧长度的一半

void process_pcm_frames(int8_t *pcm_data, size_t pcm_length, float *mfcc) {
    int num_frames = (pcm_length - FRAME_SIZE) / (FRAME_SIZE - OVERLAP) + 1;
    float frame[FRAME_SIZE];
    for (int i = 0; i < num_frames; i++) {
        // 提取当前帧数据
        int start_index = i * (FRAME_SIZE - OVERLAP);
        for (int j = 0; j < FRAME_SIZE; j++) {
            frame[j] = (float)pcm_data[start_index + j]; // 转换为浮点数
        }
        extract_mfcc(frame, mfcc);
        // 此处可以将 mfcc 存储或用于进一步分析
    }
}

// Hamming 窗函数
void hamming_window(float *frame, int frame_size)
{
    for (int i = 0; i < frame_size; i++)
    {
        frame[i] *= 0.54 - 0.46 * cos(2 * PI * i / (frame_size - 1));
    }
}

// Mel滤波器组初始化
void mel_filter_bank(float mel_filters[NUM_FILTERS][FRAME_SIZE / 2 + 1], int sample_rate, int frame_size)
{
    float low_freq_mel = 0;
    float high_freq_mel = 2595 * log10(1 + (sample_rate / 2) / 700.0);
    float mel_points[NUM_FILTERS + 2];
    float hz_points[NUM_FILTERS + 2];
    int bin[NUM_FILTERS + 2];

    // 计算Mel频率点
    for (int i = 0; i < NUM_FILTERS + 2; i++)
    {
        mel_points[i] = low_freq_mel + (high_freq_mel - low_freq_mel) * i / (NUM_FILTERS + 1);
        hz_points[i] = 700 * (pow(10, mel_points[i] / 2595) - 1);
        bin[i] = floor((frame_size + 1) * hz_points[i] / sample_rate);
    }

    // 初始化滤波器组
    for (int i = 0; i < NUM_FILTERS; i++)
    {
        for (int j = 0; j < frame_size / 2 + 1; j++)
        {
            mel_filters[i][j] = 0;
            if (j >= bin[i] && j <= bin[i + 1])
                mel_filters[i][j] = (float)(j - bin[i]) / (bin[i + 1] - bin[i]);
            if (j >= bin[i + 1] && j <= bin[i + 2])
                mel_filters[i][j] = (float)(bin[i + 2] - j) / (bin[i + 2] - bin[i + 1]);
        }
    }
}

// 对数Mel频谱
void log_mel_spectrum(float *fft_magnitude, float mel_filters[NUM_FILTERS][FRAME_SIZE / 2 + 1], float *mel_spectrum)
{
    for (int i = 0; i < NUM_FILTERS; i++)
    {
        mel_spectrum[i] = 0;
        for (int j = 0; j < FRAME_SIZE / 2 + 1; j++)
        {
            mel_spectrum[i] += fft_magnitude[j] * mel_filters[i][j];
        }
        mel_spectrum[i] = log(mel_spectrum[i]);
    }
}

// DCT 变换
void dct(float *mel_spectrum, float *mfcc)
{
    for (int k = 0; k < NUM_MFCC_COEFFS; k++)
    {
        mfcc[k] = 0;
        for (int n = 0; n < NUM_FILTERS; n++)
        {
            mfcc[k] += mel_spectrum[n] * cos(PI * k * (2 * n + 1) / (2 * NUM_FILTERS));
        }
    }
}

// MFCC提取
void extract_mfcc(float *frame, float *mfcc)
{

    float mel_filters[NUM_FILTERS][FRAME_SIZE / 2 + 1];

    // 步骤1: 加窗
    hamming_window(frame, FRAME_SIZE);

    // 步骤2: DFT变换
    Complex *X = malloc(sizeof(Complex) * N);
    float *fft_magnitude = malloc(sizeof(float) * (FRAME_SIZE / 2 + 1));
    dft(frame, X);
    for (int i = 0; i < FRAME_SIZE / 2 + 1; i++)
    {
        fft_magnitude[i] = sqrt(X[i].real * X[i].real + X[i].imag * X[i].imag);
    }
    free(X);
    // 步骤3: Mel滤波器组初始化
    mel_filter_bank(mel_filters, SAMPLE_RATE, FRAME_SIZE);
    float *mel_spectrum = malloc(sizeof(float) * NUM_FILTERS);
    // 步骤4: 计算对数Mel频谱
    log_mel_spectrum(fft_magnitude, mel_filters, mel_spectrum);
    // 步骤5: 进行DCT变换，得到MFCC系数
    dct(mel_spectrum, mfcc);
    free(fft_magnitude);
    free(mel_spectrum);
}

#endif