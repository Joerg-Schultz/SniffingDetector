#include <Arduino.h>
#include <driver/i2s.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "I2SMicSampler.h"
#include "NeuralNetwork.h"
#include "AudioProcessor.h"
#include "MasterESPNow.h"

static const char* TAG = "Main";

// Audio Processing settings
#define WINDOW_SIZE 320
#define STEP_SIZE 160
#define POOLING_SIZE 6
#define AUDIO_LENGTH 16000

// Microphone settings
// Hardware left means software right...
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_RIGHT
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_25
#define I2S_MIC_SERIAL_DATA GPIO_NUM_33

// RTOS Queue
static const uint8_t msg_queue_len = 5;
static QueueHandle_t msg_queue;

// Detection globals
int m_number_of_runs = 0;
float m_average_detect_time = 0;
int m_number_of_detections = 0;
I2SSampler *m_sample_provider;
NeuralNetwork *m_nn;
AudioProcessor *m_audio_processor;

static boolean detected = false;

// i2s config for reading from both channels of I2S
i2s_config_t i2sMemsConfigBothChannels = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_MIC_CHANNEL,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0};

// i2s microphone pins
i2s_pin_config_t i2s_mic_pins = {
        .bck_io_num = I2S_MIC_SERIAL_CLOCK,
        .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_SERIAL_DATA};


/**
 * Send any message of the Queue to the BT recipients
 * @param parameters
 */
void sendStatusTask (void* parameters) {
    message currentStatus;
    while (true) {
        // See if there's a message in the queue (do not block)
        if (xQueueReceive(msg_queue, (void *) &currentStatus, 0) == pdTRUE) {
            Serial.println(currentStatus.sniffing);
            sendToBTHubs(currentStatus);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS); //Needed to avoid watchdog alert
    }
}

/**
 * Detect Sniffing
 */
void detectSniffingTask(void* parameters) {
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);
    while (true) {
        // wait for some audio samples to arrive
        uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        if (ulNotificationValue > 0) {
            // time how long this takes for stats
            long start = millis();
            // get access to the samples that have been read in
            RingBufferAccessor *reader = m_sample_provider->getRingBufferReader();
            // rewind by 1 second
            reader->rewind(16000);
            // get hold of the input buffer for the neural network so we can feed it data
            float *input_buffer = m_nn->getInputBuffer();
            // process the samples to get the spectrogram
            m_audio_processor->get_spectrogram(reader, input_buffer);
            // finished with the sample reader
            delete reader;
            // get the prediction for the spectrogram
            float output = m_nn->predict();
            long end = millis();
            // compute the stats
            m_average_detect_time = (end - start) * 0.1 + m_average_detect_time * 0.9;
            m_number_of_runs++;
            // log out some timing info
            if (m_number_of_runs == 50)
            {
                m_number_of_runs = 0;
                Serial.printf("Average detection time %.fms\n", m_average_detect_time);
            }
            // use the same threshold as in training
            if (output > 0.6) {
                m_number_of_detections++;
                if (m_number_of_detections > 2) // TODO Problem when increasing to 1
                {
                    //m_number_of_detections = 0;
                    // detected the wake word in several runs, move to the next state
                    Serial.printf("P(%.2f): I am Sniffing!...\n", output);
                    if (!detected) {
                        detected = true;
                        message currentStatus;
                        currentStatus.sniffing = true;
                        xQueueSend(msg_queue, (void *)&currentStatus, 10);
                    }
                }
            } else {
                if (detected) {
                    detected = false;
                    message currentStatus;
                    currentStatus.sniffing = false;
                    m_number_of_detections = 0;
                    xQueueSend(msg_queue, (void *)&currentStatus, 10);
                }
            }
        }
    }
}


void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting up");
    Serial.printf("Start Total heap: %d\n", ESP.getHeapSize());
    Serial.printf("Start Free heap: %d\n", ESP.getFreeHeap());

    WiFi.mode(WIFI_STA);
    //https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html
    // If the Wi-Fi mode is SoftAP, the ifx should be WIFI_IF_AP.
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    Serial.println("Detector - BTHub Communication");
    Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());

    /** ESP NOW
     * Connection to Reward Stations
     */
    InitESPNow();
    Serial.println("Searching for BTHub(s)");
    SearchBTHubs();


    msg_queue = xQueueCreate(msg_queue_len, sizeof(int));
    BaseType_t xReturnedSendStatus = xTaskCreate(
            sendStatusTask,
            "sendStatusTask",
            1024,
            NULL,
            1,
            NULL
    );
    static const char* createSendTaskMessage = (xReturnedSendStatus == pdPASS) ?
            "SendStatusTask generated\n" : "FAIL: SendStatusTask not generated\n";
    ESP_LOGI(TAG, "%s", createSendTaskMessage);

    m_sample_provider = new I2SMicSampler(i2s_mic_pins, false);

    // Create our neural network
    m_nn = new NeuralNetwork();
    ESP_LOGI(TAG, "Created Neural Net");
    // create our audio processor
    m_audio_processor = new AudioProcessor(AUDIO_LENGTH, WINDOW_SIZE, STEP_SIZE, POOLING_SIZE);
    ESP_LOGI(TAG, "Created audio processor");

    ESP_LOGI(TAG, "Free heap before task: %d\n", ESP.getFreeHeap());

    TaskHandle_t detectSniffingTaskHandle;
    BaseType_t xReturnedSniffingStatus = xTaskCreatePinnedToCore(detectSniffingTask,
                            "detectSniffing Task",
                            4096,
                            NULL,
                            1,
                            &detectSniffingTaskHandle,
                            1);
    static const char* createDetectSniffingTaskMessage = (xReturnedSniffingStatus == pdPASS) ?
                                               "detectSniffing Task generated\n" :
                                               "FAIL: detectSniffing Task not generated\n";
    ESP_LOGI(TAG, "%s", createDetectSniffingTaskMessage);

    ESP_LOGI(TAG,"Free heap after sniffingTask: %d\n", ESP.getFreeHeap());
    m_sample_provider->start(I2S_NUM_0, i2sMemsConfigBothChannels, detectSniffingTaskHandle);
    ESP_LOGI(TAG,"Free heap after start: %d\n", ESP.getFreeHeap());
}

void loop() {
    // Nothing to be done here
}