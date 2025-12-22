#include <Arduino.h>  // include the core Arduino library
#include <WiFi.h>     // include the library for WiFi connectivity
#include <WiFiUdp.h>  // include the library to handle UDP protocol
#include <esp_wifi.h> // include the native ESP32 WiFi driver for low-level control

#define BAUDRATE 921600 // define the speed for serial communication with the computer

const char *ssid = "CSI_PROJECT_NETWORK";          // the name of the WiFi network we are creating (SSID name)
const char *password = "passwordhardlikeassembly"; // the password required to connect to this network (SSID password)
const char *address = "192.168.4.255";             // the IP address where we send packets (the .255 means 'send to everyone')
const int channel = 6;                             // the specific radio frequency channel (1-13) to use
const bool hide_ssid = false;                      // to disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int max_connections = 4;                     // maximum simultaneous connected clients on the AP
const int port = 8080;                             // the destination port for the UDP packets

IPAddress local_ip(192, 168, 4, 1); // the static IP Address we assign to this ESP32 (Transmitter)
IPAddress gateway(192, 168, 4, 1);  // the gateway address (same as IP)
IPAddress subnet(255, 255, 255, 0); // the subnet mask (defines the size of the network)

WiFiUDP udp; // create a WiFiUDP object instance to handle sending data

void setup()
{
    Serial.begin(BAUDRATE); // initialize the Serial Monitor to view debug messages

    WiFi.mode(WIFI_AP);     // set the WiFi mode to Access Point(AP) only

    WiFi.softAPConfig(local_ip, gateway, subnet);                     // apply the custom IP settings
    WiFi.softAP(ssid, password, channel, hide_ssid, max_connections); // start the AP with our defined settings

    esp_wifi_set_ps(WIFI_PS_NONE); // disable WiFi Power Saving Mode to ensure stable, low-latency transmission for CSI

    /* get and print the IP Address to the Serial Monitor for verification
    IPAddress IP = WiFi.softAPIP();
    Serial.print("TX AP started. IP Address: ");
    Serial.println(IP);*/
}

void loop()
{
    udp.beginPacket(address, port); // begin constructing a UDP packet for the broadcast address
    udp.print("ADA_IS_SILLY");      // add the payload (content doesn't matter)
    udp.endPacket();                // finalize and send the packet
    delay(15);                      // wait for 15ms before sending another packet (controls the sampling rate, approximately ~66 packets per second)
}