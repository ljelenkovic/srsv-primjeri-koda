#!/usr/bin/env python3
import pika
import time
import random
import json
import sys
from datetime import datetime

PRODUCTS = [
    "Laptop", "Smartphone", "Headphones", "Keyboard", "Mouse",
    "Monitor", "Webcam", "USB Cable", "Hard Drive", "RAM"
]

def generate_order():
    if random.random() < 0.2:
        bad_data_type = random.choice(['negative_quantity', 'negative_price', 'huge_number', 'zero'])
        
        if bad_data_type == 'negative_quantity':
            quantity = random.randint(-50, -1)
            price = round(random.uniform(10.0, 1000.0), 2)
        elif bad_data_type == 'negative_price':
            quantity = random.randint(1, 10)
            price = round(random.uniform(-1000.0, -0.01), 2)
        elif bad_data_type == 'huge_number':
            quantity = random.randint(1000000, 9999999)
            price = round(random.uniform(1.0, 1000.0), 2)
        else:
            quantity = 0
            price = 0.0
    else:
        quantity = random.randint(1, 10)
        price = round(random.uniform(10.0, 1000.0), 2)
    
    order = {
        "order_id": f"ORD-{random.randint(10000, 99999)}",
        "product": random.choice(PRODUCTS),
        "quantity": quantity,
        "price": price
    }
    
    return order

def main():
    startup_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
    print(f'[SUPPLIER] Started at: {startup_time}', flush=True)
    
    max_retries = 30
    retry_delay = 2
    connection = None
    time.sleep(10)
    for attempt in range(max_retries):
        try:
            print(f'[SUPPLIER] Attempting to connect to RabbitMQ (attempt {attempt + 1}/{max_retries})...', flush=True)
            connection = pika.BlockingConnection(
                pika.ConnectionParameters(host='rabbitmq'))
            print('[SUPPLIER] Successfully connected to RabbitMQ!', flush=True)
            break
        except pika.exceptions.AMQPConnectionError:
            if attempt < max_retries - 1:
                print(f'[SUPPLIER] Connection failed, retrying in {retry_delay} seconds...', flush=True)
                time.sleep(retry_delay)
            else:
                print('[SUPPLIER] Failed to connect to RabbitMQ after maximum retries', flush=True)
                sys.exit(1)
    
    channel = connection.channel()

    channel.queue_declare(queue='raw_orders')

    print('[SUPPLIER] Connected to RabbitMQ. Generating orders...', flush=True)
    
    try:
        counter = 0
        while True:
            order = generate_order()
            
            channel.basic_publish(
                exchange='',
                routing_key='raw_orders',
                body=json.dumps(order))
            
            counter += 1
            current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
            
            is_suspicious = order['quantity'] <= 0 or order['price'] <= 0 or order['quantity'] > 1000
            flag = "SUSPICIOUS" if is_suspicious else ""
            
            print(f'[SUPPLIER] [{current_time}] Order #{counter}: {order["product"]} | '
                  f'Qty: {order["quantity"]} | Price: ${order["price"]:.2f}{flag}', flush=True)
            
            time.sleep(5)
                
    except KeyboardInterrupt:
        print('[SUPPLIER] Interrupted by user', flush=True)
    finally:
        shutdown_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        print(f'[SUPPLIER] Shutting down at: {shutdown_time}', flush=True)
        connection.close()

if __name__ == '__main__':
    main()