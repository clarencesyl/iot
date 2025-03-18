#include <M5StickCPlus.h>

#define MOISTURE_SENSOR_PIN 26  // Analog pin where the sensor is connected

void setup() {
    M5.begin(); // Initialize M5StickCPlus
    M5.Lcd.setRotation(3); // Set screen orientation
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);

    Serial.begin(115200);
}

void loop() {
    int sensorValue = analogRead(MOISTURE_SENSOR_PIN);  // Read analog value from sensor
    float voltage = sensorValue * (3.3 / 4095.0); // Convert ADC value to voltage (ESP32 ADC range: 0-4095)
    
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

    Serial.print("Sensor Value: ");
    Serial.print(sensorValue);
    Serial.print(" | Voltage: ");
    Serial.println(voltage);

    delay(1000);  // Wait for 1 second before next reading
}
