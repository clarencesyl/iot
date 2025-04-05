import telegram
from telegram import Update
from telegram.ext import Application, CommandHandler, MessageHandler, filters, ContextTypes
import asyncio
from data_manager import DataManager

# Replace with your bot's API token and user ID
BOT_TOKEN = '8021420031:AAHJYXHQ40stDTJxiyG1pcArayDDp4OZQEI'
BOT_USERNAME = 'kitchen_safety_bot'
USER_ID = 531028339


async def send_telegram_message(message, user_id):
    try:
        bot = telegram.Bot(token=BOT_TOKEN)
        await bot.send_message(chat_id=user_id, text=message)
        print("Message sent successfully!")
    except Exception as e:
        print(f"Error sending message: {e}")


async def status_update(data):
    message = f"Status Update: {data}"
    await send_telegram_message(message, USER_ID)


async def start_command(update: Update, context: ContextTypes.DEFAULT_TYPE):
    await update.message.reply_text("Hello! I am your smart kitchen assistant. I will send you updates on your kitchen environment.")

async def help_command(update: Update, context: ContextTypes.DEFAULT_TYPE):
    await update.message.reply_text("Use /status to find out about your kitchen!")

async def status_command(update: Update, context: ContextTypes.DEFAULT_TYPE):
    from data_processing import KitchenSafetyMonitor
    data = DataManager.get_data()

    message = KitchenSafetyMonitor.process_status_update(data)

    await update.message.reply_text(message)


def handle_response(text:str)->str:
    processed:str = text.lower()

    if 'status' in processed:
        return 'status received'

async def handle_message(update: Update, context: ContextTypes.DEFAULT_TYPE):
    message_type: str = update.message.chat.type # Identifies private or group chat
    text: str = update.message.text # The incoming message

    response = handle_response(text)
    print(f'User({update.message.chat_id}) sent: {message_type}: "{text}"')

    await send_telegram_message(response, USER_ID)

async def error(update: Update, context: ContextTypes.DEFAULT_TYPE):
    print(f"Update {update} caused error {context.error}")

async def start_bot():
    print('Starting bot...')
    app = Application.builder().token(BOT_TOKEN).build()

    await app.initialize()
    await app.start()

    # Start other asyncio frameworks here
    # Add some logic that keeps the event loop running until you want to shutdown
    # Stop the other asyncio frameworks here
    # await app.updater.stop()
    # await app.stop()
    # await app.shutdown()
    # Commands
    app.add_handler(CommandHandler('start', start_command))
    app.add_handler(CommandHandler('help', help_command))
    app.add_handler(CommandHandler('status', status_command))

    # Messages
    app.add_handler(MessageHandler(filters.Text, handle_message))

    # Error handling
    app.add_error_handler(error)

    await app.updater.start_polling()
    # Polls the bot
    print('Polling...')
    # Keep the bot running using asyncio.Event()
    stop_event = asyncio.Event()
    try:
        await stop_event.wait()  # Keeps the event loop alive
    except KeyboardInterrupt:
        print("Shutting down bot...")

    # Cleanup when exiting
    await app.updater.stop()
    await app.stop()
    await app.shutdown()

