#include <Arduino.h>
#include <DHT11.h>

DHT11::DHT11(gpio_num_t pin) {
    _DHT11_PIN = pin;

    DHT11_OUT;          //設置端口方向
    DHT11_CLR;          //拉低端口
    DelayUs(19*1000);   //持續最低18ms

    DHT11_SET;          //釋放總線
    DelayUs(30);        //總線由上拉電阻拉高，主機延時 30us
    DHT11_IN;           //設置端口方向

    while(!gpio_get_level(_DHT11_PIN));  //等待80us低電位響應信號結束
    while(gpio_get_level(_DHT11_PIN));   //將總線拉高80us
    }

//us延時函數
void DHT11::DelayUs(uint32_t nCount) {
    ets_delay_us(nCount);
}

//讀取 DHT11 數據
uint8_t DHT11::DHT11_ReadValue(void) {
    uint8_t i, sbuf = 0;
    for (i=8; i>0; i--) {
        sbuf<<=1;
        while(!gpio_get_level(_DHT11_PIN));
        DelayUs(30);  //延時 30us 後檢測數據線是否還是高電位
        if (gpio_get_level(_DHT11_PIN)) {
        sbuf|=1;
        }
        else {
        sbuf|=0;
        }
        while(gpio_get_level(_DHT11_PIN));
    }
    return sbuf;
}

//讀取溫溼度
uint8_t* DHT11::DHT11_ReadTemHum(uint8_t *buf) {
    for (uint8_t i=0; i<=4; i++) {
        buf[i] = DHT11_ReadValue();
    }
    return buf;
}