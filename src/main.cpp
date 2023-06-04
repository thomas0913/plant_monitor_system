#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <DHT11.h>
#include "driver/gpio.h"

#define LDR_PIN                       36
#define DHT11_PIN                     (gpio_num_t)27
#define DHT11_Timer_Cycle             3
#define DUSTHUMIDETECTOR_ANALOG_PIN   33
#define DUSTHUMIDETECTOR_DIGITAL_PIN  25
#define DUSTHUMIDETECTOR_Timer_Cycle  3

//=========================//
//        任務參照          //
//=========================//
TaskHandle_t dhtTask, dustHumiDetectorTask;

//==========================================//
//        定時器參照定義 & 回條函數           //
//==========================================//
esp_timer_handle_t TempHumi_timer = 0, dustHumi_timer = 0;
void dht_timer_periodic(void *arg);
void dustHumiDetector_timer_periodic(void *arg);

//===========================//
//        WiFi帳密設定        //
//===========================//
const char* ssid = "Galaxy A71B0A7"; //手機 AP 熱點
const char* password = "svjh6777";

//===========================//
//      遠端主機連線設定       //
//===========================//
const char* ip = "192.168.195.19"; //遠端主機 socket host
const uint port = 1884;
bool remote_host_connect_status = false;

// AP 連線設定
void APConnect(void) {
  WiFi.mode(WIFI_STA); // 模式為連線方
  WiFi.begin(ssid, password); // 開始連線
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }

  //輸出連線資訊
  Serial.println("");
  Serial.print("\nIP位置 : ");
  Serial.println(WiFi.localIP());
  Serial.print("\n訊號強度 : ");
  Serial.println(WiFi.RSSI());
  Serial.print("\n");
}

void sendRequest(String topic, String data, String logger) {
  WiFiClient espClient;
  if (!remote_host_connect_status) {
    if (espClient.connect(ip, port)) { // Establish a connection
      Serial.println("[遠端伺服器]: 連線成功 ! ! !");
      remote_host_connect_status = true;
    }
    else {
      Serial.println("[遠端伺服器]: 連線失敗 . . .");
    }
  }

  if (espClient.connected()) {
    String payload;
    payload += "Title=";
    payload += topic;
    payload += "\nContent=\n";
    payload += data;
    payload += "\n";
    payload += logger;
    payload += "\n======================";
    payload += "\n======================";
    espClient.println(payload);  // send data
    Serial.println("\n[Tx]");
    Serial.println(payload);
    Serial.println("\n");
  } else {
    Serial.println("[遠端伺服器]: 連線中斷 . . .");
    remote_host_connect_status = false;
    espClient.stop();
    return;
  }
  espClient.stop();
  Serial.println("[遠端伺服器]: 連線結束 . . .");
}

//DHT11任務
void DHT11_task(void *pvParam) {
  gpio_pad_select_gpio(DHT11_PIN);

  while(1) {
    String message;
    String message_log;
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
      message = "temp: ";
      message += String(temp);
      message += "\nhumi: ";
      message += String(humi);
      message_log = "";
    } else {
      printf("DHT11 Error!\r\n");
      message = "";
      message_log = "DHT11 Error!";
    }
    sendRequest("esp32_dht11", message, message_log);

    //vTaskSuspend(NULL);//將任務自身暫停，不恢復將不運行
    vTaskDelay( 4000 / portTICK_PERIOD_MS );
  }
}

//
void DustHumiDetect_task(void *pvParam) {
  pinMode(DUSTHUMIDETECTOR_DIGITAL_PIN, INPUT);
  pinMode(DUSTHUMIDETECTOR_ANALOG_PIN, INPUT);

  while(1) {
    String message;
    String message_log;
    uint16_t DustHumi_data = analogRead(DUSTHUMIDETECTOR_ANALOG_PIN);
    float DustHumi_percent = ((4096.0 - (float)DustHumi_data)/4096.0)*100.0;
    
    printf("Humi=%f%%\r\n", DustHumi_percent);
    message = "DustHumi_percent: ";
    message += String(DustHumi_percent);

    if (!digitalRead(DUSTHUMIDETECTOR_DIGITAL_PIN)) {
      printf("請保持適度的澆水，目前土壤非常的濕潤~~~\r\n");
      message_log = "Your plant love u ~~~";
    } else {
      printf("你的植物需要澆水了!!!\r\n");
      message_log = "Please help your plant living !!!";
    }
    sendRequest("esp32_DustHumiDetect", message, message_log);

    vTaskDelay( 3000 / portTICK_PERIOD_MS );
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  // AP 連線
  APConnect();

  //--------配置定時器-------//
  esp_timer_create_args_t start_dht = {
    .callback = &dht_timer_periodic,
    .arg = NULL,
    .name = "PeriodicTimer"};
  esp_timer_create_args_t start_dustHumiDetector = {
    .callback = &dustHumiDetector_timer_periodic,
    .arg = NULL,
    .name = "DustHumi PeriodicTimer"};
  //--------定時器初始化---------//
  esp_timer_init();
  esp_timer_create(&start_dht, &TempHumi_timer);//创建定时器
  esp_timer_start_periodic(TempHumi_timer, DHT11_Timer_Cycle * 1000 * 1000); //定时器每300万微秒（3秒）中断一次
  esp_timer_create(&start_dustHumiDetector, &dustHumi_timer);
  esp_timer_start_periodic(dustHumi_timer, DUSTHUMIDETECTOR_Timer_Cycle * 1000 * 1000);

  // 監測來自 DHT11 模組的資料
  xTaskCreatePinnedToCore(DHT11_task, "DHT11", 4000, NULL, 3, &dhtTask, 1);
  xTaskCreatePinnedToCore(DustHumiDetect_task, "DustHumiDetect", 4000, NULL, 2, &dustHumiDetectorTask, 1);
}

void loop() {
  ;
}

//定時器回調函數
void dht_timer_periodic(void *arg) {
  vTaskResume(dhtTask); //恢復任務
}

void dustHumiDetector_timer_periodic(void *arg) {
  vTaskResume(dustHumiDetectorTask);
}