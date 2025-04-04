import aiohttp
import asyncio
import json
import paho.mqtt.client as mqtt
import ast

# Global asyncio queue to store data to pass to main.py
data_queue = asyncio.Queue()
loop = asyncio.new_event_loop()
asyncio.set_event_loop(loop)

""" Athom PG04V2-Uk16A Tasmota Smart Plug """
TASMOTA_IP = "http://192.168.218.167" # connect to same hotspot as raspberry pi, when connect will show ip of tasmota so no need to find it

# Tasmota API endpoints
POWER_ON_URL = f"{TASMOTA_IP}/cm?cmnd=Power%20On"
POWER_OFF_URL = f"{TASMOTA_IP}/cm?cmnd=Power%20Off"
POWER_TOGGLE_URL = f"{TASMOTA_IP}/cm?cmnd=Power%20Toggle"

async def send_request(url):
    """ Send an asynchronous HTTP GET request """
    async with aiohttp.ClientSession() as session:
        try:
            async with session.get(url) as response:
                if response.status == 200:
                    print(f"SUCCESS: Request to {url} successful.")
                else:
                    print(f"FAILED: Request to {url} failed: {await response.text()}")
        except Exception as e:
            print("Error:", e)

async def toggle_power_fan():
    """ Toggle the ON/OFF the smart plug """
    await send_request(POWER_TOGGLE_URL)

async def turn_on_fan():
    """ Turn the smart plug ON """
    await send_request(POWER_ON_URL)

async def turn_off_fan():
    """ Turn the smart plug OFF """
    await send_request(POWER_OFF_URL)

""" Smart Air Box """
# Define the MQTT broker and topic
MQTT_BROKER = "192.168.218.73"  # Raspberry pi ip address REMEMBER TO CONNECT LAPTOP TO HOTSPOT
MQTT_PORT = 1883
# Remember to change accordingly for the respective sensor code file also
MQTT_TOPIC_SMART_AIR_BOX = "zigbee2mqtt/SmartAirBox"
MQTT_TOPIC_MOTION_SENSOR = "zigbee2mqtt/motion_sensor"
MQTT_TOPIC_MOISTURE_SENSOR = "zigbee2mqtt/moisture_sensor"
MQTT_TOPIC_GAS_SENSOR = "zigbee2mqtt/gas_sensor"

# Global variable to store received data from respective sensors via MQTT topic
SMART_AIR_BOX_DATA = {}
MOTION_SENSOR_DATA = ""
MOISTURE_SENSOR_DATA = ""
GAS_SENSOR_DATA = ""
FINAL_DATA = {"SMART_AIR_BOX_DATA_HUMIDITY": "", "SMART_AIR_BOX_DATA_TEMPERATURE": "", "MOTION_SENSOR_DATA": "", "MOISTURE_SENSOR_DATA": "", "GAS_SENSOR_DATA": ""}  # This variable is to store all the other data combined

def on_message(client, userdata, msg):  # CANNOT HAVE ASYNC IN THIS FUNC
    """ MQTT callback function for received messages """
    global SMART_AIR_BOX_DATA, MOTION_SENSOR_DATA, MOISTURE_SENSOR_DATA, GAS_SENSOR_DATA, FINAL_DATA, loop

    payload = msg.payload.decode("utf-8")
    # if msg.topic == "zigbee2mqtt/gas_sensor":
        # print(f"Message received on topic {msg.topic}: {payload}")  # print the received messages on the respective topics

    if msg.topic == MQTT_TOPIC_SMART_AIR_BOX:
        SMART_AIR_BOX_DATA = json.loads(payload)
        # print("SMART AIR BOX DATA:", SMART_AIR_BOX_DATA)

        FINAL_DATA["SMART_AIR_BOX_DATA_HUMIDITY"] = str(SMART_AIR_BOX_DATA.get("humidity", 0))
        FINAL_DATA["SMART_AIR_BOX_DATA_TEMPERATURE"] = str(SMART_AIR_BOX_DATA.get("temperature", 0))

    if msg.topic == MQTT_TOPIC_MOTION_SENSOR:
        # String data (refer to sensor code file client.publish)
        MOTION_SENSOR_DATA = payload.strip()
        # print("MOTION SENSOR DATA:", MOTION_SENSOR_DATA)

        FINAL_DATA["MOTION_SENSOR_DATA"] = str(MOTION_SENSOR_DATA)

    if msg.topic == MQTT_TOPIC_MOISTURE_SENSOR:
        # String data (refer to sensor code file client.publish)
        MOISTURE_SENSOR_DATA = payload.strip()
        # print("MOISTURE SENSOR DATA:", MOISTURE_SENSOR_DATA)

        FINAL_DATA["MOISTURE_SENSOR_DATA"] = str(ast.literal_eval(MOISTURE_SENSOR_DATA).get("moisture", 0))

    if msg.topic == MQTT_TOPIC_GAS_SENSOR:
        # String data OR JSON/dictionary
        GAS_SENSOR_DATA = payload.strip()
        # print("GAS SENSOR DATA:", GAS_SENSOR_DATA)

        FINAL_DATA["GAS_SENSOR_DATA"] = str(ast.literal_eval(GAS_SENSOR_DATA).get("analog_value", 0))

    # print("FINAL DATA:", FINAL_DATA)

    # Add data to queue to be sent to main.py
    # Add data to queue safely
    if loop.is_running():
        asyncio.run_coroutine_threadsafe(data_queue.put(FINAL_DATA.copy()), loop)
    else:
        print("Event loop is not running yet!")

async def start_mqtt_client():
    """ Start the MQTT client asynchronously """
    client = mqtt.Client()
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    # Subscribe to the various topics accordingly
    client.subscribe(MQTT_TOPIC_SMART_AIR_BOX)
    client.subscribe(MQTT_TOPIC_MOTION_SENSOR)
    client.subscribe(MQTT_TOPIC_MOISTURE_SENSOR)
    client.subscribe(MQTT_TOPIC_GAS_SENSOR)
    client.loop_start()
    while True:
        await asyncio.sleep(1)

async def run_mqtt():
    """ Runs the MQTT client in an asyncio-compatible way. """
    await start_mqtt_client()

async def main():
    """ Main function to run the MQTT client and process the queue and telegram bot """
    # Start MQTT client
    mqtt_task = asyncio.create_task(run_mqtt())

    await asyncio.gather(mqtt_task)

if __name__ == "__main__":
    asyncio.run(main())
