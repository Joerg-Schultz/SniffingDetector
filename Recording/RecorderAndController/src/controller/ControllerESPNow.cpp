#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "message.h"

const String rewarderPrefix = "Recorder_"; //extract to shared file
static const char* TAG = "ControllerESPNow";

// Global copy of slave
#define NUMSLAVES 20
esp_now_peer_info_t slaves[NUMSLAVES] = {};
int SlaveCnt = 0;

#define CHANNEL 1
#define PRINTSCANRESULTS 0

// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Successfully Rewarded at " : "Fail at ");
    Serial.println(macStr);

}

// Init ESP Now with fallback
void InitESPNow() {
    if (esp_now_init() == ESP_OK) {
        Serial.println("ESPNow Init Success");
        esp_now_register_send_cb(OnDataSent);
    }
    else {
        Serial.println("ESPNow Init Failed");
    }
}
// Check if the slave is already paired with the master.
// If not, pair the slave with master
void manageRewardStations() {
    if (SlaveCnt > 0) {
        for (int i = 0; i < SlaveCnt; i++) {
            const esp_now_peer_info_t *peer = &slaves[i];
            const uint8_t *peer_addr = slaves[i].peer_addr;
            Serial.print("Processing: ");
            for (int ii = 0; ii < 6; ++ii ) {
                Serial.print((uint8_t) slaves[i].peer_addr[ii], HEX);
                if (ii != 5) Serial.print(":");
            }
            Serial.print(" Status: ");
            // check if the peer exists
            bool exists = esp_now_is_peer_exist(peer_addr);
            if (exists) {
                // Slave already paired.
                Serial.println("Already Paired");
            } else {
                // Slave not paired, attempt pair
                esp_err_t addStatus = esp_now_add_peer(peer);
                if (addStatus == ESP_OK) {
                    // Pair success
                    Serial.println("Pair success");
                } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
                    // How did we get so far!!
                    Serial.println("ESPNOW Not Init");
                } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
                    Serial.println("Add Peer - Invalid Argument");
                } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
                    Serial.println("Peer list full");
                } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
                    Serial.println("Out of memory");
                } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
                    Serial.println("Peer Exists");
                } else {
                    Serial.println("Not sure what happened");
                }
                delay(100);
            }
        }
    } else {
        // No slave found to process
        Serial.println("No Recorder found to process");
    }
}

// Scan for slaves in AP mode
void SearchRewardStations() {
    int8_t scanResults = WiFi.scanNetworks();
    //reset slaves
    memset(slaves, 0, sizeof(slaves));
    SlaveCnt = 0;
    Serial.println("");
    if (scanResults == 0) {
        Serial.println("No WiFi devices in AP Mode found");
    } else {
        Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");
        for (int i = 0; i < scanResults; ++i) {
            // Print SSID and RSSI for each device found
            String SSID = WiFi.SSID(i);
            int32_t RSSI = WiFi.RSSI(i);
            String BSSIDstr = WiFi.BSSIDstr(i);

            if (PRINTSCANRESULTS) {
                Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
            }
            delay(10);
            // Check if the current device starts with `Slave`
            if (SSID.indexOf(rewarderPrefix) == 0) {
                // SSID of interest
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(SSID);
                Serial.print(" [");
                Serial.print(BSSIDstr);
                Serial.print("]");
                Serial.print(" (");
                Serial.print(RSSI);
                Serial.print(")");
                Serial.println("");
                // Get BSSID => Mac Address of the Slave
                int mac[6];

                if (6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x%c", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4],
                                &mac[5])) {
                    for (int ii = 0; ii < 6; ++ii) {
                        slaves[SlaveCnt].peer_addr[ii] = (uint8_t) mac[ii];
                    }
                }
                slaves[SlaveCnt].channel = CHANNEL; // pick a channel
                slaves[SlaveCnt].encrypt = 0; // no encryption
                SlaveCnt++;
            }
        }
    }

    if (SlaveCnt > 0) {
        Serial.print(SlaveCnt); Serial.println(" Recorder found, processing..");
        manageRewardStations();
    } else {
        Serial.println("No Recorder Found, trying again.");
    }

    // clean up ram
    WiFi.scanDelete();
}

// send data
void sendToAllRecorder(int startStop) {
    message recorderMessage;
    ESP_LOGI(TAG, "Sending Message");
    recorderMessage.startRecording = startStop;
    for (int i = 0; i < SlaveCnt; ++i) {
        const uint8_t *peer_addr = slaves[i].peer_addr;
        esp_err_t result = esp_now_send(peer_addr, (uint8_t *) &recorderMessage, sizeof(recorderMessage));
        if (result != ESP_OK)
            Serial.println("Send Fail");
    }
}

int getRewardStationsCount() {
    return SlaveCnt;
}
