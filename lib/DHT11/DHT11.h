#ifndef DHT11_H
#define DHT11_H

#include <Arduino.h>
#include "driver/gpio.h"

#define DHT11_CLR gpio_set_level(_DHT11_PIN, 0)
#define DHT11_SET gpio_set_level(_DHT11_PIN, 1)
#define DHT11_IN  gpio_set_direction(_DHT11_PIN, (gpio_mode_t)GPIO_MODE_DEF_INPUT)
#define DHT11_OUT gpio_set_direction(_DHT11_PIN, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT)

class DHT11 {
    private:
        gpio_num_t _DHT11_PIN; //任意數位腳位

    public:

        DHT11(gpio_num_t pin);
        void DelayUs(uint32_t nCount);
        uint8_t DHT11_ReadValue();
        uint8_t* DHT11_ReadTemHum(uint8_t *buf);
};

#endif