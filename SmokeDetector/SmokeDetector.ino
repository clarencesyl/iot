#include <M5StickCPlus.h>
#include <WiFi.h>
#include <PubSubClient.h>  
#include <ArduinoJson.h>   

// WiFi credentials
const char* ssid = "help";         
const char* password = "12678935"; 

// MQTT Broker settings
const char* mqtt_server = "192.168.218.73"; 
const int mqtt_port = 1883;                 
const char* mqtt_user = "";                 
const char* mqtt_password = "";             
const char* client_id = "SmokeSensorM5";    

// MQTT Topic 
const char* mqtt_topic = "zigbee2mqtt/gas_sensor";

const int mqAnalogPin = 36; 
const int mqDigitalPin = 26; 
const int buzzerPin = 0; 

// Smoke detection parameters
int analogThreshold = 2000; // Initial threshold (will be calibrated)
int sensorValue = 0; 
int baselineValue = 0; 
bool digitalValue = false; 
bool digitalEnabled = true; 
bool isCalibrationMode = true; 
bool alarmActive = false; 
const int readingInterval = 500; 

// Display colors
const uint16_t COLOR_BACKGROUND = BLACK;
const uint16_t COLOR_TEXT = WHITE;
const uint16_t COLOR_WARNING = RED;
const uint16_t COLOR_SAFE = GREEN;
const uint16_t COLOR_CAUTION = YELLOW;

// Timing variables
unsigned long lastReadingTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastMqttPublishTime = 0;
unsigned long mqttPublishInterval = 5000; 
unsigned long uptime = 0;
unsigned long lastWifiCheckTime = 0;
unsigned long wifiCheckInterval = 30000; 

// Sensor data history
const int historySize = 10;
int valueHistory[historySize];
int historyIndex = 0;

// WiFi and MQTT client instances
WiFiClient espClient;
PubSubClient mqttClient(espClient);


void updateDisplay();
void checkSmoke();
void triggerAlarm();
void calibrateSensor();
int getAverageReading();
void flashMessage(const char* line1, const char* line2, uint16_t bgColor, uint16_t textColor, int flashes);
void potentiometerAdjustmentMode();
void setupWifi();
void reconnectMqtt();
void publishSensorData(const char* state = NULL);
void mqttCallback(char* topic, byte* payload, unsigned int length);

