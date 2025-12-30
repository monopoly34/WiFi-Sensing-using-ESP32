# ESP32 WiFi Sensing (offline and real-time)

## Overview
Hey, this is a project about utilizing the **Channel State Information (CSI)** embedded in standard WiFi signals to detect human activity in a given environment. By applying Machine Learning algorithms to the CSI data collected by the ESP32 microcontrollers, the model can predict whether a room is empty or occupied with high accuracy.

This project consists of two main components:
  - **Offline training and testing:**
    * gathering data and storing in a CSV format
    * filtering the collected data
    * visualizing the data using a graph
    * training and testing a model using Random Forest Machine Learning algorithm to get its accuracy
    * visualizing the model's mistakes with a confusion matrix
  - **Real-time detection:**
    * using slightly optimized code, we train the model using the previously filtered dataset
    * we run the trained model on live data streams for instant classification
        
## Hardware used in this project:
  - **2x ESP32 microcontrollers** (for now, will add more in the future):
    * one acts as the **Transmitter (TX)** (router simulation).
    * one acts as the **Receiver (RX)** (CSI collector).
  - **external battery** to power the **TX** microcontroller remotely.
  - **laptop/desktop computer** for data logging, model training, and running the real-time Python script.
  - **Micro_USB cables** to connect your desired device to the microcontrollers, thus giving you the ability to upload code on them.

## Software and other tools used in this project:
  - **Visual Studio Code**
  - **Python 3.14.2** with the following libraries installed:
    * **scikit-learn**
    * **matplotlib**
    * **pyserial**
    * **seaborn**
    * **pandas**
    * **numpy**
    * **time**
    * **csv**
    * **re**
  - **PlatformIO IDE VSCode extension** for flashing ESP32 firmware.
  - **Jupyter Notebooks VSCode extension** for data analysis.
  - **Drivers for the ESP32** so it can be detected when connecting to a port.

## Project structure
```text
WiFi Sensing using ESP32
┣ 📂 WS_RX
┃ ┣ 📂 src
┃ ┃ ┣ main.cpp      # source code for RX
┃ ┃ ┗ README.md     # short documentation for RX
┃ ┗ platformio.ini  # configuration file for RX
┣ 📂 WS_TX
┃ ┣ 📂 src
┃ ┃ ┣ main.cpp      # source code for TX
┃ ┃ ┗ README.md     # short documentation for TX
┃ ┗ platformio.ini  # configuration file for TX
┣ csi_dataset.csv                    # raw dataset
┣ filtered_csi_dataset.csv           # filtered dataset
┣ csv_extraction.ipynb               # source code for the offline training and testing
┗ csv_extraction_real_time.ipynb     # source code for the real-time testing
```

## Results (as of 12.22.2025)
The model has been trained on varying environmental states and performes quite well for its reduced complexity, with an accuracy of **~98%** in **offline** testing and an accuracy of **~91%** in **real-time** testing.

## How to use
  1. Flash the `WS_TX` code to one ESP32 and `WS_RX` to the other using PlatformIO.
  2. Connect the RX ESP32 to the PC using a Micro-USB or USB-C, depending on your ESP32 board model.
  3. Check the **COM** port in **Device Manager** and update it in the Python script (`COM_PORT` variable).
  4. Run either of the two scripts.

## Future improvements
  - [x] optimize the real-time detection model.
  - [x] fix the inaccuracy of the model when standing still in front of the RX.
  - [x] implement hysteresis logic to prevent output flickering.
  - [x] implement Savitzky-Golay filter to reduce noise and smooth data.
  - [x] implement principal component analysis (PCA) to reduce feature dimensionality.
  - [x] implement auto-calibration logic to subtract static environment (furniture, objects, etc.) baseline.
  - [ ] implement deep learning models (LSTM/CNN) for gesture recognition.
  - [ ] add more ESP32 receivers to create a mesh sensing network.

### Thanks to every Stackoverflow thread, Github repository, Youtube tutorial and article on Google that helped me learn and make this project!
------
*Project by [monopoly34]*
