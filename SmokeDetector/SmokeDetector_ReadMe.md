1. **Power on the Device**  

2. **Connect to the Device Hotspot**  

3. **Wait for Calibration (5 Seconds)**  
   Upon startup, the device enters *calibration mode* for 5 seconds. Allow it to complete this step without interruption.

4. **Press the A Button (Front Button)**  
   Once calibration is complete, press the **A button** located below the screen.  
   → The screen will display the current **threshold** and **analog sensor value**.

5. **Trigger Test Alarm with B Button (Side Button)**  
   Press the **B button** on the side of the device.  
   → This will simulate an alarm test and send `status: test` to the MQTT broker.

6. **Simulate a Fire Event**  
   Blow hard into the smoke sensor (iykyk).  
   → If the sensor detects it, the device will send `status: alarm` to the broker.

