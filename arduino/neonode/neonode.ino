#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#include "./env.h"

// Update these values for your specific Neopixel strip.
#define NEOPIXEL_PIN 12
#define NUM_PIXELS 3
#define NUM_ANIMATIONS 2
#define BRIGHTNESS 50
#define RGB_SETTING NEO_GRB

const char *STRIP_TYPE = "strip";
const char *ANIM_DISABLED = "\0";
// Must be unique.
const char* ANIMATIONS[] = {"Rainbow", "Meteor Rain"};
const bool SUPPORTS_RGBW = false;

// Set static IP address.
IPAddress localIP(192, 168, 0, 110);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);

Adafruit_NeoPixel neoPixel = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, RGB_SETTING + NEO_KHZ800);

String activeAnimation = ANIM_DISABLED;
boolean animationChanged = false;

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
  server.on("/animation", HTTP_POST, toggleAnimation);

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
  if (activeAnimation == ANIMATIONS[0]) {
    allOff();
    playRainbowAnimation();
  }
  else if (activeAnimation == ANIMATIONS[1]) {
    allOff();
    playMeteorRainAnimation();
  }
}

bool animationEnabled() {
  return activeAnimation != ANIM_DISABLED;
}

void playRainbowAnimation() {
  uint16_t i, j;
  
  while (animationEnabled()) {
    for (i = 0; i < 256; i++) {
      for (j = 0; j < NUM_PIXELS; j++) {
        neoPixel.setPixelColor(j, Wheel((i + j) & 255));

        if (!animationEnabled() || animationChanged) {
          animationChanged = false;
          return;
        }
      }
      neoPixel.show();
      delay(20);
    }
  }
}

void playMeteorRainAnimation() {
  uint16_t i, j, k;
  byte r, g, b = 0xff;
  byte meteorSize = 10;
  byte meteorTrailDecay = 64;
  boolean meteroRandomDecay = true;
  int speedDelay = 30;

  while (animationEnabled()) {
    for (i = 0; i < NUM_PIXELS * 2; i++) {
      for (j = 0; j < NUM_PIXELS; j++) {
        if (random(10) < 5) {
          if (!animationEnabled() || animationChanged) {
            return;
          }
          fadeToBlack(j, meteorTrailDecay);
        }
      }
  
      for (k = 0; k < meteorSize; k++) {
        if ((i - j < NUM_PIXELS) && (i - j >= 0)) {
          if (!animationEnabled() || animationChanged) {
            return;
          }
          neoPixel.setPixelColor(i - j, r, g, b);
        }
      }
  
      neoPixel.show();
      delay(speedDelay);
    }
  }
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

void toggleAnimation(AsyncWebServerRequest *request) {
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
  if (activeAnimation == animationId) {
    // Disable animation.
    activeAnimation = ANIM_DISABLED;
  }
  else {
    // Enable animation
    activeAnimation = animationId;
    animationChanged = true;
  }
  
  request->send(200, "text/plain", activeAnimation);
}

void sendNeopixelInfo(AsyncWebServerRequest *request) {
  // Size of json object = number of static data + num pixels in neopixel strip + num animations + number of arrays.
  const size_t CAPACITY = 4 + NUM_PIXELS + NUM_ANIMATIONS + 3;
  
  StaticJsonDocument<JSON_OBJECT_SIZE(CAPACITY)> doc;
  doc["num_pixels"] = NUM_PIXELS;
  doc["strip_type"] = STRIP_TYPE;
  doc["supports_rgbw"] = SUPPORTS_RGBW;
  doc["active_animation"] = activeAnimation;

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

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return neoPixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return neoPixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return neoPixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void fadeToBlack(int pixelNum, byte fadeValue) {
  uint32_t oldColor;
  uint8_t r, g, b;
  int value;

  oldColor = neoPixel.getPixelColor(pixelNum);
  r = (oldColor & 0x00ff0000UL) >> 16;
  g = (oldColor & 0x0000ff00UL) >> 8;
  b = (oldColor & 0x000000ffUL);

  r = (r<=10)? 0 : (int) r-(r*fadeValue/256);
  g = (g<=10)? 0 : (int) g-(g*fadeValue/256);
  b = (b<=10)? 0 : (int) b-(b*fadeValue/256);

  neoPixel.setPixelColor(pixelNum, r, g, b);
}
