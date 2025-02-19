# T-Glass & macOS Flutter App (BLE Image Transfer)

## Overview
This project consists of two applications:
1. **T-Glass Application** (ESP32-S3) – A firmware that runs on T-Glass v2, receives images via BLE, and displays them on the screen using LVGL.
2. **macOS Flutter Application** – A desktop application that captures images, resizes them while maintaining aspect ratio, and sends them to T-Glass via BLE.

The goal of this project is to enable seamless image transfer from the macOS app to T-Glass using BLE communication.


[![Showcase](https://github.com/0015/T-Glass-Applications/blob/main/misc/t_glass_image_capture_app.jpg?raw=true)](https://youtu.be/uTF7egfNgn4)

---

## Project Structure
```
project-folder/
│── main/      			# T-Glass firmware (ESP-IDF & LVGL)
│── t_glass_ble_app/    # Flutter app for macOS (BLE-based image transfer)
│── README.md           # Project documentation
```

---

## T-Glass Application (ESP32-S3)

### Features
- BLE communication using ESP-IDF.
- LVGL-based image rendering.
- Supports displaying images received from macOS.

---

## macOS Flutter Application

### Features
- Captures images and resizes them while maintaining aspect ratio.
- Scans for T-Glass and establishes a BLE connection.
- Transfers the image to T-Glass for display.

---

## BLE Communication
- The Flutter app scans for T-Glass and connects via BLE.
- Once connected, the app compresses and sends image data in packets.
- T-Glass receives the packets, reconstructs the image, and displays it on the screen.

---

## Usage
### Scanning & Connecting to BLE Devices
1. Enter the **Target Device Name** in the text field.
2. Click **Scan** to start discovering BLE devices.
3. If a matching device is found, it **automatically connects**.
4. Once connected, the button changes to **Disconnect**.
5. Click **Disconnect** to disconnect and allow new scans.

### Capturing & Sending Images Over BLE
1. Click **Capture Region** to take a screenshot.
2. The image is resized to **126x126 pixels**.
3. The processed image is converted to **RGB565** format.
4. The app sends the image data over BLE.


## Contribution
Feel free to contribute! Fork the repository, make changes, and submit a pull request.

---

## License
This project is licensed under the MIT License.

---

## Author

- Eric Nam
- GitHub: [Eric Nam](https://github.com/0015)