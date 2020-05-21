#include <WiFi.h>
#include <WebServer.h>
#include "./env.h"

WebServer server(80);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  printConnectionInfo();

  server.on("/", sendHeartbeat);

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
