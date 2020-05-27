#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#include "./env.h"

// Update these values for your specific Neopixel strip.
#define NEOPIXEL_PIN 12
#define NUM_PIXELS 3
#define RGB_SETTING NEO_GRB
const char* STRIP_TYPE = "strip";
const bool SUPPORTS_RGBW = false;

WebServer server(80);
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, RGB_SETTING + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  pixels.begin();

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  printConnectionInfo();

  server.on("/", sendHeartbeat);
  server.on("/neopixel", sendNeopixelInfo);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

void printConnectionInfo() {
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void sendHeartbeat() {
  server.send(200, "text/plain", "Success");
}

void sendNeopixelInfo() {
  StaticJsonDocument<JSON_OBJECT_SIZE(3)> doc;
  doc["num_pixels"] = NUM_PIXELS;
  doc["strip_type"] = STRIP_TYPE;
  doc["supports_rgbw"] = SUPPORTS_RGBW;
  
  String res;
  serializeJson(doc, res);
  
  server.send(200, "application/json", res);
}