void setup() {
  // Initialize 
  M5.begin();
  M5.Axp.ScreenBreath(15); 
  M5.Lcd.setRotation(3); 
  M5.Lcd.fillScreen(COLOR_BACKGROUND);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(COLOR_TEXT);
  M5.Lcd.println("Smoke Detector");
  M5.Lcd.println("Initializing...");
  Serial.begin(115200);
  Serial.println("M5StickC Plus Smoke Detector Starting...");
  
  pinMode(mqAnalogPin, INPUT);
  pinMode(mqDigitalPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
  M5.Axp.SetLDO2(true); // Enable LDO2 for model 1.1
  
  for (int i = 0; i < historySize; i++) {
    valueHistory[i] = 0;
  }
  
  setupWifi();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  
  // Attempt to connect to MQTT broker
  if (WiFi.status() == WL_CONNECTED) {
    reconnectMqtt();
  }
  
  M5.Lcd.fillScreen(COLOR_BACKGROUND);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.println("Warming up...");
  M5.Lcd.println("Please wait");
  
  // Shorter warmup for testing (5 seconds)
  for (int i = 5; i > 0; i--) {
    M5.Lcd.fillRect(0, 60, M5.Lcd.width(), 20, COLOR_BACKGROUND);
    M5.Lcd.setCursor(0, 60);
    M5.Lcd.print(i);
    M5.Lcd.print(" seconds...");
    delay(1000);
  }
  calibrateSensor();
  
  M5.Lcd.fillScreen(COLOR_BACKGROUND);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("Smoke Detector");
  M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
  
  Serial.println("M5StickC Plus Smoke Detector Ready");
  
  // Publish initial status
  if (mqttClient.connected()) {
    publishSensorData("online");
  }
}

void loop() {
  // Update buttons state 
  M5.update();
  
  if (isCalibrationMode && M5.BtnA.isPressed()) {
    M5.Lcd.fillCircle(M5.Lcd.width() - 10, 10, 5, GREEN);
  }
  
  // Check WiFi and MQTT connections
  unsigned long currentMillis = millis();
  if (currentMillis - lastWifiCheckTime >= wifiCheckInterval) {
    lastWifiCheckTime = currentMillis;
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost. Reconnecting...");
      setupWifi();
    }
    
    if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) {
      Serial.println("MQTT connection lost. Reconnecting...");
      reconnectMqtt();
    }
  }
  
  // Process MQTT messages
  if (mqttClient.connected()) {
    mqttClient.loop();
  }
  
  // Track uptime (might remove to save resources)
  uptime = millis() / 1000;
  
  // Read sensor at specified intervals
  if (millis() - lastReadingTime >= readingInterval) {
    lastReadingTime = millis();
    sensorValue = analogRead(mqAnalogPin);
    digitalValue = digitalRead(mqDigitalPin);
    
    // Update history array
    valueHistory[historyIndex] = sensorValue;
    historyIndex = (historyIndex + 1) % historySize;
    
    Serial.print("Analog: ");
    Serial.print(sensorValue);
    Serial.print(", Digital: ");
    Serial.print(digitalValue);
    Serial.print(", Threshold: ");
    Serial.println(analogThreshold);
    
    if (millis() - lastDisplayTime >= 1000) {
      updateDisplay();
      lastDisplayTime = millis();
    }
    
    // Check for smoke detection (skip if in calibration mode)
    if (!isCalibrationMode) {
      checkSmoke();
    }
    
    // Publish sensor data via MQTT at regular intervals
    if (millis() - lastMqttPublishTime >= mqttPublishInterval) {
      lastMqttPublishTime = millis();
      if (mqttClient.connected()) {
        publishSensorData();
      }
    }
  }
  
  // Button A: Reset/Acknowledge alarm or exit calibration
  if (M5.BtnA.wasPressed()) {
    Serial.println("Button A pressed!");
    
    // Reset alarm state
    digitalWrite(buzzerPin, LOW);
    alarmActive = false;
    
    // Publish alarm clear via MQTT
    if (mqttClient.connected()) {
      publishSensorData("clear");
    }
    
    // Exit calibration mode if still in it
    if (isCalibrationMode) {
      isCalibrationMode = false;
      
      // Special visual feedback for exiting calibration
      M5.Lcd.fillScreen(BLUE);
      M5.Lcd.setCursor(10, 40);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setTextSize(2);
      M5.Lcd.println("CALIBRATION");
      M5.Lcd.println("COMPLETE");
      delay(1000);
    }
    else {
      // Regular reset feedback
      M5.Lcd.fillScreen(GREEN);
      M5.Lcd.setCursor(20, 40);
      M5.Lcd.setTextColor(BLACK);
      M5.Lcd.setTextSize(2);
      M5.Lcd.println("RESET");
      M5.Lcd.println("COMPLETE");
      delay(500);
    }
    
    // Redraw entire screen
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextColor(COLOR_TEXT);
    M5.Lcd.println("Smoke Detector");
    M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
    updateDisplay();
  }
  
  // Button A long press: Toggle digital sensor
  if (M5.BtnA.pressedFor(2000)) {
    // Toggle digital sensor
    digitalEnabled = !digitalEnabled;
    
    // Visual feedback
    if (digitalEnabled) {
      M5.Lcd.fillScreen(BLUE);
      M5.Lcd.setCursor(20, 40);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.println("DIGITAL");
      M5.Lcd.println("ENABLED");
    } else {
      M5.Lcd.fillScreen(YELLOW);
      M5.Lcd.setCursor(20, 40);
      M5.Lcd.setTextColor(BLACK);
      M5.Lcd.println("DIGITAL");
      M5.Lcd.println("DISABLED");
    }
    delay(1000);
    
    // Redraw screen
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextColor(COLOR_TEXT);
    M5.Lcd.println("Smoke Detector");
    M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
    updateDisplay();
    
    // Publish updated state
    if (mqttClient.connected()) {
      publishSensorData();
    }
  }
  
  // Button B: Test mode (simulates smoke detection) or recalibrate
  if (M5.BtnB.wasPressed()) {
    Serial.println("Button B pressed");
    
    // Long press check (for recalibration)
    unsigned long pressStart = millis();
    while (M5.BtnB.read() && (millis() - pressStart < 2000)) {
      delay(10);
    }
    
    // If long press (>=2 seconds), recalibrate
    if (millis() - pressStart >= 2000) {
      Serial.println("Long press - Recalibrating");
      calibrateSensor();
      
      // Publish new threshold via MQTT
      if (mqttClient.connected()) {
        publishSensorData();
      }
    }
    // Otherwise do a test alarm
    else {
      Serial.println("Short press - Test Mode");
      
      // Exit calibration mode
      isCalibrationMode = false;
      
      // Trigger test alarm
      flashMessage("TEST", "ALARM", COLOR_WARNING, BLACK, 3);
      
      // Publish test alert via MQTT
      if (mqttClient.connected()) {
        publishSensorData("test");
      }
      
      // Visual feedback
      M5.Lcd.fillRect(M5.Lcd.width() - 40, 100, 40, 15, YELLOW);
      M5.Lcd.setCursor(M5.Lcd.width() - 35, 102);
      M5.Lcd.setTextColor(BLACK);
      M5.Lcd.setTextSize(1);
      M5.Lcd.print("TEST");
      M5.Lcd.setTextColor(COLOR_TEXT);
      M5.Lcd.setTextSize(2);
      delay(300);
      M5.Lcd.fillRect(M5.Lcd.width() - 40, 100, 40, 15, COLOR_BACKGROUND);
      
      // Reset screen
      M5.Lcd.fillScreen(COLOR_BACKGROUND);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.println("Smoke Detector");
      M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
    }
  }
  
  // Both buttons pressed together - Enter potentiometer adjustment mode
  if (M5.BtnA.read() && M5.BtnB.read()) {
    delay(100); // Debounce
    if (M5.BtnA.read() && M5.BtnB.read()) {
      potentiometerAdjustmentMode();
    }
  }
}

