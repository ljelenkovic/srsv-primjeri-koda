#!/usr/bin/env python
import json
import os
import sys
import time

import pika

hostname = "localhost"
SERVICE_ID = os.getenv('SERVICE_ID')
FAULTY_SIMULATION = os.getenv('FAULTY_SIMULATION', 'false').lower() == 'true'
count = 0
def main():
    connection = pika.BlockingConnection(pika.ConnectionParameters(hostname))
    channel = connection.channel()
    request_queue = f'heartbeat_requests_{SERVICE_ID}'
    channel.queue_declare(queue=request_queue)
    response_queue = f'heartbeat_response_{SERVICE_ID}'
    channel.queue_declare(queue=response_queue)

    def on_heartbeat_request(ch, method, properties, body):
        try:
            global count
            payload = json.loads(body)
            request_id = payload['healthcheckId']

            if FAULTY_SIMULATION:
                # Simulate error for every 10th heartbeat
                count +=1;  
                print(f"Count {count}",flush=True)
                if count % 10 == 0:
                    raise Exception(f"[FAULTY_SIMULATION] Skipping response for heartbeat request {request_id}")

            # Prepare the response
            response_payload = {
                'service': SERVICE_ID,
                'request_id': request_id,
                'message': "I am alive",
                'timestamp': time.time()
            }
            ch.basic_publish(exchange='', routing_key=response_queue, body=json.dumps(response_payload))
            print(f"Responded to heartbeat {request_id} - {response_queue}",flush=True)
            ch.basic_ack(delivery_tag=method.delivery_tag)
        except Exception as e:
            ch.basic_nack(delivery_tag=method.delivery_tag, requeue=True)
            ch.stop_consuming()
            raise Exception(e)

    channel.basic_consume(queue=f'heartbeat_requests_{SERVICE_ID}', on_message_callback=on_heartbeat_request)
    print("Listening for heartbeats...",flush=True)
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
