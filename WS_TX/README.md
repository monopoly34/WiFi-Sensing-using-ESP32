# ESP32 transmitter firmware: WiFi sensing beacon 

## 1. Theoretical foundation: The basic physics of WiFi Sensing
To understand the code, we first have to get a grasp of the physical phenomena it exploits. In this architecture, the transmitter (TX) acts as a **Radio Frequency (RF) Illuminator**.

### 1.1 Electromagnetic wave propagation
The ESP32 radio operates at approximately **2.4 GHz**, which corresponds to a wavelength ($\lambda$) of about **12.5 cm**. When the TX emits a packet, it generates an electromagnetic field that propagates through the environment.

$$\lambda = \frac{c}{f}$$
 - **$\lambda$ (lambda)** = wavelength (0.125m)
 - **c** = speed of light (roughly 300,000,000 $$\frac{m}{s}$$)
 - **f** = frequency (2,400,000,000Hz)

Unlike a laser, WiFi is **omnidirectional** (it transmits signals in all directions). The signal travels from the TX to the receiver (RX) via multiple pathways:
 - **Line of Sight (LoS):** the direct path between antennas (if unobstructed).
 - **Non-Line of Sight (NLoS):** paths created by reflections off walls, floor, furniture, etc.

### 1.2 Multipath Interference and CSI
This phenomenon is known as **Multipath propagation**, a wireless signal phenomenon where a transmitted signal reaches the receiver via **multiple paths (direct, reflected, scattered or refracted)**, hence the name, arriving at different times with varying strengths, causing **constructive/destructive interference**, signal fading and distortion. **Constructive interference** builds waves up, creating larger amplitude when wave peaks meet through in phase, increasing intensity, while **Destructive interference** cancels waves out, reducing amplitude when peaks meet trough out of phase, decreasing intensity.

### 1.3 The human scatterer
Since the human body is roughly **70% water**, and water is a significant absorber and reflector of 2.4 GHz radio waves (this phenomenon is known as **dielectric scattering**, a phenomenon where electromagnetic waves interact with non-conducting materials, also known as **dielectrics** like water, plastics, etc. causing them to scatter in various directions due to the material's polarization), when a person enters the room or moves around, they physically intercept specific signal paths. This alters the **constructive and destructive interference** patterns at the receiver. By keeping the TX signal mathematically constant, any variation in the received CSI matrix can be solely attributed to physical changes in the environment (e.g., human presence). 

------

## 2. Hardware logic & Network topology
The TX ESP32 is configured in **SoftAP (Software Access Point)** mode. It simulates a router but is optimized for **Channel sounding** rather than data transfer. **Channel sounding** is a precise distance measurement technique that analyzes radio signal characteristics (phase, time) across multiple frequencies to determine distance more accurately than signal strength **(RSSI)**.

The firmware targets the IP address `192.168.4.255`. In IPv4, the suffix `.255` represents a **broadcast address**, a special IP address used to send data packets to **all** devices on a local network (LAN) simultaneously, without needing individual addresses, acting like an announcement to everyone nearby. In short, the TX sends packets to the entire subnet without establishing a TCP handshake. This is a better option because it removes the overhead of **acknowledgment packets (ACK)** (ACKs are crucial in networking for reliable data transfer, acting as confirmation signals from a receiver to a sender that data was successfully received, which we do not need in the purpose of this project).

------

## 3. Firmware code analysis
### 3.1 Core configuration (`setup`)
The setup phase is critical for ensuring signal **stationarity**. If the TX radio fluctuates in power, the model will confuse hardware noise with human activity, thus giving a bad prediction.

**Network initialization**: we force a static IP configuration to bypass the DHCP process, ensuring the TX is ready to transmit immediately upon boot. Additionally, by locking the WiFi channel, we ensure the **carrier frequency** remains constant, as CSI varies significantly between channels.

```cpp
WiFi.mode(WIFI_AP);                                                 // sets the WiFi mode to Acces Point only
WiFi.softAPConfig(local_ip, gateway, subnet);                       // apply the custom IP settings
WiFi.softAP(ssid, password, channel, hide_ssid, max_connections);   // starts the AP with our defined settings
```

**Power management**: standard WiFi logic puts the radio to sleep (modem sleep) between packets to save battery. This causes the internal voltage regulator to modulate the radio's transmission power. Variable transmission power introduces artificial variance in the signal amplitude (`X`). `WIFI_PS_NONE` forces the radio to stay **active at full power** continuously. This ensures that any change in the received signal (`Y`) is strictly due to environmental changes (human motion or presence), not hardware power fluctuations.

```cpp
esp_wifi_set_ps(WIFI_PS_NONE);  // disable WiFi Power Saving Mode to ensure stable, low-latency transmission for data
```

### 3.2 The transmission loop (`loop`)

```cpp
udp.beginPacket(address, port); // begin constructing a UDP packet for the broadcast address
udp.print("ADA_IS_SILLY");      // add the payload (content doesn't matter)
udp.endPacket();                // finalize and send the packet
delay(15);                      // wait for 15ms before sending another packet (controls the sampling rate, approximately ~66 packets per second)
```

**Protocol & payload**:
 * **UDP Protocol**: we use User Datagram Protocol because it is connectionless and has minimal header overhead, allowing for the cleanest CSI extraction.
 * **Payload Independence**: the actual text content is irrelevant to the physics. The receiver analyzes the **Physical Layer (PHY)** preamble of the WiFi frame, not the data payload.

**Timing & sampling rate**: 
 * the `delay(15)` command, combined with the code execution time, results in a packet interval of approximately **15-16ms**. This translates to a sampling frequency of **~66Hz**.

> **Nyquist-Shannon Theorem**: the Nyquist Theorem is a fundamental rule in digital signal processing stating that to perfectly reconstruct a continuous signal, you must sample it at a rate **greater than twice the highest frequency present in the signal**, also known as Nyquist rate. Hence, with a sampling rate of approximately 66Hz, we can theoretically detect physical movements occurring at frequencies up to 33Hz. Since human motion (walking, breathing, standing, etc.) is less than **5Hz**, this resolution is more than sufficient for accurate detection.
