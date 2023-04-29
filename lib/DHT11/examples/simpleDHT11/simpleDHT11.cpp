#include <Arduino.h>
#include <DHT11.h>
#include "driver/gpio.h"

#define DHT11_PIN           (gpio_num_t)27
#define DHT11_Timer_Cycle   3

//=========================//
//        任務參照          //
//=========================//
TaskHandle_t dhtTask;

//==========================================//
//        定時器參照定義 & 回條函數           //
//==========================================//
esp_timer_handle_t TempHumi_timer = 0;
void timer_periodic(void *arg);

//DHT11任務
void DHT11_task(void *pvParam) {
  gpio_pad_select_gpio(DHT11_PIN);

  while(1) {
    DHT11 dht11(DHT11_PIN);

    uint8_t data[5] = {0};
    dht11.DHT11_ReadTemHum(data);
    uint8_t humi         = data[0];
    uint8_t humi_decimal = data[1];
    uint8_t temp         = data[2];
    uint8_t temp_decimal = data[3];
    uint8_t check        = data[4];
    if (check = temp + temp_decimal + humi + humi_decimal) {
      printf("Temp=%d, Humi=%d\r\n", temp, humi);
      //------- 以JSON資料供於網頁使用 -------//
    } else {
      printf("DHT11 Error!\r\n");
    }

    vTaskSuspend(NULL);//將任務自身暫停，不恢復將不運行
  }
}



void setup() {
  Serial.begin(115200);

  //--------配置定時器-------//
  esp_timer_create_args_t start_dht = {
    .callback = &timer_periodic,
    .arg = NULL,
    .name = "PeriodicTimer"};
  //--------定時器初始化---------//
  esp_timer_init();
  esp_timer_create(&start_dht, &TempHumi_timer);//创建定时器
  esp_timer_start_periodic(TempHumi_timer, DHT11_Timer_Cycle * 1000 * 1000); //定时器每300万微秒（3秒）中断一次

  // 監測來自 DHT11 模組的資料
  xTaskCreatePinnedToCore(DHT11_task, "DHT11", 4000, NULL, 2, &dhtTask, 1);
}

void loop() { }

//定時器回調函數
void timer_periodic(void *arg) {
  vTaskResume(dhtTask); //恢復任務
}