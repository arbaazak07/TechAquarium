# TechAquarium

## Project Overview
TechAquarium is an IoT-enabled Smart Aquarium system built using the ESP32 microcontroller. It monitors essential water parameters such as temperature, Total Dissolved Solids (TDS), and water level, and automates water refilling, draining, and fish feeding. The system offers real-time updates on an OLED display and allows remote control and monitoring via the Blynk mobile app.

## Features
- **Temperature Monitoring:** Accurate water temperature measurement using DS18B20 sensor.
- **Water Quality Monitoring:** TDS sensor checks water purity.
- **Water Level Detection:** Ultrasonic sensor measures aquarium water level.
- **Automatic Water Management:** Pumps manage refilling and draining based on sensor data.
- **Automatic Fish Feeding:** Servo motor dispenses fish food at scheduled intervals.
- **OLED Display:** Shows sensor data, system status, alerts, and operation mode (Auto/Manual).
- **Alert System:** Buzzer sounds alarms for abnormal conditions like low water level, high temperature, or high TDS.
- **Manual & Auto Modes:** Switch between automatic and manual control through Blynk.
- **Blynk Integration:** Remote monitoring and control via WiFi.

## Components Used
- ESP32 Development Board
- DS18B20 Temperature Sensor
- TDS Sensor
- Ultrasonic Water Level Sensor (HC-SR04)
- SG90 Servo Motor
- OLED Display (SH1106, 128x64)
- Two Water Pumps (Refill and Drain)
- Active Buzzer
- Jumper wires, Breadboard, 9V Power Supply with regulators

## Hardware Setup
- Pin configuration:
  - Ultrasonic Sensor: TRIG → GPIO 13, ECHO → GPIO 12
  - DS18B20 Sensor: GPIO 4
  - Buzzer: GPIO 32
  - Refill Pump (Relay controlled): GPIO 26
  - Drain Pump (Relay controlled): GPIO 27
  - TDS Sensor (Analog): GPIO 35
  - Servo Motor: GPIO 25
- Ensure proper power supply and voltage regulation.

## Software Setup
1. Install Arduino IDE or PlatformIO.
2. Install necessary libraries: OneWire, DallasTemperature, U8g2, Blynk, etc.
3. Update WiFi credentials and Blynk Auth Token in the code or `.env`.
4. Upload the firmware to ESP32.
5. Connect the Blynk app to your project dashboard for remote control.

## How It Works
- Sensors continuously monitor aquarium conditions.
- Refill pump activates when water level falls below threshold.
- Drain pump activates to reduce high TDS levels.
- Servo motor automates fish feeding.
- OLED cycles through status pages (temperature, TDS, water level, feeding, alerts).
- Buzzer alerts on abnormal conditions; alerts can be acknowledged via Blynk.
- Manual control via Blynk overrides automatic mode temporarily.

## Future Enhancements
- Sensor data logging and cloud integration.
- Advanced water quality sensors.
- Push notifications for critical alerts.

## License
MIT License

---

### Contact
For any questions or contributions, contact Arbaj Ansari:

- Email: arbajansari2004@gmail.com  
- LinkedIn: https://www.linkedin.com/in/arbaazak07
