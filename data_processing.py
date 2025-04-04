import time
from tele_bot import status_update
from smart_air_box_and_plug import turn_on_fan, turn_off_fan, toggle_power_fan

class KitchenSafetyMonitor:
    def __init__(self):
        # Thresholds for scenarios
        self.GAS_LEAK_THRESHOLD = 500 # If gas > this, gas detected
        self.STOVE_OFF_TEMP = 30.7  # If temp < this, stove is considered "off"
        self.HIGH_TEMP_THRESHOLD = 30.7  # Temp considered "elevated" for stove
        self.HIGH_HUMIDITY_THRESHOLD = 85  # Humidity considered "elevated"
        self.LOW_TEMP_THRESHOLD = 50  # Temp considered "low" for humidity check
        self.SMOKE_FIRE_THRESHOLD = 1000  # Smoke level indicating fire
        self.MOISTURE_THRESHOLD = 1000  # Moisture level indicating leak

        self.gas_leak_start = None
        self.unattended_stove_start = None
        self.humidity_start = None
        self.fire_detected = False
        self.water_leak_detected = False

        self.is_fan_on = False

        # self.send_status = False
        self.humid_check = False
        self.water_leak_check = False
        self.gas_leak_check = False
        self.unattended_check = False

    async def process_data(self, data):
        await self.check_gas_leak(data)
        await self.check_unattended_stove(data)
        await self.check_humidity(data)
        await self.check_fire(data)
        await self.check_water_leak(data)

    async def check_gas_leak(self, data):
        print(data)
        gas_level = int(data.get('GAS_SENSOR_DATA', 0) or 0)
        current_temp = float(data.get('SMART_AIR_BOX_DATA_TEMPERATURE', 0) or 0)
        print(f"Gas Level: {gas_level}")

        gas_detected = gas_level > self.GAS_LEAK_THRESHOLD
        stove_is_off = current_temp < self.STOVE_OFF_TEMP

        if gas_detected and stove_is_off:
            # current_time = time.time()
            # if self.gas_leak_start is None:
            #     self.gas_leak_start = current_time
            # elif current_time - self.gas_leak_start >= 30:
            #     gas_leak_time = current_time - self.gas_leak_start
            print("Gas leak detected!")
            if self.gas_leak_check == False:
                await status_update(f"⚠️ GAS LEAK DETECTED! High gas levels with stove off for 30 minutes")
                self.gas_leak_check = True 
            # print("Gas leak detected!")
            self.gas_leak_start = None
        else:
            self.gas_leak_start = None

    async def check_unattended_stove(self, data):
        current_temp = float(data.get('SMART_AIR_BOX_DATA_TEMPERATURE', 0) or 0)
        motion_status = data.get('MOTION_SENSOR_DATA', '')
        print(f"Current Temperature: {current_temp}")
        print(f"Motion Status: {motion_status}")

        temp_is_high = current_temp > self.HIGH_TEMP_THRESHOLD
        no_motion = motion_status != "Motion Detected!"

        if temp_is_high and no_motion:
            # current_time = time.time()
            # if self.unattended_stove_start is None:
            #     self.unattended_stove_start = current_time
            # elif current_time - self.unattended_stove_start >= 3600:
            #     unattended_stove_time = current_time - self.unattended_stove_start
            #     print("Unattended stove detected!")
            #     await status_update(f"⚠️ UNATTENDED STOVE DETECTED! High temperature with no motion for {unattended_stove_time} seconds")
            #     self.unattended_stove_start = None
            if self.unattended_check == False:
                print("Stove is on but unattended!")
                await status_update(f"⚠️ UNATTENDED STOVE DETECTED! High temperature with no motion for 30 minutes")
                self.unattended_check = True
        else:
            self.unattended_stove_start = None

    async def check_humidity(self, data):
        current_humidity = int(data.get('SMART_AIR_BOX_DATA_HUMIDITY', 0) or 0)
        current_temp = float(data.get('SMART_AIR_BOX_DATA_TEMPERATURE', 0) or 0)
        current_co2 = int(data.get('SMART_AIR_BOX_DATA_CO2', 0) or 0)
        print(f"Current Humidity: {current_humidity}")
        print(f"Current Temperature: {current_temp}")
        print(f"Current CO2: {current_co2}")

        humidity_is_high = current_humidity > self.HIGH_HUMIDITY_THRESHOLD
        temp_is_low = current_temp < self.LOW_TEMP_THRESHOLD

        # if humidity_is_high and temp_is_low:
        #     current_time = time.time()
        #     if self.humidity_start is None:
        #         self.humidity_start = current_time
        #     elif current_time - self.humidity_start >= 10800:  # 3 hours in seconds
        #         humidity_time = current_time - self.humidity_start
        #         print("Elevated unwarranted humidity detected!")
        #         await turn_on_fan()
        #         await status_update(f"⚠️ HIGH HUMIDITY DETECTED! High humidity with low temperature for {humidity_time} seconds")
        #         self.humidity_start = None

        if humidity_is_high and self.is_fan_on == False:
            await turn_on_fan()
            self.is_fan_on = True

            if self.humid_check == False:
                # await status_update(f"⚠️ HIGH HUMIDITY DETECTED! High humidity with low temperature for 30 minutes")
                self.humid_check = True
            self.humidity_start = None

        elif(humidity_is_high == False and self.is_fan_on):
            await turn_off_fan()
            self.is_fan_on = False
            self.humidity_start = None

    async def check_fire(self, data):
        smoke_level = int(data.get('GAS_SENSOR_DATA', 0) or 0)
        fire_detected = smoke_level > self.SMOKE_FIRE_THRESHOLD
        # print(f"Smoke Level: {smoke_level}")

        if fire_detected:
            if not self.fire_detected:
                print("Fire detected!")
                # await status_update("🔥 FIRE DETECTED! High smoke levels detected.")
                self.fire_detected = True
        else:
            self.fire_detected = False

    async def check_water_leak(self, data):
        moisture_level = int(data.get('MOISTURE_SENSOR_DATA', 0) or 0)
        water_leak_detected = moisture_level > self.MOISTURE_THRESHOLD
        print(f"Moisture Level: {moisture_level}")

        if water_leak_detected:
            print("Water leak detected!")
            if self.water_leak_check == False:
                await status_update("💧 WATER LEAK DETECTED! High moisture levels detected.")
                self.water_leak_check = True
        else:
            self.water_leak_detected = False

    def reset(self):
        """ Reset all monitoring states """
        self.gas_leak_start = None
        self.unattended_stove_start = None
        self.humidity_start = None
        self.fire_detected = False


# Example usage:
if __name__ == "__main__":
    monitor = KitchenSafetyMonitor()
    sample_data = {
        "SMART_AIR_BOX_DATA_HUMIDITY": "100",
        "SMART_AIR_BOX_DATA_TEMPERATURE": "100",
        "MOTION_SENSOR_DATA": "Motion Detected",
        "MOISTURE_SENSOR_DATA": "100",
        "SMOKE_SENSOR_DATA": "1000",
        "GAS_SENSOR_DATA": "1000"
    }
    monitor.process_data(sample_data)