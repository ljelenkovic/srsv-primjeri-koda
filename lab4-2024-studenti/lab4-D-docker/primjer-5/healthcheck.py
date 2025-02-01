#!/usr/bin/env python
import json
import os
import sys
import time

import pika

hostname = "localhost"
print("Script started",flush=True)
response_times = []

def main():
    print("Entered main",flush=True)
    connection = pika.BlockingConnection(pika.ConnectionParameters(hostname))
    channel = connection.channel()
    channel.queue_declare(queue='heartbeat_requests')

    for i in range(1, 101):
        global response_times
        response_times = []
        requestTime = time.time()
        payload = {'healthcheckId': i,'message' : "Are you alive?", 'timestamp': requestTime}

        # Publish the heartbeat request
        channel.basic_publish(exchange='', routing_key=f'heartbeat_requests_2', body=json.dumps(payload))
        channel.basic_publish(exchange='', routing_key=f'heartbeat_requests_3', body=json.dumps(payload))
        print(f"[Heartbeat {i}] Sent: {payload}",flush=True)

        c2_queue = f'heartbeat_response_2'
        c3_queue = f'heartbeat_response_3'
        connection_process = pika.BlockingConnection(pika.ConnectionParameters(hostname))
        channel_process = connection_process.channel()
        channel_process.queue_declare(queue=c2_queue)
        channel_process.queue_declare(queue=c3_queue)
        
        def on_response(ch, method, properties, body):
            response = json.loads(body)
            responseTime = (response['timestamp'] - requestTime) * 1000
            response_times.append(responseTime)
            print(f"[Response from {response['service']}] Time: {responseTime:.4f} ms Response : {response}",flush=True)
            if len(response_times)>=2:
                ch.stop_consuming()
        
        # Set up listeners for both queues
        channel_process.basic_consume(queue=c2_queue, on_message_callback=on_response, auto_ack=True)
        channel_process.basic_consume(queue=c3_queue, on_message_callback=on_response, auto_ack=True)

        # Wait for both responses
        print(f"Waiting for responses from c2 and c3 for {i}...",flush=True)
        channel_process.start_consuming()
        
        time.sleep(5)

if __name__ == "__main__":
    try:
        time.sleep(5)
        main()
    except KeyboardInterrupt:
        print("Interrupted",flush=True)
        try:
            sys.exit(0)
        except SystemExit:
            os._exit(0)
