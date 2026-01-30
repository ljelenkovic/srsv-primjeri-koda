import pika, time


def connect():
    while True:
        try:
            return pika.BlockingConnection(
                pika.ConnectionParameters(host='rabbitmq')
            )
        except pika.exceptions.AMQPConnectionError:
            print("Waiting for RabbitMQ...", flush=True)
            time.sleep(2)

connection = connect() 
channel = connection.channel()
channel.queue_declare(queue='hello')

while True:
    channel.basic_publish(exchange='', routing_key='hello', body="Hello World!")
    print("Sent Hello World!")
    time.sleep(2)
