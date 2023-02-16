#include <Arduino.h>
#include <stdio.h>
#include "I2SMEMSSampler.h"
#include "SDCard.h"
#include "WAVFileWriter.h"
#include "config.h"
#include <WiFi.h>
#include <esp_now.h>
#include "message.h"


static const char* TAG = "Main";

#define CHANNEL 1
const String rewarderPrefix = "Recorder_";

I2SSampler *input;
TaskHandle_t recordHandle = NULL;
SDCard* sdCard;

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
void record(void * parameters)
{
    uint32_t notifiedValue;
    while(true) {
        if (xTaskNotifyWait(0, 0, &notifiedValue, portMAX_DELAY) == pdTRUE) { //xTaskNotifyWait returns true if it was notified
            int16_t *samples = (int16_t *) malloc(sizeof(int16_t) * 1024);
            ESP_LOGI(TAG, "Start recording");
            input->start();
            // open the file on the sdcard
            int counter = sdCard->getFileCount();
            std::string fileNameConstructor = "/sdcard/test_";
            fileNameConstructor += std::to_string(counter);
            fileNameConstructor += ".wav";
            ESP_LOGI(TAG, "Filename: %s", fileNameConstructor.c_str());
            const char *fileName = fileNameConstructor.c_str();
            FILE *fp = fopen(fileName, "wb");
            // create a new wave file writer
            WAVFileWriter *writer = new WAVFileWriter(fp, input->sample_rate());
            // keep writing until the user releases the button
            //while (! xTaskNotifyWait(0, 0, &notifiedValue, portMAX_DELAY)) { //TODO this does not work as supposed
            while (! xTaskNotifyWait(0, 0, &notifiedValue, 0)) { //TODO this does not work as supposed
                int samples_read = input->read(samples, 1024);
                int64_t start = esp_timer_get_time();
                writer->write(samples, samples_read);
                int64_t end = esp_timer_get_time();
                //ESP_LOGI(TAG, "Wrote %d samples in %lld microseconds", samples_read, end - start);
            }
            // stop the input
            input->stop();
            // and finish the writing
            writer->finish();
            fclose(fp);
            delete writer;
            free(samples);
            ESP_LOGI(TAG, "Finished recording");
        }
    }
}

message incomingMessage;
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    ESP_LOGI(TAG, "Got message");
    memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
    if (incomingMessage.startRecording == 1) {
        // notify to start
        ESP_LOGI(TAG, "NotifyStart");
        xTaskNotify(recordHandle, 1, eSetValueWithoutOverwrite);
    } else {
        // notify to stop
        ESP_LOGI(TAG, "NotifyStop");
        xTaskNotify(recordHandle, 0, eSetValueWithoutOverwrite);
    }
}

void setup()
{
    Serial.begin(115200);
    // Added
    pinMode(SD_DATA0, INPUT_PULLUP);
    pinMode(SD_DATA1, INPUT_PULLUP);
    pinMode(SD_DATA4, INPUT_PULLUP);
    pinMode(SD_DATA3, INPUT_PULLUP);
    pinMode(SD_CMD, INPUT_PULLUP);

    WiFi.mode(WIFI_AP);
    configDeviceAP();
    Serial.print("AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    InitESPNow();
    esp_now_register_recv_cb(OnDataRecv);

    ESP_LOGI(TAG, "Mounting SDCard on /sdcard");
    sdCard = new SDCard("/sdcard");

    ESP_LOGI(TAG, "Creating microphone");
    input = new I2SMEMSSampler(I2S_NUM_0, i2s_mic_pins, i2s_mic_Config);

    ESP_LOGI(TAG, "Switch on LED");
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    xTaskCreate(record,
                "Record",
                4096,
                NULL,
                0,
                &recordHandle);
}

void loop()
{
}