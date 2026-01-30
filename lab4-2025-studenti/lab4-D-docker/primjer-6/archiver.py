from datetime import datetime
import pika
import time
import sys
import json

# Ime usluge za razmjenu poruka
hostname = "rabbitmq"

message_N = 0
latency_to_censor = 0 # kašnjenje u ms
latency_to_archiver = 0

# Spajanje na uslugu za razmjenu poruka
def connect_to_rabbitmq():
    while True:
        try:
            connection = pika.BlockingConnection(pika.ConnectionParameters(hostname))
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            print(f"[{timestamp}] Arhiver: Uspješno spojeno na RabbitMQ")
            return connection
        except pika.exceptions.AMQPConnectionError:
            time.sleep(1)

# Obrada nadolazećih poruka (iz cenzora)
def callback(ch, method, properties, body):
    try:
        # Parsiranje poruke
        payload = json.loads(body.decode())
        sender_id = payload["sender_id"]
        original_timestamp = payload["original_timestamp"]
        censored_timestamp = payload["censored_timestamp"]
        message = payload["message"]

        # Ispiši konačnu poruku, identifikator člana, trenutno (i originalno) vrijeme
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        #print(f"[{timestamp}] Arhiver: [{original_timestamp}] {sender_id}: {message}")
        print(f"[{timestamp}] {sender_id}: {message}")
        
        # Praćenje statistike kašnjenja
        global message_N, latency_to_censor, latency_to_archiver
        message_N += 1
        
        fmt = "%H:%M:%S.%f"
        original_time = datetime.strptime(original_timestamp + "000", fmt)
        cenzor_time = datetime.strptime(censored_timestamp + "000", fmt)
        current_time = datetime.strptime(timestamp + "000", fmt)
        
        latency_to_censor += (cenzor_time - original_time).total_seconds() * 1000
        latency_to_archiver += (current_time - cenzor_time).total_seconds() * 1000
        
        print(f"[{timestamp}] Srednja kašnjenja: {latency_to_censor / message_N:.3f} ms do moderatora - "
        f"{latency_to_archiver / message_N:.3f} ms do arhivera - "
        f"{(latency_to_censor + latency_to_archiver) / message_N:.3f} ms ukupno")
        
        sys.stdout.flush()

    except Exception as e:
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"[{timestamp}] Arhiver: Greška u obradi {e}")
        sys.stdout.flush()


# Otvaranje kanala prema usluzi za razmjenjivanje poruka
connection = connect_to_rabbitmq()
channel = connection.channel()

# Pripremanje reda za primanje poruka
channel.queue_declare(queue="censor_to_archiver")

# Pretplata reakcije na nadolazeće poruke (iz cenzora)
channel.basic_consume(
    queue="censor_to_archiver",
    on_message_callback=callback,
    auto_ack=True
)

sys.stdout.flush()
# Pokreni obradu
channel.start_consuming()
