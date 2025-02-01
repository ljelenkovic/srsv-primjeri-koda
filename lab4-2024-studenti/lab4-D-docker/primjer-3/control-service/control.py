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
    data = json.loads(body)
    logging.info(f"Received: {data}")
    if data['temperature'] > 35:
        alert = {
            "message": f"High temperature detected: {data['temperature']}°C",
            "deviceId": data['deviceId'],
            "timestamp": data['timestamp']
        }
        channel.basic_publish(exchange='', routing_key='alerts', body=json.dumps(alert))
        logging.info(f"Alert sent: {alert}")

connection = connect_to_rabbitmq()
channel = connection.channel()
channel.queue_declare(queue='sensor.data')
channel.queue_declare(queue='alerts')
channel.basic_consume(queue='sensor.data', on_message_callback=callback, auto_ack=True)
logging.info("Control Service is running...")
channel.start_consuming()
