import pika, sys, os, time

def generateMessage(key):

    return key + 1

def main():

    connection = pika.BlockingConnection(pika.ConnectionParameters(host='rabbitmq'))
    channel = connection.channel()

    channel.queue_declare(queue='sender_queue')

    value = 0
    print("Starting to send messages. To exit press CTRL+C.")

    try:
        while(value < 1000):
            value = generateMessage(value)
            if (value % 10 == 0):
                channel.basic_publish(exchange='', routing_key='sender_queue', body='#')
                print(f"Sent '#'")
            else:
                channel.basic_publish(exchange='', routing_key='sender_queue', body=f'{value}')
                print(f"Sent '{value}'")
            time.sleep(3)
    
    except KeyboardInterrupt:
        print('Interrupted')
    
    finally:
        connection.close()
    
    return 0

main()