// Update the display with current sensor values and status
void updateDisplay() {
  // Clear value area
  M5.Lcd.fillRect(0, 25, M5.Lcd.width(), 70, COLOR_BACKGROUND);
  M5.Lcd.setCursor(0, 30);
  
  if (isCalibrationMode) {
    // Show calibration information
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.println("CALIBRATION MODE");
    M5.Lcd.setTextColor(COLOR_TEXT);
    M5.Lcd.print("Base: ");
    M5.Lcd.println(baselineValue);
    M5.Lcd.print("Curr: ");
    M5.Lcd.println(sensorValue);
    M5.Lcd.print("Thrs: ");
    M5.Lcd.println(analogThreshold);
    
    // Calibration instructions
    M5.Lcd.setCursor(0, 90);
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("A:Save B:Recalibrate");
  }
  else {
    // Regular monitoring display
    // Digital indicator - only show if enabled
    if (digitalEnabled) {
      if (digitalValue == LOW) { // Most MQ sensors trigger LOW when gas detected
        M5.Lcd.setTextColor(COLOR_WARNING);
        M5.Lcd.print("DIGITAL: ");
        M5.Lcd.println("ALERT!");
      } else {
        M5.Lcd.setTextColor(COLOR_SAFE);
        M5.Lcd.print("DIGITAL: ");
        M5.Lcd.println("OK");
      }
    } else {
      M5.Lcd.setTextColor(COLOR_TEXT);
      M5.Lcd.print("DIGITAL: ");
      M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.println("OFF");
    }
    
    M5.Lcd.setTextColor(COLOR_TEXT);
    
    // Analog value
    M5.Lcd.print("ANALOG: ");
    M5.Lcd.println(sensorValue);
    
    // Threshold value
    M5.Lcd.print("THRESH: ");
    M5.Lcd.println(analogThreshold);
    
    M5.Lcd.setTextColor(COLOR_TEXT);
    
    // Status indicator (only based on active sensors)
    M5.Lcd.setCursor(0, 75);
    bool analogAlert = (sensorValue > analogThreshold);
    bool digitalAlert = digitalEnabled && (digitalValue == LOW);
    
    if (analogAlert || digitalAlert) {
      M5.Lcd.setTextColor(COLOR_WARNING);
      M5.Lcd.println("SMOKE DETECTED!");
    } else {
      M5.Lcd.setTextColor(COLOR_SAFE);
      M5.Lcd.println("No Smoke");
    }
    
    M5.Lcd.setTextColor(COLOR_TEXT);
    
    // Help text
    M5.Lcd.setCursor(0, 95);
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("A:Reset B:Test A+B:Adjust");
    
    // WiFi/MQTT status indicator
    M5.Lcd.setCursor(M5.Lcd.width() - 20, 5);
    if (WiFi.status() == WL_CONNECTED) {
      if (mqttClient.connected()) {
        M5.Lcd.setTextColor(COLOR_SAFE);
        M5.Lcd.print("M");
      } else {
        M5.Lcd.setTextColor(COLOR_CAUTION);
        M5.Lcd.print("W");
      }
    } else {
      M5.Lcd.setTextColor(COLOR_WARNING);
      M5.Lcd.print("X");
    }
  }
  
  M5.Lcd.setTextSize(2);
}

