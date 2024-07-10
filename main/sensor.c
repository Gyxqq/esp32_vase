#ifndef SENSOR_C
#define SENSOR_C

#include "driver/i2c_master.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#define I2C_SCL_IO 18
#define I2C_SDA_IO 5
void search_sensor();
static i2c_master_dev_handle_t dev_handle;
static i2c_master_bus_handle_t bus_handle;
struct temp_hum_data
{
    float temp;
    float hum;
};
int init_i2c()
{

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = 0,
        .scl_io_num = 18,
        .sda_io_num = 5,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x38,
        .scl_speed_hz = 10000,
        .flags.disable_ack_check = false,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
    search_sensor();
    return 0;
}
struct temp_hum_data read_sensor()
{
    esp_err_t ret;
    // uint8_t com_init[3] = {0xbe, 0x08, 0x00}; // 0xbe
    // i2c_master_transmit(dev_handle, com_init, 3, -1);
    uint8_t data[8] = {0xac, 0x33, 0x00};
    ret = i2c_master_transmit(dev_handle, data, 3, -1);
    if (ret != ESP_OK)
    {
        printf("i2c_master_transmit failed\n");
        struct temp_hum_data re={-1,-1};
        return re;
        
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    data[0] = 0x71;
    ret = i2c_master_transmit_receive(dev_handle, data, 1, data, 7, -1);
    if (ret != ESP_OK)
    {
        printf("i2c_master_transmit_receive failed\n");
        struct temp_hum_data re={-1,-1};
        return re;
    }

    for (int i = 0; i < 7; i++)
    {
        printf("%x ", data[i]);
    }
    int32_t tmp = 0;
    float temp = 0;
    tmp <<= 8;
    tmp |= data[3];
    tmp <<= 8;
    tmp |= data[4];
    tmp <<= 8;
    tmp |= data[5];
    tmp &= 0x000fffff;
    temp = (tmp * 1.0 / 1048576 * 1.0) * 200 - 50;
    float humidity = 0;
    tmp = 0;
    tmp |= data[1];
    tmp <<= 8;
    tmp |= data[2];
    tmp <<= 8;
    tmp |= data[3];
    tmp >>= 4;
    tmp &= 0x000fffff;
    humidity = (tmp * 1.0 / 1048576 * 1.0) * 100;
    printf("\ntemp: %.2f\nhumidity: %.2f%%\n", temp, humidity);

    struct temp_hum_data re={temp,humidity};
    return re;
}
void search_sensor()
{
    // 按地址搜索所有设备 使用i2c_master_probe
    for (int i = 0; i < 128; i++)
    {
        if (i2c_master_probe(bus_handle, i, 20) == ESP_OK)
        {
            printf("Found device at address: %x\n", i);
        }
    }
}

#endif