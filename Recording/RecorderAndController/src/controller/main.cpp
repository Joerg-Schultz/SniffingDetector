#include <WiFi.h>
#include <esp_wifi.h>
#include "BluetoothSerial.h"
#include "ControllerESPNow.h"

BluetoothSerial SerialBT;

void setup() {
    Serial.begin(115200);
    SerialBT.begin("TWTController");
    WiFi.mode(WIFI_STA);
    //https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html
    // If the Wi-Fi mode is SoftAP, the ifx should be WIFI_IF_AP.
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    Serial.println("Controller - Recorder Communication");
    Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());

    /** ESP NOW
     * Connection to Reward Stations
     */
    InitESPNow();
    Serial.println("Detecting Recorder");
    SearchRewardStations();
}

void loop() {
    if (SerialBT.available()) {
        sendToAllRecorder(SerialBT.read());
    }
}