# M5StickCPlus Smoke Detector with MQTT

A portable smoke detector built with the M5StickCPlus and MQ gas sensor, featuring wireless alerts via MQTT.

## Overview

This project transforms an M5StickCPlus and MQ-type gas sensor into a fully functional smoke detector with:

- Real-time smoke/gas detection
- Visual and audible alerts
- Auto-calibration for reliable operation
- Remote monitoring and control via MQTT
- WiFi connectivity

## Hardware Requirements

- M5StickCPlus
- MQ gas sensor (MQ-2, MQ-7, etc.)
- Optional: Passive buzzer for audio alerts
- Optional: USB power adapter for permanent installation

## Wiring

Connect the MQ sensor to the M5StickCPlus as follows:

1. **Power connections:**
   - VCC on sensor → 3.3V on M5StickCPlus
   - GND on sensor → GND on M5StickCPlus

2. **Data connections:**
   - AO (Analog Output) on sensor → G26 on M5StickCPlus
   - DO (Digital Output) on sensor → G36 on M5StickCPlus

3. **Optional buzzer:**
   - Connect positive to G0
   - Connect negative to GND

## Software Setup

### Prerequisites

1. Arduino IDE installed
2. M5StickCPlus library installed
3. PubSubClient library installed (for MQTT)

### Configuration

1. Open the `SmokeDetector.ino` file in Arduino IDE
2. Update the WiFi and MQTT settings at the top of the file:

```cpp
// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";         // Replace with your WiFi SSID
const char* password = "YOUR_WIFI_PASSWORD"; // Replace with your WiFi password

// MQTT Broker settings
const char* mqtt_server = "YOUR_MQTT_BROKER"; // Replace with your MQTT broker address
const int mqtt_port = 1883;                   // Standard MQTT port
const char* mqtt_user = "YOUR_MQTT_USERNAME"; // Replace with your MQTT username or leave empty
const char* mqtt_password = "YOUR_MQTT_PASSWORD"; // Replace with your MQTT password or leave empty
```

3. Compile and upload the code to your M5StickCPlus

## Usage Instructions

### Initial Setup

1. After powering on, the device enters "warmup mode" for 5 seconds
2. The device then auto-calibrates (make sure it's in clean air)
3. Press Button A to exit calibration and start monitoring

### Button Controls

- **Button A (short press)**: Acknowledge/reset alarm or exit calibration
- **Button A (long press)**: Toggle digital sensor on/off
- **Button B (short press)**: Test alarm
- **Button B (long press)**: Recalibrate sensor
- **Both buttons (A+B)**: Enter potentiometer adjustment mode

### Display Information

The screen shows:
- Digital sensor status
- Analog sensor reading
- Current threshold value
- Alarm status
- WiFi/MQTT connection indicator (top right)

### MQTT Integration

#### Topics

The device publishes to these MQTT topics:

- `home/smoke_detector/status`: Regular status updates
- `home/smoke_detector/alert`: Alerts when smoke is detected
- `home/smoke_detector/value`: Current sensor value
- `home/smoke_detector/threshold`: Current threshold value

#### Remote Control

Send commands to `home/smoke_detector/command`:

- `reset`: Reset the alarm
- `calibrate`: Trigger sensor recalibration
- `test`: Run a test alarm
- `threshold:X`: Set custom threshold (replace X with value)
- `digital_on` / `digital_off`: Enable/disable digital sensor

#### Example Commands

```bash
# Test the alarm
mosquitto_pub -h YOUR_BROKER -t home/smoke_detector/command -m "test"

# Set threshold to 2500
mosquitto_pub -h YOUR_BROKER -t home/smoke_detector/command -m "threshold:2500"
```

## Adjusting Sensitivity

If you get false positives or negatives:

1. Use the potentiometer on the MQ sensor to adjust the digital threshold
2. Press both buttons (A+B) to enter adjustment mode
3. Turn the potentiometer until the status shows "OK" in clean air
4. Expose the sensor to a small amount of smoke/gas to test 
5. Press any button to exit adjustment mode

For software sensitivity adjustment, try changing the `thresholdMultiplier` value (default 1.5) in the `calibrateSensor()` function:
- Higher value = less sensitive
- Lower value = more sensitive

## Troubleshooting

- **No WiFi connection**: Check credentials, restart device
- **MQTT not connecting**: Verify broker address and credentials
- **False alarms**: Recalibrate in clean air, adjust potentiometer
- **No response to smoke**: Lower threshold value or adjust potentiometer
- **Screen showing "X"**: WiFi connection lost
- **Screen showing "W"**: WiFi connected but MQTT disconnected

## Home Automation Integration

Compatible with Home Assistant, Node-RED, and other platforms that support MQTT.

Example Home Assistant configuration:

```yaml
binary_sensor:
  - platform: mqtt
    name: "Smoke Detector"
    state_topic: "home/smoke_detector/alert"
    payload_on: "{\"alert\":true"
    payload_off: "clear"
    device_class: smoke

sensor:
  - platform: mqtt
    name: "Smoke Level"
    state_topic: "home/smoke_detector/value"
    unit_of_measurement: "ppm"
```
