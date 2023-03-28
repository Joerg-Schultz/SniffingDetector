#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "message.h"

static const char* TAG = "MasterESPNow";

const String btHubPrefix = "BTHub_"; //extract to shared file

// Global copy of slave
#define NUMSLAVES 20
esp_now_peer_info_t slaves[NUMSLAVES] = {};
int SlaveCnt = 0;

#define CHANNEL 1
#define PRINTSCANRESULTS 1

// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Send message to BTHub " : "Fail at ");
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
void manageBTHubs() {
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
        Serial.println("No BTHub found to process");
    }
}

// Scan for slaves in AP mode
void SearchBTHubs() {
    int8_t scanResults = WiFi.scanNetworks();
    //reset slaves
    memset(slaves, 0, sizeof(slaves));
    SlaveCnt = 0;
    Serial.println("");
    if (scanResults == 0) {
        ESP_LOGI(TAG,"No WiFi devices in AP Mode found");
    } else {
        ESP_LOGI(TAG, "Found %d devices", scanResults);
        for (int i = 0; i < scanResults; ++i) {
            // Print SSID and RSSI for each device found
            String SSID = WiFi.SSID(i);
            int32_t RSSI = WiFi.RSSI(i);
            String BSSIDstr = WiFi.BSSIDstr(i);

            if (PRINTSCANRESULTS) {
                Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
            }
            delay(10);
            // Check if the current device starts with the Prefix
            if (SSID.indexOf(btHubPrefix) == 0) {
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
        ESP_LOGI(TAG, "%d BTHubs(s) found", SlaveCnt);
        manageBTHubs();
    } else {
        ESP_LOGI(TAG,"No BTHub Found, Moving on");
    }

    // clean up ram
    WiFi.scanDelete();
}

// send data
void sendToBTHubs(message currentStatus) {
    for (int i = 0; i < SlaveCnt; ++i) {
        esp_now_peer_info_t btStation = slaves[i];
        const uint8_t *peer_addr = btStation.peer_addr;
        esp_err_t result = esp_now_send(peer_addr, (uint8_t *) &currentStatus, sizeof(currentStatus));
        if (result != ESP_OK) {
            Serial.println("Send Fail");
        } else {
            Serial.println("Successfully send!");
        }
    }
}

int getNumberBTHubs() {
    return SlaveCnt;
}