// Check if smoke is detected and trigger alarm if necessary
void checkSmoke() {
  // Check only the analog sensor by default
  bool analogAlertCondition = (sensorValue > analogThreshold);
  
  // Only check digital sensor if enabled
  bool digitalAlertCondition = false;
  if (digitalEnabled) {
    digitalAlertCondition = (digitalValue == LOW); // Most MQ sensors trigger LOW when gas detected
  }
  
  // Trigger alarm if either sensor detects smoke (if enabled)
  if (analogAlertCondition || digitalAlertCondition) {
    // Smoke detected
    if (!alarmActive) {
      alarmActive = true;
      triggerAlarm();
      
      // Publish alert via MQTT
      if (mqttClient.connected()) {
        publishSensorData("alarm");
      }
    }
  } else {
    // No smoke
    if (alarmActive) {
      alarmActive = false;
      digitalWrite(buzzerPin, LOW);
      
      // Publish alert clear via MQTT
      if (mqttClient.connected()) {
        publishSensorData("clear");
      }
    }
  }
}

// Trigger alarm when smoke is detected
void triggerAlarm() {
  // Beep the buzzer
  for (int i = 0; i < 3; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(200);
    digitalWrite(buzzerPin, LOW);
    delay(100);
  }
  
  // Keep buzzer on
  digitalWrite(buzzerPin, HIGH);
  
  // Flash warning on screen
  flashMessage("SMOKE", "DETECTED!", COLOR_WARNING, BLACK, 5);
  
  // Return to normal display but with warning
  M5.Lcd.fillScreen(COLOR_BACKGROUND);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(COLOR_TEXT);
  M5.Lcd.println("Smoke Detector");
  M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
  updateDisplay();
}

