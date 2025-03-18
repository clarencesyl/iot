"""
This file contains the logic to handle the smart air box and smart plug such
as the logic for the if else statement to turn ON/OFF the smart plug based
on the parameters accordingly.

This file also contains the code to collect the data from the various sensors 
via the MQTT topic and passes it to the main.py file.
"""

import requests
import threading
import queue
import time
import paho.mqtt.client as mqtt
import json

# Global queue to store data
data_queue = queue.Queue()

""" Athom PG04V2-Uk16A Tasmota Smart Plug """
TASMOTA_IP = "http://192.168.202.167"  # Smart plug ip address (must be connected to the same network as raspberry pi)

# Tasmota API endpoints
POWER_ON_URL = f"{TASMOTA_IP}/cm?cmnd=Power%20On"
POWER_OFF_URL = f"{TASMOTA_IP}/cm?cmnd=Power%20Off"
POWER_TOGGLE_URL = f"{TASMOTA_IP}/cm?cmnd=Power%20Toggle"

def toggle_power():
    """ Toggle the ON/OFF the smart plug """
    try:
        response = requests.get(POWER_TOGGLE_URL)
        if response.status_code == 200:
            print("SUCCESS: Successfully toggled smart plug.")
        else:
            print("FAILED: Failed to toggle smart plug:", response.text)
    except Exception as e:
        print("Error:", e)

def turn_on():
    """ Turn the smart plug ON """
    try:
        response = requests.get(POWER_ON_URL)
        if response.status_code == 200:
            print("SUCCESS: Smart plug successfully turned ON.")
        else:
            print("FAILED: Smart plug failed to turn ON:", response.text)
    except Exception as e:
        print("Error:", e)

def turn_off():
    """ Turn the smart plug OFF """
    try:
        response = requests.get(POWER_OFF_URL)
        if response.status_code == 200:
            print("SUCCESS: Smart plug successfully turned OFF.")
        else:
            print("FAILED: Smart plug failed to turn OFF:", response.text)
    except Exception as e:
        print("Error:", e)

""" Smart Air Box """
# Define the MQTT broker and topic
MQTT_BROKER = "192.168.202.73"  # Raspberry pi's ip address
MQTT_PORT = 1883
MQTT_TOPIC = "zigbee2mqtt/0xa4c138a5d0b49a7c"  # Topic to subscribe to

SMART_AIR_BOX_DATA = {}  # Global variable that stores json data from on_message() function so that it can be used in telegram function
MOTION_SENSOR_DATA = ""  # # Global variable that stores string data from motion sensor

# This function will be called when a message is received
def on_message(client, userdata, msg):
    global SMART_AIR_BOX_DATA
    global MOTION_SENSOR_DATA

    # Print the topic and the payload (JSON data)
    print(f"Message received on topic {msg.topic}: {msg.payload}")
    
    payload  = msg.payload.decode("utf-8")

    # JSON data (smart airbox and plug stuff)
    try:
        # Parse the JSON data from the message payload
            SMART_AIR_BOX_DATA = json.loads(msg.payload)  # Update global variable
            print("Parsed JSON Data:", SMART_AIR_BOX_DATA)  # {"co2":368,"formaldehyd":2,"humidity":62,"linkquality":136,"temperature":29.7,"voc":1}

            # TODO: Move this if else logic to below as it needs to include the other sensor data as well not only humidity

            # Logic to determine when to toggle Smart Plug ON/OFF
            humidity = SMART_AIR_BOX_DATA["humidity"]

            # Eventually need to take data from smoke sensor as well and implement inside this if else logic not only use humidity

            # If humidity > 55, turn ON Smart Plug
            if humidity > 55:
                turn_on()
            
            # If humidity <= 55, turn OFF Smart Plug
            else:
                turn_off()

            # Send JSON data (return SMART_AIR_BOX_DATA global variable) to main.py file
            # Put the data into the queue to make it accessible by the main.py file
            data_queue.put(SMART_AIR_BOX_DATA)

    # If it is not JSON data (other sensor data)
    except:
        # Motion sensor (string data)
        MOTION_SENSOR_DATA = payload.strip()  # Store motion sensor string data to global variable
        print("Received Motion Sensor Data:", MOTION_SENSOR_DATA)

        # Send string data (return MOTION_SENSOR_DATA global variable) to main.py file
        # Put the data into the queue to make it accessible by the main.py file
        data_queue.put(MOTION_SENSOR_DATA)

        # TODO: Water sensor, gas sensor. Do the same thing as motion sensor as they will all be string data then place it in the data_queue to be sent to the main.py file.
        
def start_mqtt_client():
    """ Starts the MQTT client in a separate thread """
    # Create an MQTT client instance
    client = mqtt.Client()
    # Set the callback function for message reception
    client.on_message = on_message
    # Connect to the MQTT broker
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    # Subscribe to the topic
    client.subscribe(MQTT_TOPIC)
    # Loop forever to keep receiving messages
    client.loop_forever()

# Threading to handle concurrency for Smart Air Box and Smart Plug 
# to allow the code to know when to call the turn ON/OFF Smart Plug code 
# accordingly when the criteria match within the Smart Air Box code

# The following code below is used to run the smart_air_box_and_plug.py threading code
# however we no longer need it as we are running it in main.py which is why we make the
# if __name__ == "__main__" so that it will only be ran if this file is executed directly

if __name__ == "__main__":
    # Start MQTT in a new thread
    mqtt_thread = threading.Thread(target=start_mqtt_client, daemon=True)
    mqtt_thread.start()

    # Keep the script running only when executed directly
    # Need this for the threading to work
    while True:
        time.sleep(1)  # Add a delay to prevent high CPU usage