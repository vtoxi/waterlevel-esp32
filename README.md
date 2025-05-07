# Water Level Indicator

![PlatformIO](https://img.shields.io/badge/platformio-esp32-blue)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

A smart water level monitoring system for ESP32 with a modern web dashboard, real-time animated tank, persistent logging, and easy configuration.

---

## Features
- Real-time water level monitoring with animated tank visualization
- Responsive web dashboard (mobile & desktop)
- WiFi, MQTT, tank, sensor, display, network, alert, and device settings
- Persistent log file with download/view options
- Help page with wiring diagram and connection guide
- Factory reset, OTA update, and more

---

## Screenshots

| Dashboard | Logs | Help & Diagram |
|-----------|------|---------------|
| ![Dashboard](docs/screenshots/dashboard.png) | ![Logs](docs/screenshots/logs.png) | ![Help](docs/screenshots/help.png) |

| WiFi | MQTT | Tank | Sensor |
|------|------|------|--------|
| ![WiFi](docs/screenshots/wifi.png) | ![MQTT](docs/screenshots/settings_mqtt.png) | ![Tank](docs/screenshots/settings_tank.png) | ![Sensor](docs/screenshots/settings_sensor.png) |

| Display | Network | Alerts | Device |
|---------|---------|--------|--------|
| ![Display](docs/screenshots/settings_display.png) | ![Network](docs/screenshots/settings_network.png) | ![Alerts](docs/screenshots/settings_alerts.png) | ![Device](docs/screenshots/settings_device.png) |

| Connected |
|-----------|
| ![Connected](docs/screenshots/connected.png) |

---

## Hardware Required
- ESP32 Development Board
- JSN-SR04T (or HC-SR04) Ultrasonic Sensor
- MAX7219 8x8 LED Matrix Display
- Buzzer (optional)
- LED (optional)
- Wires, breadboard, power supply

---

## Wiring Diagram

![Wiring Diagram](docs/screenshots/diagram.png)

---

## Getting Started

### Prerequisites
- [PlatformIO](https://platformio.org/)
- Python 3 (for upload script)

### Setup
```sh
git clone https://github.com/yourusername/water-level-indicator.git
cd water-level-indicator
# Upload web files to ESP32 LittleFS
python3 upload_files.py
# Build and upload firmware
platformio run -t upload
```

### WiFi Setup
- Connect to the ESP32 AP if not already on your network
- Open the web interface (see serial output for IP)
- Configure WiFi and other settings

---

## Web Interface Features
- **Dashboard:** Animated tank, quick access widgets
- **Logs:** Real-time and persistent logs, download/view options
- **Settings:** WiFi, MQTT, tank, sensor, display, network, alerts, device
- **Help:** Connection guide, wiring diagram

---

## Configuration
- All settings are saved in non-volatile storage
- Only WiFi/Network changes require a reboot
- All other settings apply instantly

---

## Logging
- Logs are stored in `/logs.txt` on LittleFS
- View/download logs from the web interface
- Log rotation and clear options available

---

## Contributing
Pull requests and suggestions welcome! Please open an issue for major changes.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

## Credits
- Developed by Faisal S. ([github.com/vtoxi](https://github.com/vtoxi))
- Powered by ESP32, PlatformIO, and open source libraries 