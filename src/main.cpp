#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <DHT11.h>
#include "driver/gpio.h"

#define LDR_PIN                       36
#define DHT11_PIN                     (gpio_num_t)27
#define DHT11_Timer_Cycle             3
#define DURTHUMIDETECTOR_ANALOG_PIN   33
#define DURTHUMIDETECTOR_DIGITAL_PIN  25
#define DURTHUMIDETECTOR_Timer_Cycle  3

//=========================//
//        任務參照          //
//=========================//
TaskHandle_t dhtTask, durtHumiDetectorTask;

//==========================================//
//        定時器參照定義 & 回條函數           //
//==========================================//
esp_timer_handle_t TempHumi_timer = 0, durtHumi_timer = 0;
void dht_timer_periodic(void *arg);
void durtHumiDetector_timer_periodic(void *arg);

//===========================//
//        WiFi帳密設定        //
//===========================//
const char* ssid_0 = "羊羊ㄉiphone🐑";//手機 AP 熱點
const char* password_0 = "ibmf7777";
const char* ssid_1 = "69-7";//淡水住處 AP
const char* password_1 = "0982215945";

//===========================//
//        MQTT參數設定        //
//===========================//
const char* mqttServer = "0.tcp.jp.ngrok.io";
const int mqttPort = 11872;
const char* mqttClientID = "ESP32Client";
const char* mqttUser = "thomas890913";
const char* mqttPassword = "Tm1309su";

//===========================//
//       Instance Object     //
//===========================//
AsyncWebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

// AP 連線設定
void APConnect(void) {
  WiFi.mode(WIFI_STA); // 模式為連線方
  WiFi.begin(ssid_0, password_0); // 開始連線
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
}

// MQTT Server 連線設定
void MQTTConnect(void) {
  client.setServer(mqttServer, mqttPort);

  while(!client.connected()) {
    Serial.println("\nConnecting to MQTT ...");

    if (client.connect(mqttClientID, mqttUser, mqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state : ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// 網頁伺服器設置
void WebServerRunnig(void) {
  // 靜態網頁
  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
  server.serveStatic("/img", SPIFFS, "/www/img/");
  server.serveStatic("/favicon", SPIFFS, "/www/favicon.png");

  // API
  server.on("/LDR", HTTP_GET, [](AsyncWebServerRequest * req) {
    uint16_t val = analogRead(LDR_PIN);
    req->send(200, "text/plain", String(val));
  });

  server.begin();
}

//DHT11任務
void DHT11_task(void *pvParam) {
  gpio_pad_select_gpio(DHT11_PIN);

  while(1) {
    StaticJsonDocument<200> doc;
    char outputJSON[sizeof(doc)];
    doc["ModelName"] = "DHT11 Sensor";
    doc["description"] = "detecting the value of the humi and temp in the air instantly.";

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
      doc["percentOfTemp"] = temp;
      doc["percentOfHumi"] = humi;
      doc["message"] = "OK";
      //------- 以JSON資料供於網頁使用 -------//
    } else {
      printf("DHT11 Error!\r\n");
      doc["message"] = "DHT11 Error!";
    }

    serializeJson(doc, outputJSON);
    if (client.publish("esp32/dht11", outputJSON) == true) {
      Serial.println("publishing success");
    } else {
      Serial.println("publishing failed");
    }

    vTaskSuspend(NULL);//將任務自身暫停，不恢復將不運行
  }
}

void DurtHumiDetect_task(void *pvParam) {
  pinMode(DURTHUMIDETECTOR_DIGITAL_PIN, INPUT);
  pinMode(DURTHUMIDETECTOR_ANALOG_PIN, INPUT);

  while(1) {
    StaticJsonDocument<200> doc;
    char outputJSON[sizeof(doc)];
    doc["ModelName"] = "Dust Humi Sensor";
    doc["description"] = "Detecting the value of the humi in the dust instantly.";

    uint16_t DurtHumi_data = analogRead(DURTHUMIDETECTOR_ANALOG_PIN);
    float DustHumi_percent = ((4096.0 - (float)DurtHumi_data)/4096.0)*100.0;
    printf("Humi=%f%%\r\n", DustHumi_percent);
    doc["PercentOfHumi"] = DustHumi_percent;

    if (!digitalRead(DURTHUMIDETECTOR_DIGITAL_PIN)) {
      printf("請保持適度的澆水，目前土壤非常的濕潤~~~\r\n");
      doc["message"] = "請保持適度的澆水，目前土壤非常的濕潤~~~";
    } else {
      printf("你的植物需要澆水了!!!\r\n");
      doc["message"] = "你的植物需要澆水了!!!";
    }

    serializeJson(doc, outputJSON);
    if (client.publish("esp32/dustHumi", outputJSON) == true) {
      Serial.println("publishing success");
    } else {
      Serial.println("publishing failed");
    }

    vTaskDelay( 3000 / portTICK_PERIOD_MS );
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  // SPIFFS 開啟
  if (!SPIFFS.begin(true)) {
    Serial.println("無法掛載SPIFFS分區~");
    while (1) { }
  }

  // AP 連線
  APConnect();
  // Web伺服器開啟
  WebServerRunnig();
  // MQTT客戶端連線
  MQTTConnect();

  //--------配置定時器-------//
  esp_timer_create_args_t start_dht = {
    .callback = &dht_timer_periodic,
    .arg = NULL,
    .name = "PeriodicTimer"};
  esp_timer_create_args_t start_durtHumiDetector = {
    .callback = &durtHumiDetector_timer_periodic,
    .arg = NULL,
    .name = "DurtHumi PeriodicTimer"};
  //--------定時器初始化---------//
  esp_timer_init();
  esp_timer_create(&start_dht, &TempHumi_timer);//创建定时器
  esp_timer_start_periodic(TempHumi_timer, DHT11_Timer_Cycle * 1000 * 1000); //定时器每300万微秒（3秒）中断一次
  esp_timer_create(&start_durtHumiDetector, &durtHumi_timer);
  esp_timer_start_periodic(durtHumi_timer, DURTHUMIDETECTOR_Timer_Cycle * 1000 * 1000);

  // 監測來自 DHT11 模組的資料
  xTaskCreatePinnedToCore(DHT11_task, "DHT11", 4000, NULL, 3, &dhtTask, 1);
  xTaskCreatePinnedToCore(DurtHumiDetect_task, "DurtHumiDetect", 4000, NULL, 2, &durtHumiDetectorTask, 1);
}

void loop() {
  client.loop();
}

//定時器回調函數
void dht_timer_periodic(void *arg) {
  vTaskResume(dhtTask); //恢復任務
}

void durtHumiDetector_timer_periodic(void *arg) {
  vTaskResume(durtHumiDetectorTask);
}