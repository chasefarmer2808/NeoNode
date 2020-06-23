#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#include "./env.h"

// Update these values for your specific Neopixel strip.
#define NEOPIXEL_PIN 12
#define NUM_PIXELS 3
#define BRIGHTNESS 50
#define RGB_SETTING NEO_GRB

const char* STRIP_TYPE = "strip";
const bool SUPPORTS_RGBW = false;

// Set static IP address.
IPAddress localIP(192, 168, 0, 110);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);

Adafruit_NeoPixel neoPixel = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, RGB_SETTING + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  neoPixel.setBrightness(BRIGHTNESS);
  neoPixel.begin();
  allOff();
  neoPixel.show();

  // Configures static IP address
  if (!WiFi.config(localIP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  printConnectionInfo();

  server.on("/", HTTP_GET, sendHeartbeat);
  server.on("/neopixel", HTTP_POST, setPixels);
  server.on("/neopixel/info", HTTP_GET, sendNeopixelInfo);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {}

void allOff() {
    for (int i = 0; i < neoPixel.numPixels(); i++) {
        neoPixel.setPixelColor(i, 0);
    }
    neoPixel.show();
}

void printConnectionInfo() {
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void sendHeartbeat(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", "Success");
}

void setPixels(AsyncWebServerRequest *request) {
  // Make sure body is not empty.
  if (!request->hasParam("pixels", true)) {
    return request->send(400, "text/plain", "Pixel color array required.");
  }

  // Need to build a special size variable.
  const size_t ARRAY_SIZE = JSON_ARRAY_SIZE(NUM_PIXELS);
  String rawArray = request->getParam("pixels", true)->value();
  StaticJsonDocument<ARRAY_SIZE> arrayBuf;
  deserializeJson(arrayBuf, rawArray);
  JsonArray colorArray = arrayBuf.as<JsonArray>();
  
  for (int i = 0; i < NUM_PIXELS; i++) {
    neoPixel.setPixelColor(i, colorArray[i].as<uint32_t>());

    // For some reason, putting this in the loop makes the first pixel the correct color.
    neoPixel.show();
  }
  
  request->send(200);
}

void sendNeopixelInfo(AsyncWebServerRequest *request) {
  // Size of json object = number of static data + num pixels in neopixel strip + 1 for array item.
  const size_t CAPACITY = 3 + NUM_PIXELS + 1;
  
  StaticJsonDocument<JSON_OBJECT_SIZE(CAPACITY)> doc;
  doc["num_pixels"] = NUM_PIXELS;
  doc["strip_type"] = STRIP_TYPE;
  doc["supports_rgbw"] = SUPPORTS_RGBW;

  JsonArray pixels = doc.createNestedArray("pixels");

  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.add(neoPixel.getPixelColor(i));
  }
  
  String res;
  serializeJson(doc, res);
  
  request->send(200, "application/json", res);
}
