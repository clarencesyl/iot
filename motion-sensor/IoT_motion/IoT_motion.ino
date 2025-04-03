#include <M5StickCPlus.h>
#include <radar.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define MOTION_SENSOR_PIN 26  // Connect OUT1 to GPIO26

// WiFi credentials
const char* ssid = "help";          // Replace with your Wi-Fi SSID
const char* password = "12678935";  // Replace with your Wi-Fi password

// MQTT broker settings
const char* mqtt_server = "192.168.218.73";  // Replace with your MQTT broker's address
const int mqtt_port = 1883;                 // Default MQTT port
const char* mqtt_topic = "zigbee2mqtt/motion_sensor";  // Topic for motion sensor status

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
    M5.begin();
    pinMode(MOTION_SENSOR_PIN, INPUT);  // Set GPIO26 as input
    Serial.begin(115200);  // Start serial monitor
    
    // Initialize WiFi
    setup_wifi();
    
    // Set up MQTT client
    client.setServer(mqtt_server, mqtt_port);

    // Initialize Serial2 for radar communication
    Serial2.begin(115200, SERIAL_8N1, 36 , 25);  // RX on GPIO36 (S1), TX on GPIO25 (S2) for radar communication
    
    Serial.println("Radar Sensor Initialized...");
}

void setup_wifi() {
    delay(10);
    // Connecting to Wi-Fi
    Serial.println();
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to Wi-Fi");
}

void reconnect() {
    // Loop until we're reconnected to the MQTT broker
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        
        // Create a random client ID
        String clientId = "MotionSensor";
        clientId += String(random(0xffff), HEX);
        
        // Attempt to connect
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            delay(5000);
        }
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();  // Keep the MQTT connection alive

    // Read motion sensor state
    int motionState = digitalRead(MOTION_SENSOR_PIN);  // Read sensor output
    if (motionState == HIGH) {
        Serial.println("Motion Detected!");
        // Publish message to MQTT topic
        client.publish(mqtt_topic, "Motion Detected!");
    } else {
        Serial.println("No Motion");
        // Publish message to MQTT topic
        client.publish(mqtt_topic, "No Motion");
    }

    delay(500);  // Wait for 500ms before checking again
}
