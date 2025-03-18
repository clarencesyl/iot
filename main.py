from tele_bot import routine_check
from tele_bot import start_bot, send_telegram_message
import asyncio
import threading
import time

async def simple_async_task():
    print("Simple sync task has started")
    while True:
        print("Simple sync task is running...")
        await asyncio.sleep(1)

async def main():
    # Start bot in the background
    bot_task = asyncio.create_task(start_bot())

    simple_task = asyncio.create_task(simple_async_task())

    # Run both at once
    await asyncio.gather(bot_task, simple_task)

if __name__ == "__main__":
    asyncio.run(main())


