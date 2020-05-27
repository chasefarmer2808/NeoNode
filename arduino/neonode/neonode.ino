#include <WiFi.h>
#include <WebServer.h>
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

WebServer server(80);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, RGB_SETTING + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  pixels.setBrightness(BRIGHTNESS);
  pixels.begin();
  allOff();
  pixels.show();

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  printConnectionInfo();

  server.on("/", sendHeartbeat);
  server.on("/neopixel", setColor);
  server.on("/neopixel/info", sendNeopixelInfo);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

void allOff() {
    for (int i = 0; i < pixels.numPixels(); i++) {
        pixels.setPixelColor(i, 0);
    }
    pixels.show();
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

void setColor() {
  int red, green, blue, alpha, startPixel, endPixel;

  // Parse request query params.
  for (int i = 0; i < server.args(); i++) {
    String currParamName = server.argName(i);
    int currParam = server.arg(i).toInt();

    if (currParamName == "red") {
      red = currParam;
    }
    else if (currParamName == "green") {
      green = currParam;
    }
    else if (currParamName == "blue") {
      blue = currParam;
    }
    else if (currParamName == "alpha") {
      alpha = currParam;
    }
    else if (currParamName == "start") {
      startPixel = currParam;
    }
    else if (currParamName == "end") {
      endPixel = currParam;
    }
  }

  // Set pixels based on range.
  for (int i = startPixel; i <= endPixel; i++) {
    if (SUPPORTS_RGBW) {
      pixels.setPixelColor(i, red, green, blue, alpha);
    }
    else {
      pixels.setPixelColor(i, red, green, blue);
    }
    pixels.show();
  }
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
