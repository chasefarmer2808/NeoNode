#include <WiFi.h>

const char* ssid = "bmore_wifi";
const char* password = "XmegaA1USphs2014";

void setup() {
    Serial.begin(115200);
    scanNetworks();
    connectToNetwork();

    Serial.println(WiFi.localIP());
    WiFi.disconnect(true);
}

void scanNetworks() {
    int numberOfNetworks = WiFi.scanNetworks();
    Serial.println(numberOfNetworks);

    for (int i = 0; i < numberOfNetworks; i++) {
        Serial.println(WiFi.SSID(i));
    }
}

void connectToNetwork() {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Establishing connection...");
    }

    Serial.println("Connected!");
}
