#ifndef LIGHT
#define LIGHT
#include "driver/gpio.h"

int init_light_gpio()
{
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1UL << GPIO_NUM_23;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    ESP_ERROR_CHECK( gpio_config(&io_conf));
    return 1;
}
int light_on()
{
    ESP_ERROR_CHECK( gpio_set_level(GPIO_NUM_23, 0));
    return 1;
}
int light_off()
{
    ESP_ERROR_CHECK( gpio_set_level(GPIO_NUM_23, 1));
    return 1;
}
#endif