## T-Glass v2 Notification Viewer

This project demonstrates a BLE-based notification viewer for the T-Glass v2 hardware. It integrates ESP-IDF, LVGL, and esp-bsp-generic to display notifications from an iPhone on the T-Glass v2 prism display.

### Features
- **BLE ANCS (Apple Notification Center Service)**: Connects to an iPhone to receive and display notifications.
- **Dynamic UI with LVGL**: Uses LVGL to create a tile-based UI to display notification details.
- **Battery Monitoring**: Tracks and displays the T-Glass v2 battery status.
- **Non-Volatile Storage**: Stores and retrieves data using NVS.
- **Custom Display Driver**: Manages the JD9613 display.

### Requirements

#### Hardware
	•	T-Glass v2: Wearable device with ESP32-S3.

#### Software
	•	ESP-IDF v5.3.2: The development framework for ESP32 series.
	•	esp-bsp-generic v2.0.0: Board support package.
	•	LVGL v9.2.2: Graphics library for creating UIs.

### Setup Instructions

1. Install ESP-IDF
```
    Follow the official ESP-IDF installation guide for your platform.
```
2. Clone the Repository
```
    git clone https://github.com/0015/T-Glass-Applications.git
    cd ancs_app
```
3. Open this folder with VS Code or your IDE

### Folder Structure
```
.
├── main/                       # Main application source code
│   ├── include/                # Header files for all modules
│   │   ├── ancs_app.h
│   │   ├── battery_measurement.h
│   │   ├── ble_ancs.h
│   │   ├── jd9613.h
│   │   ├── nvs_manager.h
│   │   └── t_glass.h
│   │
│   ├── ancs_app.c              # BLE ANCS logic
│   ├── battery_measurement.c   # Battery measurement functions
│   ├── ble_ancs.c              # BLE functionality implementation
│   ├── jd9613.c                # JD9613 display driver
│   ├── main.c                  # Entry point of the project
│   ├── nvs_manager.c           # NVS storage management
│   └── t_glass.c               # T-Glass UI (LVGL)
│
├── CMakeLists.txt              # Build configuration for the ESP-IDF
├── README.md                   # Project overview
└── sdkconfig.thatproject       # ESP-IDF Settings

```

Here’s the CMakeLists.txt configuration file for this project:
```
idf_component_register(SRCS "ancs_app.c" 
                       "nvs_manager.c" 
                       "ble_ancs.c" 
                       "battery_measurement.c" 
                       "main.c" 
                       "jd9613.c" 
                       "t_glass.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES touch_element
                       REQUIRES nvs_flash bt)
```
### Key Functionality

####  BLE ANCS Integration

* The ancs_app.c file handles BLE communication with the iPhone:
	*	Fetches notifications via ANCS.
	*	Processes attributes like title and message.
	*	Provides callbacks to update the UI dynamically.

* Battery Measurement

    * The battery_measurement.c file reads and displays the battery status of the T-Glass v2.

* Display Management

    * The jd9613.c driver manages the JD9613 screen, ensuring notifications and battery status are displayed correctly.

* Future Improvements
	*	Add support for dismissing notifications from the T-Glass v2.
	*	Enhance the UI with custom themes.
	*	Extend BLE functionality to support Android notifications.


### License

This project is licensed under the MIT License.

### Contributing

Contributions are welcome! Feel free to fork the repository and submit a pull request.
