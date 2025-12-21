#include <Arduino.h>  // include the core Arduino library
#include <WiFi.h>     // include the library for WiFi connectivity
#include <WiFiUdp.h>  // include UDP library to listen to packets
#include <esp_wifi.h> // include low-level ESP WiFi driver for CSI

#define BAUDRATE 921600 // serial communication speed

const char *ssid = "CSI_PROJECT_NETWORK";          // the SSID of the TX AP
const char *password = "passwordhardlikeassembly"; // the password of the TX AP
const char *target_ip = "192.168.4.1";             // the IP address of the TX device
const int target_port = 8080;                      // the port where TX sends data

volatile unsigned long packets = 0; // volatile tells the compiler that this variable may change unexpectedly

WiFiUDP udp; // UDP instance to listen to incoming packets

// MAC address of the TX device to filter CSI data
const uint8_t TX_mac[6] = {0x84, 0x1F, 0xE8, 0x67, 0xF6, 0xAD};

// callback function to handle incoming CSI data
void _csi_rx_cb(void *ctx, wifi_csi_info_t *data)
{
    // check if data length is greater than 0 to ensure valid CSI data
    if (data->len > 0)
    {
        // verify that the data is from the expected transmitter by checking the MAC address
        bool match = true;
        for (int i = 0; i < 6; i++)
        {
            if (data->mac[i] != TX_mac[i])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            Serial.print("[CSI DATA] Packet no. ");
            Serial.print(packets++);
            Serial.print(", RSSI ");
            Serial.print(data->rx_ctrl.rssi);
            Serial.print(", Length ");
            Serial.print(data->len);
            Serial.print(", Packet Data: ");

            // we print the payload
            int8_t *ptr = data->buf;

            for (int i = 0; i < data->len; i++)
            {
                Serial.print(ptr[i]);
                if (i < data->len - 1)
                    Serial.print(", ");
            }
            Serial.println();
        }
    }
}

void setup()
{
    // initialize the Serial Monitor
    Serial.begin(BAUDRATE);

    // set WiFi to Station mode (Client) so it can connect to TX
    WiFi.mode(WIFI_STA);

    // disable Power Saving Mode
    esp_wifi_set_ps(WIFI_PS_NONE);

    // initate connection to TX
    WiFi.begin(ssid, password);

    Serial.print("Connecting...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected.");

    // enable CSI functionality on the ESP32
    ESP_ERROR_CHECK(esp_wifi_set_csi(true));

    // configuration of the CSI parameters
    wifi_csi_config_t configuration = {};
    configuration.lltf_en = 1;           // Enable Long Training Field
    configuration.htltf_en = 1;          // Enable High Throughput LTF
    configuration.stbc_htltf2_en = 1;    // Enable Space Time Block Code
    configuration.ltf_merge_en = 1;      // Merge LTF fields for smoother data
    configuration.channel_filter_en = 0; // Disable channel filtering
    configuration.manu_scale = 0;        // Automatic scaling
    configuration.shift = 0;             // No shifting

    // apply the CSI configuration and set the callback function
    ESP_ERROR_CHECK(esp_wifi_set_csi_config(&configuration));
    ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(&_csi_rx_cb, NULL));
}

void loop()
{
    // check if the connection is lost and try to reconnect
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Connection lost. Reconnecting...");
        WiFi.disconnect();
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nReconnected.");
    }
    else
    {
        udp.beginPacket(target_ip, target_port);
        udp.print(0x00); // send a dummy packet to keep the connection alive
        udp.endPacket();
    }

    delay(15);
}