#!/usr/bin/env python3
import pika
import json
import sys
import time
from datetime import datetime

class InvalidOrderException(Exception):
    """Exception raised when order contains invalid data"""
    pass

def validate_and_print_order(processed_order):
    """Validate order and print receipt. Crashes on invalid data."""
    
    order_id = processed_order['order_id']
    product = processed_order['product']
    quantity = processed_order['quantity']
    unit_price = processed_order['unit_price']
    total = processed_order['total']
    
    if quantity < 0:
        raise InvalidOrderException(f"CRITICAL ERROR: Negative quantity detected! ({quantity})")
    
    if unit_price < 0:
        raise InvalidOrderException(f"CRITICAL ERROR: Negative price detected! (${unit_price})")
    
    if total < 0:
        raise InvalidOrderException(f"CRITICAL ERROR: Negative total detected! (${total})")
    
    if quantity == 0:
        raise InvalidOrderException(f"CRITICAL ERROR: Zero quantity order detected!")
    
    if unit_price == 0:
        raise InvalidOrderException(f"CRITICAL ERROR: Zero price detected!")
    
    if quantity > 10000:
        raise InvalidOrderException(f"CRITICAL ERROR: Suspiciously large quantity! ({quantity})")
    
    if total > 1000000:
        raise InvalidOrderException(f"CRITICAL ERROR: Suspiciously large total! (${total})")
    
    current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
    
    print(f'\n{"="*60}', flush=True)
    print(f'[RECEIPT] [{current_time}] ORDER CONFIRMED', flush=True)
    print(f'{"="*60}', flush=True)
    print(f'Order ID:      {order_id}', flush=True)
    print(f'Product:       {product}', flush=True)
    print(f'Quantity:      {quantity}', flush=True)
    print(f'Unit Price:    ${unit_price:.2f}', flush=True)
    print(f'TOTAL:         ${total:.2f}', flush=True)
    print(f'{"="*60}\n', flush=True)

def main():
    startup_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
    print(f'[RECEIPT] Started at: {startup_time}', flush=True)
    
    max_retries = 30
    retry_delay = 2
    connection = None
    for attempt in range(max_retries):
        try:
            print(f'[RECEIPT] Attempting to connect to RabbitMQ (attempt {attempt + 1}/{max_retries})...', flush=True)
            connection = pika.BlockingConnection(
                pika.ConnectionParameters(host='rabbitmq'))
            print('[RECEIPT] Successfully connected to RabbitMQ!', flush=True)
            break
        except pika.exceptions.AMQPConnectionError:
            if attempt < max_retries - 1:
                print(f'[RECEIPT] Connection failed, retrying in {retry_delay} seconds...', flush=True)
                time.sleep(retry_delay)
            else:
                print('[RECEIPT] Failed to connect to RabbitMQ after maximum retries', flush=True)
                sys.exit(1)
    
    channel = connection.channel()

    channel.queue_declare(queue='processed_orders')

    print('[RECEIPT] Waiting for processed orders...', flush=True)

    def callback(ch, method, properties, body):
        try:
            processed_order = json.loads(body)
            
            validate_and_print_order(processed_order)
            
            ch.basic_ack(delivery_tag=method.delivery_tag)
            
        except InvalidOrderException as e:
            current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
            print(f'\n[RECEIPT] [{current_time}]  {str(e)}', flush=True)
            print(f'[RECEIPT] Shutting down at: {current_time}', flush=True)
            ch.basic_ack(delivery_tag=method.delivery_tag)
            
            try:
                connection.close()
            except:
                pass
            
            sys.exit(1)
            
        except Exception as e:
            current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
            print(f'[RECEIPT] [{current_time}] Unexpected error: {e}', flush=True)
            ch.basic_nack(delivery_tag=method.delivery_tag)

    channel.basic_consume(
        queue='processed_orders',
        on_message_callback=callback,
        auto_ack=False)

    try:
        channel.start_consuming()
    except KeyboardInterrupt:
        print('[RECEIPT] Interrupted by user', flush=True)
        channel.stop_consuming()
    finally:
        shutdown_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        print(f'[RECEIPT] Final shutdown at: {shutdown_time}', flush=True)
        try:
            connection.close()
        except:
            pass 

if __name__ == '__main__':
    main()