// Calibrate smoke sensor
void calibrateSensor() {
  M5.Lcd.fillScreen(COLOR_BACKGROUND);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("CALIBRATION");
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("Keep in clean air for accurate calibration");
  M5.Lcd.setTextSize(2);
  
  // Disable digital readings by default
  digitalEnabled = true;
  
  // Display digital reading status during calibration
  M5.Lcd.setCursor(0, 80);
  M5.Lcd.setTextSize(1);
  M5.Lcd.print("Digital pin: ");
  bool digitalReading = digitalRead(mqDigitalPin);
  
  if (digitalReading == LOW) {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("ACTIVE");
    M5.Lcd.setTextColor(COLOR_TEXT);
    M5.Lcd.println("Digital sensor too sensitive!");
    M5.Lcd.println("Adjust potentiometer on sensor board");
  } else {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("INACTIVE");
    M5.Lcd.setTextColor(COLOR_TEXT);
  }
  
  M5.Lcd.setTextSize(2);
  
  // Take multiple readings for better accuracy
  long sum = 0;
  int samples = 15; // More samples for better accuracy
  
  for (int i = 0; i < samples; i++) {
    int reading = analogRead(mqAnalogPin);
    sum += reading;
    
    // Display current reading
    M5.Lcd.fillRect(0, 40, M5.Lcd.width(), 20, COLOR_BACKGROUND);
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.print("Reading: ");
    M5.Lcd.print(reading);
    
    // Progress indicator
    M5.Lcd.fillRect(0, 60, M5.Lcd.width(), 10, COLOR_BACKGROUND);
    M5.Lcd.fillRect(0, 60, (i+1) * (M5.Lcd.width()/samples), 10, BLUE);
    delay(300);
  }
  
  baselineValue = sum / samples;
  
  // Automatically set threshold above baseline with a reasonable margin
  // Higher multiplier = less sensitive
  float thresholdMultiplier = 1.5;
  analogThreshold = baselineValue * thresholdMultiplier;
  
  // Set a minimum threshold value to prevent false positives
  if (analogThreshold < baselineValue + 400) {
    analogThreshold = baselineValue + 400;
  }
  
  M5.Lcd.fillRect(0, 40, M5.Lcd.width(), 40, COLOR_BACKGROUND);
  M5.Lcd.setCursor(0, 40);
  M5.Lcd.print("Base: ");
  M5.Lcd.println(baselineValue);
  M5.Lcd.print("Thrs: ");
  M5.Lcd.println(analogThreshold);
  
  Serial.print("Calibration complete - Baseline: ");
  Serial.print(baselineValue);
  Serial.print(", Threshold: ");
  Serial.println(analogThreshold);

  M5.Lcd.fillRect(0, 80, M5.Lcd.width(), 40, COLOR_BACKGROUND);
  M5.Lcd.setCursor(0, 80);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("Digital sensor enabled");
  M5.Lcd.println("Using both analog and digital");
  M5.Lcd.println("");
  M5.Lcd.println("Press A to start monitoring");
  M5.Lcd.println("Hold A to disable digital sensor if needed");
  M5.Lcd.setTextSize(2);
  
  // Reset history array
  for (int i = 0; i < historySize; i++) {
    valueHistory[i] = baselineValue;
  }
  
  // Keep in calibration mode until user confirms
  isCalibrationMode = true;
}

// Get average reading from history
int getAverageReading() {
  long sum = 0;
  for (int i = 0; i < historySize; i++) {
    sum += valueHistory[i];
  }
  return sum / historySize;
}

// Flash a message on the screen
void flashMessage(const char* line1, const char* line2, uint16_t bgColor, uint16_t textColor, int flashes) {
  for (int i = 0; i < flashes; i++) {
    M5.Lcd.fillScreen(bgColor);
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.setTextColor(textColor);
    M5.Lcd.println(line1);
    M5.Lcd.println(line2);
    delay(200);
    
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.setTextColor(bgColor);
    M5.Lcd.println(line1);
    M5.Lcd.println(line2);
    delay(200);
  }
}

