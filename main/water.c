#ifndef WATER
#define WATER
#include "driver/gpio.h"
#include <portmacro.h>

void init_water()
{
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1UL << GPIO_NUM_25;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_config_t io_conf2;
    io_conf2.mode = GPIO_MODE_OUTPUT;
    io_conf2.pin_bit_mask = 1UL << GPIO_NUM_22;
    io_conf2.intr_type = GPIO_INTR_DISABLE;
    io_conf2.pull_down_en = 0;
    io_conf2.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&io_conf2));
}

void water_on()
{
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_25, 1));
    // ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_22, 0));
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_25,0)); 
    // ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_22, 0));
}
#endif