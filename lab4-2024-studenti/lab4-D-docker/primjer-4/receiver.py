import os
import sys
import pika
import time
from pika.exceptions import AMQPConnectionError

def get_timestamp():
    current_time = time.time()
    formatted_time = time.strftime('%H:%M:%S', time.gmtime(current_time))
    milliseconds = int((current_time % 1) * 1000)  
    timestamp = f"{formatted_time}.{milliseconds:03d}"  
    return timestamp

def wait_for_rabbitmq(hostname):
    while True:
        try:
            connection = pika.BlockingConnection(pika.ConnectionParameters(host=hostname))
            connection.close()
            timestamp = get_timestamp()
            print(f" [{timestamp}] RabbitMQ is available!")
            break
        except AMQPConnectionError:
            print("RabbitMQ not available, retrying...")
            time.sleep(2)


def main():
    hostname = "127.0.0.1"
    wait_for_rabbitmq(hostname)

    connection = pika.BlockingConnection(pika.ConnectionParameters(host=hostname))
    channel = connection.channel()

    channel.queue_declare(queue="hello")

    def callback(ch, method, properties, body):
        timestamp = get_timestamp()
        print(f" [{timestamp}] Received {body.decode()}")

    channel.basic_consume(queue="hello", on_message_callback=callback, auto_ack=True)

    print(" [*] Waiting for messages. To exit press CTRL+C")
    channel.start_consuming()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("Interrupted")
        try:
            sys.exit(0)
        except SystemExit:
            os._exit(0)

