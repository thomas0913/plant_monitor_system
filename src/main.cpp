#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "羊羊ㄉiphone🐑";
const char* password = "ibmf7777";

WebServer server(80);

void rootRouter();

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("\nIP位置 : ");
  Serial.println(WiFi.localIP());
  Serial.print("\n訊號強度 : ");
  Serial.println(WiFi.RSSI());

  server.on("/", rootRouter);
  server.on("/about", []() {
    server.send(200, "text/html; charset=utf-8", "是在哈囉逆?!");
  });
  server.onNotFound([]() { //匿名函式
    server.send(404, "text/plain", "File Not Found!");
  });
  server.on("/sw", []() {
    String state = server.arg("led");

    if (state == "on") {
      digitalWrite(LED_BUILTIN, LOW);
    } else if (state = "off") {
      digitalWrite(LED_BUILTIN, HIGH);
    }

    server.send(200, "text/html", "LED is <b>" + state + "</b>.");
  });

  server.begin();
}

void loop() {
  server.handleClient();
}

void rootRouter() {
  String HTML = "<!DOCTYPE html>\
  <html><head><meta charset= 'utf-8' ></head>\
  <body>慢慢長路，總要從第一步開始。\
  </body></html>";
  server.send(200, "text/html", HTML);
}