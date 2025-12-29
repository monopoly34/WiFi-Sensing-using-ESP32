# ESP32 receiver firmware: CSI data collector

## 1. Theoretical foundation: Signal capture & CSI extraction
While the transmitter (TX) acts as the **illuminator**, the receiver (RX) acts as the **camera**. However, instead of capturing visible light, it captures the distortion of radio waves caused by the phyisical environment. 

### 1.1 What is channel state information?
**Channel state information (CSI)** serves as the foundation for modern sensing techniques (WiFi sensing, LTE sensing, etc.). Its superiority over traditional metrics lies in two key areas:
  1. **Subcarrier granularity**: unlike **RSSI (Received Signal Strength Indicator)**, which provides a single, coarse value for the entire signal, CSI provides physical channel measurements at the **subcarrier level**. This allows us to access this data using the **ESP32 Network Interface Controller (NIC)**.
  2. **Geometric information**: CSI describes the signal propagation process mathematically. It contains **geometric information** of the propagation space. Essentially, the CSI matrix creates a map of the environment. Understanding the relationship between CSI data and these spatial parameters is what makes feature extraction and Machine Learning (ML) prediciton possible.

### 1.2 Signal propagation models:
To understand how the signal travels and interacts with a human body, we rely on two theoretical models:
  - **Ray-tracing model (multipath)**: in typical indoor environments, a signal sent by a transmitter arrives at the receiver via multiple paths due to the reflection of radio waves. Along each path, the signal experiences a certain **attenuation** (loss of strength) and **phase shift** (time delay). The received signal ($V$) is the **superposition** of multiple alias versions of the transmitted signal, expressed mathematically as:
     
 $$V = \sum_{n = 1}^{N} \|V_n \| e^{-j\phi_n}$$
  - $V_n$ and $\phi_n$ are the amplitude and the phase of the $n^{th}$ multipath component.
  - $N$ is the total number of multipath components
  
  **Why using only RSSI fails**: a slight change in one specific path can result in significant constructive or destructive interference, leading to considerable fluctuations in the total RSSI even for a static link.

  - **The scattering model**: the ray-tracing model assumes distinct paths, which may not apply to complex, "rich scattering" environments crowded with furniture or people. The **scattering model** treats objects in the room as **scatterers** that diffuse signals in all directions. The observed CSI is a sum of all portions contributed by:
    - **Static scatterers** like walls, furniture and everything that could be in the background.
    - **Dynamic scatteres** like humans or animals.
  
  Mathematically, we decompose the CSI into the portions contributed by individual scatterers:
  
  $$H(f,t) = \sum_{o \in \Omega_s} H_o(f,t) + \sum_{p \in \Omega_d} H_p(f,t)$$
  
  This model is particularly powerful for **speed-oriented tasks**(like fall detection or gesture recognition) because it statistically links the fluctuations in CSI to the speed of the dynamic scatterer.

### 1.3 The channel state information matrix ($H$)
In the frequency domain, the relationship between the transmitted and received signal is linear. The received signal spectrum $R(f)$ is the multiplication of the transmitted signal spectrum $S(f)$ and the **channel frequency response (CFR)** $H(f)$:

$$R(f) = S(f) \cdot H(f)$$
 - $R(f)$ is the received signal spectrum.
 - $S(f)$ is the original transmitted signal spectrum
 - $H(f)$ is the complex CSI matrix representing the channel.

On commercial devices using **orthogonal frequency-division multiplexing (OFDM)** (like the ESP32 we are using in this project), we cannot capture the infinite continuous wave. Instead, we capture a **sampled version** of $H(f)$ at specific subcarriers. The ESP32 receiver intercepts this data, providing us with a **vector of complex numbers** for each packet.

$$H(f_j) = \|H(f_j)\|e^{\angle H(f_j)}$$
 - $\|H(f_j)\|$ is the **amplitude**.
 - $\angle H(f_j)$ is the **phase** for the $j^{th}$ subcarrier.

## Hardware logic & topology
The RX ESP32 is configured in **station mode (STA)**. Unlike the TX, which is a router/AP, the RX acts like a client device, similar to a phone or a laptop. We connect the RX to the TX network (`CSI_PROJECT_NETWORK`) rather than using "**promiscuous mode**" (a network setting that lets a network interface card (NIC) capture and process all data packets it sees on the network, not just those addressed to it, also known as **sniffing**).

------

## 3. Firmware code analysis

### 3.1 The MAC filter
The air is full of WiFi signals from neighbors, and to prevent "poisoning" our dataset with irrelevant data, we implement a strict filter:

```cpp
const uint8_t TX_mac[6] = { ... }; // target MAC
// inside the loop
if (data->mac[i] !- TX_mac[i]) 
{
  match = false; 
  break;
}
// ...
```

**NOTE**: The variable `TX_mac` is hardcoded, you must replace the bytes with the actual MAC address of your TX. If it does not match, the RX will filter out everything.

### 3.2 The interrupt callback (`_csi_rx_cb`)
This function is the core of the receiver. It is an **interrupt service routine (ISR)**, a special function executed automatically when hardware triggers an event (button press, time overflow, or data arrival), allowing the processor to handle urgent tasks immediately instead of constantly checking, improving efficency and enabling real-time responses.

```cpp
void _csi_rx_cb(void *ctx, wifi_csi_info_t *data)
{ ... }
``` 

### 3.3 Core configuration (`setup`)
The initialization phase prepares the radio to extract physical layer data.

**Network initialization**: First, we configure the ESP32 in **station mode (WIFI_STA)**, setting it up as a client to connect to the transmitter. Additionally, we **disable power saving mode** to ensure minimal latency and consistent packet collection. Finally, we enable the native **CSI hardware driver**, allowing the code to intercept the low-level physical layer information that is normally discarded.

```cpp
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
```

**Configuration flags**: the `wifi_csi_config_t` structure tells the ESP32 which parts of the WiFi packet to analyze:
  - `lltf_en = 1`: enables processing of the **legacy long training field**, a reference signal used to calculate CSI.
  - `htltf_en = 1`: enables **high-throughput LTF**, providing more detailed subcarrier data.
  - `stbc_htltf2_en = 1`: enables **space-time block code (stbc)** detection, a technique used to improve signal reliability.
  - `ltf_merge_en = 1`: enables **smoothing** by merging LTF fields, reducing high-frequency noise.
  - `channel_filter_en = 0`: disables channel filtering to ensure raw, unaltered subcarrier data.
  - `manu_scale = 0`: enables automatic scaling of the CSI data to fit within byte range.

Once the configuration structure is defined, we apply it to the hardware driver and register our **interrupt callback** function.

```cpp
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
```

### 3.4 The active sensing loop (`loop`)
While the CSI data is captured asynchronously via the interrupt callback function, the main loop is used for **active sensing**.

If the WiFi network is silent, there are no radio waves to distort, and therefore no CSI to measure. To ensure a steady stream of data, the RX must force TX to transmit packets. We achieve this by sending **dummy UDP packets** to TX.

The loop performs two main tasks:
 1. **Connection health check**: it continuously monitors the WiFi status. If the connection drops, it halts execution and attempts to reconnect.
 2. **Traffic generation**: it sends a single byte (`0x00`) via UDP to the TX IP address (`192.168.4.1`) every 15 milliseconds. This forces the router (TX) to acknowledge the traffic, filling the air with packets that the CSI driver can analyze.

```cpp
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
```
