import sys
import pika
import pika.exceptions
import json
import time
from random import uniform
import logging
from datetime import datetime

# Basic logging configuration
logging.basicConfig(level=logging.INFO) 

def connect_to_rabbitmq():
    while True:
        try:
            connection = pika.BlockingConnection(pika.ConnectionParameters('rabbitmq'))
            return connection
        except pika.exceptions.AMQPConnectionError:
            time.sleep(1)

logging.info(f"Service started at {datetime.now().time()}")

connection = connect_to_rabbitmq()
channel = connection.channel()
channel.queue_declare(queue='sensor.data')

while True:
    data = {
        "deviceId": "sensor-1",
        "temperature": round(uniform(20, 40), 2),
        "humidity": round(uniform(40, 80), 2),
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    }

    if data['temperature'] > 30 and data['humidity'] > 70:
        logging.error(f"Sensor failed because of high temperature and humidity! Time: {datetime.now().time()}")
        sys.exit(1)

    channel.basic_publish(exchange='', routing_key='sensor.data', body=json.dumps(data))
    logging.info(f"Sent: {data}")
    time.sleep(5)
