#!/usr/bin/env python3
import pika
import time
import json
import sys
import random
from datetime import datetime

def calculate_discount():
    return random.choice([0.05, 0.10, 0.15])

def process_order(order):
    quantity = order['quantity']
    price = order['price']
    
    subtotal = quantity * price
    
    discount_rate = calculate_discount()
    discount_amount = subtotal * discount_rate
    
    total = subtotal - discount_amount
    
    processed_order = {
        "order_id": order['order_id'],
        "product": order['product'],
        "quantity": quantity,
        "unit_price": price,
        "subtotal": round(subtotal, 2),
        "discount_rate": discount_rate,
        "total": round(total, 2)
    }
    
    return processed_order

def main():
    startup_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
    print(f'[PROCESSOR] Started at: {startup_time}', flush=True)
    
    max_retries = 30
    retry_delay = 2
    connection = None
    for attempt in range(max_retries):
        try:
            print(f'[PROCESSOR] Attempting to connect to RabbitMQ (attempt {attempt + 1}/{max_retries})...', flush=True)
            connection = pika.BlockingConnection(
                pika.ConnectionParameters(host='rabbitmq'))
            print('[PROCESSOR] Successfully connected to RabbitMQ!', flush=True)
            break
        except pika.exceptions.AMQPConnectionError:
            if attempt < max_retries - 1:
                print(f'[PROCESSOR] Connection failed, retrying in {retry_delay} seconds...', flush=True)
                time.sleep(retry_delay)
            else:
                print('[PROCESSOR] Failed to connect to RabbitMQ after maximum retries', flush=True)
                sys.exit(1)
    
    channel = connection.channel()

    channel.queue_declare(queue='raw_orders')
    channel.queue_declare(queue='processed_orders')

    print('[PROCESSOR] Waiting for orders to process...', flush=True)

    def callback(ch, method, properties, body):
        try:
            order = json.loads(body)
            current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
            
            print(f'[PROCESSOR] [{current_time}] Processing {order["order_id"]}: '
                  f'{order["product"]} x{order["quantity"]} @ ${order["price"]:.2f}', flush=True)
            
            processed_order = process_order(order)
            time.sleep(0.1)
            print(f'[PROCESSOR] [{current_time}] Calculated Total: ${processed_order["total"]:.2f} '
                  f'(Discount: {processed_order["discount_rate"]*100:.0f}%)', flush=True)
            
            channel.basic_publish(
                exchange='',
                routing_key='processed_orders',
                body=json.dumps(processed_order))
            
            ch.basic_ack(delivery_tag=method.delivery_tag)
            
        except Exception as e:
            print(f'[PROCESSOR] ERROR processing message: {e}', flush=True)
            ch.basic_nack(delivery_tag=method.delivery_tag)

    channel.basic_consume(
        queue='raw_orders',
        on_message_callback=callback,
        auto_ack=False)

    try:
        channel.start_consuming()
    except KeyboardInterrupt:
        print('[PROCESSOR] Interrupted by user', flush=True)
        channel.stop_consuming()
    finally:
        shutdown_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        print(f'[PROCESSOR] Shutting down at: {shutdown_time}', flush=True)
        connection.close()

if __name__ == '__main__':
    main()