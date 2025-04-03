#include <M5StickCPlus.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "help";          // Replace with your Wi-Fi SSID
const char* password = "12678935";  // Replace with your Wi-Fi password

// MQTT broker settings
const char* mqtt_server = "192.168.218.73";  // Replace with your MQTT broker's address
const int mqtt_port = 1883;                  // Default MQTT port
const char* mqtt_topic = "zigbee2mqtt/moisture_sensor";  // Topic for moisture sensor status

// Moisture sensor pin (Use GPIO33 instead of GPIO26)
#define MOISTURE_SENSOR_PIN 33  

WiFiClient espClient;
PubSubClient client(espClient);

void connectWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
}

void connectMQTT() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect("M5StickCPlus_Client")) {
            Serial.println("Connected to MQTT Broker!");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Trying again in 5 seconds...");
            delay(5000);
        }
    }
}

void setup() {
    M5.begin();  
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);

    Serial.begin(115200);

    connectWiFi();
    client.setServer(mqtt_server, mqtt_port);
    connectMQTT();

    // Set up ADC properly
    analogReadResolution(12); // ESP32 ADC has 12-bit resolution (0 - 4095)
    analogSetPinAttenuation(MOISTURE_SENSOR_PIN, ADC_11db); // Supports up to 3.3V
}

void loop() {
    if (!client.connected()) {
        connectMQTT();
    }
    client.loop();

    int sensorValue = analogRead(MOISTURE_SENSOR_PIN);
    float voltage = sensorValue * (3.3 / 4095.0);

    // Debugging output
    Serial.print("Raw ADC Value: ");
    Serial.print(sensorValue);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 2);
    Serial.println("V");

    // Display readings on M5StickC Plus screen
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(10, 20);
    M5.Lcd.print("Moisture Level:");
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.print(sensorValue);

    M5.Lcd.setCursor(10, 80);
    M5.Lcd.print("Voltage: ");
    M5.Lcd.setCursor(10, 110);
    M5.Lcd.print(voltage, 2);
    M5.Lcd.print("V");

    // Publish data to MQTT
    String payload = "{ \"moisture\": " + String(sensorValue) + ", \"voltage\": " + String(voltage, 2) + " }";
    client.publish(mqtt_topic, payload.c_str());

    Serial.print("Published: ");
    Serial.println(payload);

    delay(1000);
}
