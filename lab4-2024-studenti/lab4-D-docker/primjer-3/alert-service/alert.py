import pika
import pika.exceptions
import json
import time
import logging

# Basic logging configuration
logging.basicConfig(level=logging.INFO) 

def connect_to_rabbitmq():
    while True:
        try:
            connection = pika.BlockingConnection(pika.ConnectionParameters('rabbitmq'))
            return connection
        except pika.exceptions.AMQPConnectionError:
            time.sleep(1)

def callback(ch, method, properties, body):
    alert = json.loads(body)
    logging.info(f"ALERT: {alert['message']} (Device: {alert['deviceId']}, Time: {alert['timestamp']})")

connection = connect_to_rabbitmq()
channel = connection.channel()
channel.queue_declare(queue='alerts')
channel.basic_consume(queue='alerts', on_message_callback=callback, auto_ack=True)
logging.info("Alert Service is running...")
channel.start_consuming()