// Adjust potentiometer on sensor
void potentiometerAdjustmentMode() {
  Serial.println("Entering potentiometer adjustment mode");
  
  // Initial setup
  M5.Lcd.fillScreen(COLOR_BACKGROUND);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(COLOR_TEXT);
  M5.Lcd.println("Adjust Sensor");
  M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
  
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 25);
  M5.Lcd.println("Turn potentiometer to adjust");
  M5.Lcd.println("the digital threshold");
  M5.Lcd.println("Goal: OFF in clean air,");
  M5.Lcd.println("ON when smoke detected");
  M5.Lcd.setTextSize(2);
  
  bool lastDigitalState = digitalRead(mqDigitalPin);
  int stateChangeCount = 0;
  unsigned long startTime = millis();
  

  bool previousDigitalState = digitalEnabled;
  digitalEnabled = true;
  

  while (true) {
 
    M5.update();
    
    // Read digital value
    bool currentDigitalState = digitalRead(mqDigitalPin);

    if (currentDigitalState != lastDigitalState) {
      stateChangeCount++;
      lastDigitalState = currentDigitalState;
      
      if (currentDigitalState == LOW) { 
        M5.Lcd.fillRect(0, 80, M5.Lcd.width(), 40, COLOR_WARNING);
      } else {
        M5.Lcd.fillRect(0, 80, M5.Lcd.width(), 40, COLOR_SAFE);
      }
      delay(100);
    }
    
    // Display current status
    M5.Lcd.fillRect(0, 80, M5.Lcd.width(), 40, COLOR_BACKGROUND);
    M5.Lcd.setCursor(0, 80);
    M5.Lcd.setTextSize(3); 
    
    if (currentDigitalState == LOW) { 
      M5.Lcd.setTextColor(COLOR_WARNING);
      M5.Lcd.println("ALERT!");
    } else {
      M5.Lcd.setTextColor(COLOR_SAFE);
      M5.Lcd.println("OK");
    }
    
    M5.Lcd.setTextColor(COLOR_TEXT);
    
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(0, 120);
    M5.Lcd.print("Changes: ");
    M5.Lcd.println(stateChangeCount);
    M5.Lcd.print("Time: ");
    M5.Lcd.print((millis() - startTime) / 1000);
    M5.Lcd.println("s");
    M5.Lcd.println("Press any button to exit");
    
    // Exit if any button is pressed
    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) {
      break;
    }
    
    delay(50); 
  }
  
  // Restore previous digital state
  digitalEnabled = previousDigitalState;
  
  // Exit message
  M5.Lcd.fillScreen(BLUE);
  M5.Lcd.setCursor(20, 40);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("ADJUSTMENT");
  M5.Lcd.println("COMPLETE");
  delay(1000);
  
  // Return to main screen
  M5.Lcd.fillScreen(COLOR_BACKGROUND);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(COLOR_TEXT);
  M5.Lcd.println("Smoke Detector");
  M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
  updateDisplay();
}

// Setup WiFi connection
void setupWifi() {
  M5.Lcd.fillScreen(COLOR_BACKGROUND);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(COLOR_TEXT);
  M5.Lcd.println("Connecting to WiFi...");
  M5.Lcd.println(ssid);
  

  WiFi.begin(ssid, password);
  
  // Wait for connection (with timeout)
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
    M5.Lcd.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to WiFi. IP: ");
    Serial.println(WiFi.localIP());
    
    M5.Lcd.println("");
    M5.Lcd.println("Connected!");
    M5.Lcd.print("IP: ");
    M5.Lcd.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed!");
    
    M5.Lcd.println("");
    M5.Lcd.setTextColor(COLOR_WARNING);
    M5.Lcd.println("WiFi connection failed!");
    M5.Lcd.setTextColor(COLOR_TEXT);
  }
  
  delay(1000);
}

// Connect/reconnect to MQTT broker
void reconnectMqtt() {
  // Try to connect to MQTT broker
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 3) {
    Serial.print("Attempting MQTT connection...");
    
    // Create a last will message as JSON
    StaticJsonDocument<200> willDoc;
    willDoc["state"] = "offline";
    
    char willPayload[200];
    serializeJson(willDoc, willPayload);
    
    // Attempt to connect with last will message
    bool success = false;
    if (strlen(mqtt_user) > 0) {
      success = mqttClient.connect(client_id, mqtt_user, mqtt_password, 
                                  mqtt_topic, 1, true, willPayload);
    } else {
      success = mqttClient.connect(client_id, mqtt_topic, 1, true, willPayload);
    }
    
    if (success) {
      Serial.println("connected");
      
      // Once connected, publish an announcement
      publishSensorData("online");
      
      // Subscribe to command topic
      mqttClient.subscribe("zigbee2mqtt/gas_sensor/set");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 1 second");
      attempts++;
      delay(1000);
    }
  }
}

