# T-Glass: ESP32-S3-Based Smart Glasses

Welcome to the T-Glass v2 project! This repository provides custom applications for T-Glass v2, a highly programmable and affordable smart glasses platform powered by the ESP32-S3.

---

## Overview
T-Glass v2 is designed for developers and hobbyists who want to explore IoT and wearable technology. With a cost of just $60, it provides a feature-packed environment for building innovative applications.

### Hardware Specifications
| Component        | Description                                      |
|------------------|--------------------------------------------------|
| **Processor**    | ESP32S3-FN4R2 (Flash: 4MB, PSRAM: 2MB QSPI)     |
| **Display**      | JD9613 1.1-inch LTPS AMOLED                     |
| **Sensors**      | Bosch BHI260AP (Smart Sensor Hub)               |
| **RTC**          | PCF85063A                                       |
| **Microphone**   | Integrated microphone for audio input           |
| **Input**        | Touch button for user interaction               |

### Features
- **Programmability**: Develop and deploy your own applications using ESP-IDF.
- **Connectivity**: Leverage ESP32's Wi-Fi and BLE capabilities.
- **Display**: LTPS AMOLED screen.
- **Sensors**: Includes IMU for motion tracking and a real-time clock.
- **Affordability**: Priced at $60, making it accessible to a wide audience.

### Learn More

[![Showcase](https://raw.githubusercontent.com/0015/T-Glass-Applications/refs/heads/main/misc/t_glass_v2.jpg)](https://youtu.be/lb7yXE_X9qE)


Check out the [YouTube video](https://youtu.be/lb7yXE_X9qE) for a detailed overview of the pros and cons of T-Glass v1 & v2, along with additional product insights.

---

## Branch Management
Each branch in this repository corresponds to a specific application or feature. Check the `README.md` in each branch for detailed information:

| Branch Name      | Description                                      |
|------------------|--------------------------------------------------|
| `main`           | General Information for T-Glass.         |
| `ancs_app` | BLE-based notification system for iOS.  |

---

## Contributing
Contributions are welcome! If you'd like to add features or fix bugs, please:
1. Fork this repository.
2. Create a new branch for your changes.
3. Submit a pull request with a detailed description.

---

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.