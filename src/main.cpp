#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

const byte LDR_PIN = 36;

//手機 AP 熱點
const char* ssid_0 = "羊羊ㄉiphone🐑";
const char* password_0 = "ibmf7777";
//淡水住處 AP
const char* ssid_1 = "69-7";
const char* password_1 = "0982215945";

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  // 腳位設定
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  // SPIFFS 開啟
  if (!SPIFFS.begin(true)) {
    Serial.println("無法掛載SPIFFS分區~");
    while (1) { }
  }

  // WiFi 連線設定
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_1, password_1);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("\nIP位置 : ");
  Serial.println(WiFi.localIP());
  Serial.print("\n訊號強度 : ");
  Serial.println(WiFi.RSSI());

  // 網路路由
  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
  server.serveStatic("/img", SPIFFS, "/www/img/");
  server.serveStatic("/favicon", SPIFFS, "/www/favicon.png");

  server.on("/LDR", HTTP_GET, [](AsyncWebServerRequest * req) {
    uint16_t val = analogRead(LDR_PIN);
    req->send(200, "text/plain", String(val));
  });

  server.begin();

}

void loop() {

}