// Publish sensor data via MQTT
void publishSensorData(const char* state) {
  // Create JSON document
  StaticJsonDocument<200> doc;
  
  // Determine state to publish
  if (state != NULL) {
    doc["state"] = state;
  } else {
    // Check if any sensors are triggered
    bool analogAlert = (sensorValue > analogThreshold);
    bool digitalAlert = digitalEnabled && (digitalValue == LOW);
    
    if (analogAlert || digitalAlert) {
      doc["state"] = "alarm";
    } else {
      doc["state"] = "clear";
    }
  }
  
  // Add analog value
  doc["analog_value"] = sensorValue;
  
  // Add threshold value
  doc["threshold"] = analogThreshold;
  
  // Convert to JSON string and publish
  char buffer[200];
  serializeJson(doc, buffer);
  
  // Log the JSON payload for debugging
  Serial.print("Publishing to ");
  Serial.print(mqtt_topic);
  Serial.print(": ");
  Serial.println(buffer);
  
  mqttClient.publish(mqtt_topic, buffer, true);
}

// MQTT message callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  // Convert payload to string
  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  
  Serial.println(message);
  
  // Parse JSON message
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  // Handle errors in JSON parsing
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Handle commands
  if (String(topic) == "zigbee2mqtt/gas_sensor/set") {
    if (doc.containsKey("reset") && doc["reset"].as<bool>()) {
      digitalWrite(buzzerPin, LOW);
      alarmActive = false;
      
      // Visual feedback
      M5.Lcd.fillScreen(GREEN);
      M5.Lcd.setCursor(20, 40);
      M5.Lcd.setTextColor(BLACK);
      M5.Lcd.println("REMOTE");
      M5.Lcd.println("RESET");
      delay(500);
      
      // Redraw screen
      M5.Lcd.fillScreen(COLOR_BACKGROUND);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.setTextColor(COLOR_TEXT);
      M5.Lcd.println("Smoke Detector");
      M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
      updateDisplay();
      
      // Publish alert clear
      publishSensorData("clear");
    }
    
    // Calibrate command
    else if (doc.containsKey("calibrate") && doc["calibrate"].as<bool>()) {
      // Recalibrate sensor
      calibrateSensor();
      
      // Exit calibration mode
      isCalibrationMode = false;
      
      // Redraw screen
      M5.Lcd.fillScreen(COLOR_BACKGROUND);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.setTextColor(COLOR_TEXT);
      M5.Lcd.println("Smoke Detector");
      M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
      updateDisplay();
      
      // Publish updated settings
      publishSensorData("clear");
    }
    
    // Test command
    else if (doc.containsKey("test") && doc["test"].as<bool>()) {
      // Trigger test alarm
      flashMessage("REMOTE", "TEST", COLOR_WARNING, BLACK, 3);
      
      // Publish test alert
      publishSensorData("test_alarm");
      
      // Redraw screen
      M5.Lcd.fillScreen(COLOR_BACKGROUND);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.setTextColor(COLOR_TEXT);
      M5.Lcd.println("Smoke Detector");
      M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
      updateDisplay();
    }
    
    // Set threshold
    else if (doc.containsKey("threshold")) {
      // Update threshold
      int newThreshold = doc["threshold"].as<int>();
      
      if (newThreshold > 0) {
        analogThreshold = newThreshold;
        
        // Visual feedback
        M5.Lcd.fillScreen(BLUE);
        M5.Lcd.setCursor(10, 40);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.println("NEW THRESHOLD");
        M5.Lcd.println(analogThreshold);
        delay(1000);
        
        // Redraw screen
        M5.Lcd.fillScreen(COLOR_BACKGROUND);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextColor(COLOR_TEXT);
        M5.Lcd.println("Smoke Detector");
        M5.Lcd.drawLine(0, 20, M5.Lcd.width(), 20, COLOR_TEXT);
        updateDisplay();
        
        // Publish updated settings
        publishSensorData("clear");
      }
    }
    
    // Digital sensor commands
    else if (doc.containsKey("digital_enabled")) {
      digitalEnabled = doc["digital_enabled"].as<bool>();
      updateDisplay();
      publishSensorData("clear");
    }
  }
}