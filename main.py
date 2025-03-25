from tele_bot import routine_check
from tele_bot import start_bot, send_telegram_message
from smart_air_box_and_plug import data_queue, run_mqtt
import asyncio
import time

async def retrieve_data():
    """ Process data from the queue asynchronously """
    while True:
        print("test")
        data = await data_queue.get()  # Retrieve latest data from the queue
        print("Received Data:", data)
        
        # Add any additional processing logic here
        # {"SMART_AIR_BOX_DATA_HUMIDITY": "100", "MOTION_SENSOR_DATA": "Motion Detected", "MOISTURE_SENSOR_DATA": "100", "GAS_SENSOR_DATA": ""}

        await asyncio.sleep(1)

async def main():
    """ Main function to run the MQTT client and process the queue and telegram bot """

    # Start MQTT client
    mqtt_task = asyncio.create_task(run_mqtt())

    # Start bot in the background
    bot_task = asyncio.create_task(start_bot())

    simple_task = asyncio.create_task(retrieve_data())

    await asyncio.gather(mqtt_task, bot_task, simple_task)

if __name__ == "__main__":
    asyncio.run(main())