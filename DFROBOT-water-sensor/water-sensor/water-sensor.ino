#include <M5StickCPlus.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "help";          // Replace with your Wi-Fi SSID
const char* password = "12678935";  // Replace with your Wi-Fi password

// MQTT broker settings
const char* mqtt_server = "192.168.218.73";  // Replace with your MQTT broker's address
const int mqtt_port = 1883;                 // Default MQTT port
const char* mqtt_topic = "zigbee2mqtt/moisture_sensor";  // Topic for motion sensor status

// Moisture sensor pin
#define MOISTURE_SENSOR_PIN 26  // Analog pin where the sensor is connected

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
}

void loop() {
    if (!client.connected()) {
        connectMQTT();
    }
    client.loop();

    int sensorValue = analogRead(MOISTURE_SENSOR_PIN);
    float voltage = sensorValue * (3.3 / 4095.0); 

    // Display readings on screen
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

    // Publish to MQTT
    String payload = "{ \"moisture\": " + String(sensorValue) + ", \"voltage\": " + String(voltage, 2) + " }";
    client.publish(mqtt_topic, payload.c_str());

    Serial.print("Published: ");
    Serial.println(payload);

    delay(1000);
}
