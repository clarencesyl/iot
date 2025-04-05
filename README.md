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

     - Set `mqtt.server` to `mqtt://localhost`
     - Use the correct `serial.port` path (e.g., `/dev/ttyACM0`)
     - Enable the frontend for web access
     - Change adapter to `ember` if required'
   
   - Here is an example of this project's configuration file:
   
```
homeassistant:
  enabled: true
mqtt:
  base_topic: zigbee2mqtt
  server: mqtt://localhost
serial:
  port: /dev/ttyACM0
  adapter: ember
frontend:
  enabled: true
  port: 8080
version: 4
devices:
  '0xa4c138a5d0b49a7c':
    friendly_name: 'SmartAirBox'
```


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

## New Telegram bot
In the even that you want to create a new telegram bot instead of the existing kitchen_safety_bot, follow the below steps:
- Search for @BotFather in telegram. /start to start the bot.
- /createbot to create a new bot.
- Follow the instructions to create a new bot.
- Copy the token and paste it in the code: tele_bot.py > BOT_TOKEN

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

- Telegram: Search for @kitchen_safety_bot in telegram. /start to start the bot after this codes/server is running.
