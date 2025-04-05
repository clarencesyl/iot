class DataManager:
    _instance = None
    latest_data = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
        return cls._instance

    @classmethod
    def update_data(cls, data):
        cls.latest_data = data

    @classmethod
    def get_data(cls):
        return cls.latest_data
