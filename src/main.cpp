#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#include "FastLED_RGBW.h"

#define DATA_PIN 12
#define NUM_LEDS 120

CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];
AsyncWebServer server(80);

void handleRoot(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/index.html", "text/html");
}

void handleJs(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/index.js", "text/javascript");
}

void handleColor(AsyncWebServerRequest *request) {
  int r = request->getParam("r", true)->value().toInt();
  int g = request->getParam("g", true)->value().toInt();
  int b = request->getParam("b", true)->value().toInt();

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(r, g, b);
    FastLED.show();
    delay(5);
  }

  request->send(200);
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);

  SPIFFS.begin();

  // Leds
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));

  // WiFiManager
  WiFiManager wifiManager;
  wifiManager.setBreakAfterConfig(true);

  // For testing
  // wifiManager.resetSettings();

  if (!wifiManager.autoConnect())
  {
    Serial.println("Did not connect");
    ESP.restart();
  }

  // Web server
  server.onNotFound(notFound);
  server.on("/", handleRoot);
  server.on("/index.js", handleJs);
  server.on("/color", HTTP_POST, handleColor);
  server.begin();

  Serial.println("Running main program...");
}

void loop() {

}