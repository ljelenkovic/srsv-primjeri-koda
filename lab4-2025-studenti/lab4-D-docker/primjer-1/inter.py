import pika, sys, os, time

def alterMessage(key):

    return key * 2

def main():

    connection = pika.BlockingConnection(pika.ConnectionParameters(host='rabbitmq'))
    channel = connection.channel()

    channel.queue_declare(queue='sender_queue')
    channel.queue_declare(queue='receiver_queue')

    def callback(ch, method, properties, body):
        print(f"Received '{body}'")
        value = body.decode()
        if (value == '#'):
            raise ValueError(f"Invalid value ({body})")
        value = alterMessage(body)
        time.sleep(2)
        channel.basic_publish(exchange='', routing_key='receiver_queue', body=f'{value}')
        print(f"Sent '{value}' through")
    
    channel.basic_consume(queue='sender_queue', on_message_callback=callback, auto_ack=True)

    print('Starting to intermediate. To exit press CTRL+C.')

    try:
        channel.start_consuming()
    except ValueError:
        print("Service failure")
    finally:
        channel.stop_consuming()
        connection.close()
        sys.exit(1)

    return 0

main()