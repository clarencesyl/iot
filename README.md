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
