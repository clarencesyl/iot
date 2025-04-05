# Official readme for submission, remove this line once done

## Raspberry Pi and Smart Air Box Setup

1. **Install Dependencies**

   **Node.js & npm**  
   - Zigbee2MQTT requires Node.js (v18 LTS recommended). Install or update Node.js and npm as needed.

   **Mosquitto MQTT Broker**  
   - Install Mosquitto and its clients. Enable and start the Mosquitto service to handle MQTT communication.

2. **Install Zigbee2MQTT**  
   - Clone the Zigbee2MQTT repository.  
   - Navigate to the directory and install dependencies using `npm ci`. If issues occur, fallback to `npm install`.

3. **Connect Zigbee USB Dongle**  
   - Plug in your Zigbee USB dongle and identify the serial port via `/dev/serial/by-id/`.

4. **Configure Zigbee2MQTT**  
   - Edit the `configuration.yaml`:

     -  Set `mqtt.server` to `mqtt://localhost`
     - Use the correct `serial.port` path (e.g., `/dev/ttyACM0`)
     - Enable the frontend for web access
     - Change adapter to `ember` if required

   - A copy of the config file can be found in the `config` folder.

5. **Start Zigbee2MQTT**  
   - Run the service using `npm start` from the Zigbee2MQTT directory.

6. **Optional: Auto-Start on Boot**  
   - Create a systemd service to run Zigbee2MQTT on boot.  
   - Reload systemd and enable the service.

7. **Access Web Interface**  
   - Access the Zigbee2MQTT dashboard at: `http://<your-raspberry-pi-ip>:8080`  
   - Use `hostname -I` to find your Pi’s IP.

8. **Pair Smart Air Box**  
   - Enable pairing from the frontend or via MQTT.  
   - Put your Zigbee device into pairing mode to connect.

9. **Monitor MQTT Messages (Optional)**  
   - Use `mosquitto_sub` to view live messages from your Zigbee devices.



# HOW TO RUN

Navigate to a directory of your choice.

Open cmd in that directory.

Type the command "py -m venv venv" this will create a venv folder (might take awhile).

cd into the folder ../venv/Scripts then type "activate"

After that cd into the directory containing "requirements.txt" and type this command "pip install -r requirements.txt"

After than run "py main.py" to start the program

## Setup

- smart_air_box_and_plug.py change MQTT_BROKER ip address to be that of the raspberry pi ip address. Also Connect laptop to the hotspot that the raspberry pi is using.

- TASMOTA_IP perform the below step then use that ip address in this variable.

- tasmota press and hold the power button for about 15 seconds to reset then connect it to the same hotspot as raspberry pi. Use phone go to wifi while press and hold tasmota, then connect to it, a popup will appear then connect to the raspberry pi hotspot. The ip address of the tasmota will be shown, note it down.

- tasmota if cannot find ip address, use ip scanner and scan the hotspot then match the mac address, mac address of tasmota is 34:AB:95:A7:93:F9
