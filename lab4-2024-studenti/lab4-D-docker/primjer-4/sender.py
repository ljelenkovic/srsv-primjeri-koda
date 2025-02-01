import time
import random
import pika
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

    # Initialize the number of messages
    num_messages = 15

    # Initialize an array of messages
    messages = [
        "Hello, World!",
        "Python is fun!",
        "Keep going, you're doing great!",
        "Random messages are exciting!",
        "Have a wonderful day!",
        "Coding is like magic!",
        "Never stop learning!",
        "Success is a journey, not a destination!",
        "Embrace the challenges!",
        "Believe in yourself!",
        "Every day is a new beginning!",
        "Stay positive and work hard!",
        "Be kind to yourself and others!",
        "You are capable of amazing things!",
        "Consistency is key to progress!"
    ]

    hostname = "127.0.0.1"
    wait_for_rabbitmq(hostname)

    connection = pika.BlockingConnection(pika.ConnectionParameters(hostname))
    channel = connection.channel()

    channel.queue_declare(queue='hello')

    # While loop to pick and print random messages
    while True:
        random_index = random.randint(0, num_messages + 1)
        timestamp = get_timestamp()
        if(random_index >= num_messages):
            print(f" [{timestamp}] Index out of bounds")
        
        channel.basic_publish(exchange='', routing_key='hello', body=messages[random_index])
        print(f" [{timestamp}] Sent: " + messages[random_index])
        time.sleep(1)  # Small delay of 1 second


if __name__ == "__main__":
    main()
