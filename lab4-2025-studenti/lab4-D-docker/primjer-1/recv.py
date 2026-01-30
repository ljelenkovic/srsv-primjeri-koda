import pika, sys, os, time

def main():
    
    connection = pika.BlockingConnection(pika.ConnectionParameters(host='rabbitmq'))
    channel = connection.channel()

    channel.queue_declare(queue='receiver_queue')

    def callback(ch, method, properties, body):
        print(f"Received '{body}'")

    channel.basic_consume(queue='receiver_queue', on_message_callback=callback, auto_ack=True)

    print('Waiting for messages. To exit press CTRL+C.')

    try:
        channel.start_consuming()
    except KeyboardInterrupt:
        print('Interrupted')
    finally:
        channel.stop_consuming()
        connection.close()

    return 0

main()