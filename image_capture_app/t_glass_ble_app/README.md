# Flutter Desktop BLE & Screen Capture App For T-Glass

## 📌 Overview

This Flutter desktop application provides **BLE scanning and connection** using `flutter_blue_plus`, **screen capturing**, and a system tray integration. The application supports **Windows and macOS**, featuring a **theme toggle**, **image processing**, and **persistent settings** using `shared_preferences`.

## 🚀 Features

- **Scan and Connect to BLE Devices** using `flutter_blue_plus`.
- **Send Image Data Over BLE** (Converted to RGB565 format).
- **Screen Capture & Image Processing** with `screen_capturer` and `image`.
- **System Tray Support** to minimize and restore the app.
- **Resizable & Themed Window** using `bitsdojo_window`.
- **Persistent Settings** with `shared_preferences`.

## 🛠️ Dependencies

This project uses the following Flutter packages:

| Package              | Version | Description                                     |
| -------------------- | ------- | ----------------------------------------------- |
| `system_tray`        | ^2.0.3  | Adds system tray functionality.                 |
| `flutter_blue_plus`  | ^1.35.2 | Handles BLE scanning and connections.           |
| `shared_preferences` | ^2.5.2  | Saves user settings persistently.               |
| `bitsdojo_window`    | ^0.1.6  | Customizes window behavior.                     |
| `image`              | ^4.5.2  | Image processing (resizing, format conversion). |
| `screen_capturer`    | ^0.2.3  | Captures screenshots.                           |
| `path_provider`      | ^2.1.5  | Handles file paths.                             |
| `shell_executor`     | ^0.1.6  | Runs system shell commands.                     |

## 📥 Installation

### Prerequisites

- Install **Flutter** (Latest stable version)
- Ensure **Windows** or **macOS** is set up for Flutter desktop development.

### Windows Compatibility
For Windows builds, the BLE package needs to be changed to `flutter_blue_plus_windows`.

## 🖥️ Usage

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

### Theme Toggle

- Click the **Theme Button** in the top-left to switch between **Light** and **Dark Mode**.
- The selected theme is **saved** using `shared_preferences`.

### System Tray

- The app runs in the **system tray** when minimized.
- Click the tray icon to restore the window.

## 🛠️ Troubleshooting

### BLE Connection Issues on macOS

- macOS may **not display some BLE services** due to permission restrictions.
- Ensure `NSBluetoothAlwaysUsageDescription` is set in `Info.plist`:
  ```xml
  <key>NSBluetoothAlwaysUsageDescription</key>
  <string>App requires Bluetooth access</string>
  ```
- Restart **Bluetooth** and **reconnect the device**.

### Image Processing Errors

- Ensure the image is correctly captured before processing.
- If the app crashes, check that the image file path is valid.

## 📜 License

This project is licensed under the **MIT License**.

## 👨‍💻 Author

- Eric Nam
- GitHub: [Eric Nam](https://github.com/0015)

