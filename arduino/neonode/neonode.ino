#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
//#include <Adafruit_NeoPixel.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <ArduinoJson.h>

#include "./env.h"

// Update these values for your specific Neopixel strip.
#define NEOPIXEL_PIN 12
#define NUM_PIXELS 120
#define NUM_ANIMATIONS 2
#define BRIGHTNESS 50
#define RGB_SETTING NEO_GRBW

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

//Adafruit_NeoPixel neoPixel = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, RGB_SETTING + NEO_KHZ800);
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> neoPixel(NUM_PIXELS, NEOPIXEL_PIN);
NeoGamma<NeoGammaTableMethod> colorGamma;

NeoPixelAnimator animations(1);
MyAnimationState animationState[1];

RgbColor black(0);

String activeAnimation = ANIM_DISABLED;
boolean animationChanged = false;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
//  neoPixel.setBrightness(BRIGHTNESS);
//  neoPixel.begin();
  neoPixel.Begin();
  allOff();
//  neoPixel.show();
  neoPixel.Show();

  animations.StartAnimation(0, 300, playRainbowAnimation);

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
  server.on("/pixel", HTTP_POST, setPixel);
  server.on("/pixels", HTTP_POST, fillPixels);
  server.on("/neopixel/info", HTTP_GET, sendNeopixelInfo);
  server.on("/animation", HTTP_POST, toggleAnimation);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
//  processAnimation();
  animations.UpdateAnimations();
  neoPixel.Show();
}

void allOff() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        neoPixel.SetPixelColor(i, black);
    }
    neoPixel.Show();
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
  int fadeVal=0, fadeMax=100;

  // Hue of first pixel runs 'rainbowLoops' complete loops through the color
  // wheel. Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to rainbowLoops*65536, using steps of 256 so we
  // advance around the wheel at a decent clip.
  for(uint32_t firstPixelHue = 0; firstPixelHue < 5*65536;
    firstPixelHue += 256) {

    for(int i=0; i<NUM_PIXELS; i++) { // For each pixel in strip...

      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      uint32_t pixelHue = firstPixelHue + (i * 65536L / NUM_PIXELS);

      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the three-argument variant, though the
      // second value (saturation) is a constant 255.
//      neoPixel.SetPixelColor(i, neoPixel.gamma32(neoPixel.ColorHSV(pixelHue, 255, 255)));

      if (!animationEnabled() || animationChanged) {
        return;
      }
    }

    neoPixel.Show();
    delay(30);

    if(firstPixelHue < 65536) {                              // First loop,
      if(fadeVal < fadeMax) fadeVal++;                       // fade in
    } else if(firstPixelHue >= ((5-1) * 65536)) { // Last loop,
      if(fadeVal > 0) fadeVal--;                             // fade out
    } else {
      fadeVal = fadeMax; // Interim loop, make sure fade is at max
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
          neoPixel.SetPixelColor(i - j, RgbColor(r, g, b));
        }
      }
  
      neoPixel.Show();
      delay(speedDelay);
    }
  }
}

void sendHeartbeat(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", "Success");
}

void setPixel(AsyncWebServerRequest *request) {
  // Make sure body is not empty.
  if (!request->hasParam("index", true)) {
    return request->send(400, "text/plain", "Pixel index required.");
  }

  if (!request->hasParam("color", true)) {
    return request->send(400, "text/plain", "Pixel color required.");
  }

  int index = request->getParam("index", true)->value().toInt();
  uint32_t color = request->getParam("color", true)->value().toInt();

  Serial.println(index);
  Serial.println(color);

  neoPixel.SetPixelColor(index, getRgbColor(color));
  neoPixel.Show();

  request->send(200);
}

void fillPixels(AsyncWebServerRequest *request) {
  // Make sure body is not empty.
  if (!request->hasParam("color", true)) {
    return request->send(400, "text/plain", "Pixel color required.");
  }

  uint32_t color = request->getParam("color", true)->value().toInt();

  // Fill the strip in a little animation.
  for (int i = 0; i < NUM_PIXELS; i++) {
    neoPixel.SetPixelColor(i, getRgbColor(color));

    neoPixel.Show();
    delay(5);
  }

//  neoPixel.Show();
  
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
    pixels.add(neoPixel.Pixels()[i]);
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
RgbColor Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void fadeToBlack(int pixelNum, byte fadeValue) {
//  uint32_t oldColor;
//  uint8_t r, g, b;
//  int value;
//
//  oldColor = neoPixel.GetPixelColor(pixelNum);
//  r = (oldColor & 0x00ff0000UL) >> 16;
//  g = (oldColor & 0x0000ff00UL) >> 8;
//  b = (oldColor & 0x000000ffUL);
//
//  r = (r<=10)? 0 : (int) r-(r*fadeValue/256);
//  g = (g<=10)? 0 : (int) g-(g*fadeValue/256);
//  b = (b<=10)? 0 : (int) b-(b*fadeValue/256);
//
//  neoPixel.SetPixelColor(pixelNum, RgbColor(r, g, b));
}

RgbwColor getRgbColor(int color) {
  uint8_t r = (color >> 16) & 255;
  uint8_t g = (color >> 8) & 255;
  uint8_t b = color & 255;
  Serial.println(r);
  Serial.println(g);
  Serial.println(b);
  return RgbwColor(r, g, b, 0);
}
