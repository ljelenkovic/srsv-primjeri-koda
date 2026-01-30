from typing import Callable
import pika
from pika.adapters.blocking_connection import BlockingChannel
from datetime import datetime

def pika_connect():
    hostname = "localhost"
    connection = pika.BlockingConnection(pika.ConnectionParameters(hostname))

    return connection

def pika_create_channel(connection: pika.BlockingConnection):
    return connection.channel()

def pika_send(channel: BlockingChannel, queue_name: str, message: str):
    channel.queue_declare(queue=queue_name)

    channel.basic_publish(exchange='',
                        routing_key=queue_name,
                        body=message)
    
    print(f" {message} poslano u {queue_name}")


def pika_receive(channel: BlockingChannel, queue_name: str, pika_callback: Callable):
    channel.queue_declare(queue=queue_name)

    print(f" {queue_name} čeka")
    for method_frame, properties, body in channel.consume(
                                                        queue=queue_name,
                                                        inactivity_timeout=4,
                                                        auto_ack=True,
                                                    ):
        if method_frame is None:
            channel.cancel()
            print("Vrijeme pogreške:", datetime.now())
            raise TimeoutError(f"ništa nije zaprimljeno 4 sekunde u {queue_name}, započinje se ponovno pokretanje")

        pika_callback(channel, method_frame, properties, body)