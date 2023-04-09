#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <BluetoothSerial.h>

static const char* TAG = "Main";

#define CHANNEL 1
const String rewarderPrefix = "BTHub_";

struct message {
    bool sniffing = false;
};

BluetoothSerial SerialBT;

// Init ESP Now with fallback
void InitESPNow() {
    WiFi.disconnect();
    if (esp_now_init() == 0) {
        Serial.println("ESPNow Init Success");
    } else {
        Serial.println("ESPNow Init Failed");
        ESP.restart();
    }
}

// config AP SSID
void configDeviceAP() {
    String Mac = WiFi.macAddress();
    String SSID = rewarderPrefix + Mac;
    bool result = WiFi.softAP(SSID.c_str(), "Slave_Password", CHANNEL, 0);

    if (!result) {
        Serial.println("AP Config failed.");
    } else {
        Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    }
}

message incomingMessage;
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    ESP_LOGI(TAG, "Got message");
    memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
    String btMessage = (incomingMessage.sniffing) ? "1" : "0";
    SerialBT.println(btMessage);
}

void setup() {
    Serial.begin(115200);
    SerialBT.begin("SniffingController");
    WiFi.mode(WIFI_AP);
    configDeviceAP();
    Serial.print("AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    InitESPNow();
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
// write your code here
}