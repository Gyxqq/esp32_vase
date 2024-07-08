#ifndef SENSOR_C
#define SENSOR_C

#include "driver/i2c_master.h"

// #include <test_board.h>
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#define I2C_SCL_IO 18
#define I2C_SDA_IO 5
i2c_master_bus_handle_t bus_handle;
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
        .scl_speed_hz = 100000,
    };

    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    return 0;
}
int read_sensor()
{
    uint8_t data[8] = {0x70, 0xac, 0x33, 0x00};
    i2c_master_transmit(bus_handle, data, 4, 1000);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    data[0]=0x71;
    i2c_master_transmit_receive(bus_handle, data, 1,data, 7, 1000);

        for (int i = 0; i < 7; i++)
    {
        printf("%x ", data[i]);
    }
    return 0;
}
#endif