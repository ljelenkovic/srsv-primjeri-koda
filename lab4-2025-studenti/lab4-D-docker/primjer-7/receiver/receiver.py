import pika, time, os, datetime, random, sys



def connect():
    while True:
        try:
            return pika.BlockingConnection(
                pika.ConnectionParameters(host='rabbitmq')
            )
        except pika.exceptions.AMQPConnectionError:
            print("Waiting for RabbitMQ...", flush=True)
            time.sleep(2)

def main():
    print("Worker START:", datetime.datetime.now())

    connection = connect() 
    channel = connection.channel()
    channel.queue_declare(queue='hello')

    def callback(ch, method, properties, body):
        print("Received:", body.decode())
        time.sleep(1)

        # Namjerni kvar
        if random.random() < 0.1:
            print("CRASH:", datetime.datetime.now())
            os._exit(1)

        ch.basic_ack(delivery_tag=method.delivery_tag)

    channel.basic_consume(queue='hello', on_message_callback=callback)
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
