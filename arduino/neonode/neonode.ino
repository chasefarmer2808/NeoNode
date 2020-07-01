#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#include "./env.h"

// Update these values for your specific Neopixel strip.
#define NEOPIXEL_PIN 12
#define NUM_PIXELS 3
#define NUM_ANIMATIONS 1
#define BRIGHTNESS 50
#define RGB_SETTING NEO_GRB

const char* STRIP_TYPE = "strip";
// Must be unique.
const char* ANIMATIONS[] = {"Rainbow"};
const bool SUPPORTS_RGBW = false;

// Set static IP address.
IPAddress localIP(192, 168, 0, 110);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);

Adafruit_NeoPixel neoPixel = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, RGB_SETTING + NEO_KHZ800);

String selectedAnimation = "";

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
  server.on("/animation", HTTP_POST, setAnimation);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  processAnimation();
}

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

void processAnimation() {
  if (selectedAnimation == ANIMATIONS[0]) {
    allOff();
    while (animationEnabled()) {
      neoPixel.setPixelColor(0, 255, 0, 0);
      neoPixel.show();
      delay(500);
      neoPixel.setPixelColor(0, 0, 0, 0);
      neoPixel.show();
      delay(500);
    }
  }
}

bool animationEnabled() {
  return selectedAnimation != "";
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

void setAnimation(AsyncWebServerRequest *request) {
  // Make sure body is not empty.
  if (!request->hasParam("animation", true)) {
    return request->send(400, "text/plain", "Animation ID required.");
  }

  String animationId = request->getParam("animation", true)->value();
  bool foundAnimation = false;

  // Make sure the desired animation exists.
  for (int i = 0; i < NUM_ANIMATIONS; i++) {
    if (animationId == ANIMATIONS[i]) {
      foundAnimation = true;
    }
  }
  
  if (!foundAnimation) {
    request->send(404, "text/plain", "Animation not found");
    return;
  }

  // Toggle animation.
  if (selectedAnimation == animationId) {
    // Disable animation.
    selectedAnimation = "";
  }
  else {
    // Enable animation
    selectedAnimation = animationId;
  }
  
  request->send(200);
}

void sendNeopixelInfo(AsyncWebServerRequest *request) {
  // Size of json object = number of static data + num pixels in neopixel strip + num animations + number of arrays.
  const size_t CAPACITY = 4 + NUM_PIXELS + NUM_ANIMATIONS + 3;
  
  StaticJsonDocument<JSON_OBJECT_SIZE(CAPACITY)> doc;
  doc["num_pixels"] = NUM_PIXELS;
  doc["strip_type"] = STRIP_TYPE;
  doc["supports_rgbw"] = SUPPORTS_RGBW;
  doc["selected_animation"] = selectedAnimation;

  JsonArray pixels = doc.createNestedArray("pixels");

  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.add(neoPixel.getPixelColor(i));
  }

  JsonArray animations = doc.createNestedArray("animations");

  for (int j = 0; j < NUM_ANIMATIONS; j++) {
    animations.add(ANIMATIONS[j]);
  }
  
  String res;
  serializeJson(doc, res);
  
  request->send(200, "application/json", res);
}
