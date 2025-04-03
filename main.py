from tele_bot import routine_check
from tele_bot import start_bot, send_telegram_message
from smart_air_box_and_plug import data_queue, run_mqtt, loop
import asyncio
import time

async def retrieve_data():
    """ Process data from the queue asynchronously """
    while True:
        data = await data_queue.get()  # Retrieve latest data from the queue
        print("MAIN.PY FINAL DATA:", data)
        await asyncio.sleep(1)

async def main():
    """ Main function to run the MQTT client and process the queue and telegram bot """
    global loop
    asyncio.set_event_loop(loop)

    # Start MQTT client
    mqtt_task = asyncio.create_task(run_mqtt())

    # Start bot in the background
    bot_task = asyncio.create_task(start_bot())

    simple_task = asyncio.create_task(retrieve_data())

    await asyncio.gather(mqtt_task, bot_task, simple_task)

if __name__ == "__main__":
    # asyncio.run(main())
    loop.run_until_complete(main())
