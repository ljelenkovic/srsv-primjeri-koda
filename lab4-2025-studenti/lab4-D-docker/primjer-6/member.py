import pika
import random
import time
import sys
import json
import socket
from datetime import datetime

# Ime usluge za razmjenu poruka
hostname = "rabbitmq"

# Postavljanje unikatnog identifikatora za pojedinog člana foruma
# (Koristi identifikator spremnika za konzistentnost)
MEMBER_ID = socket.gethostname()[:12]
# Postavljanje nasumičnog generatora za simulaciju za svaku instancu
random.seed(MEMBER_ID)
SHUTDOWN_OFFSET = random.uniform(30, 180) # Odmak za zaustavljanje u sekundama

# Set nasumičnih, mogućih riječi unutar poruke
WORDS = [
    "hello", "world", "forum", "post", "message", "today", "weather",
    "nice", "great", "awesome", "terrible", "stupid", "idiot", "dumb",
    "amazing", "wonderful", "hate", "love", "like", "dislike", "python",
    "docker", "rabbitmq", "system", "real-time", "distributed", "fool",
    "brilliant", "smart", "moron", "genius", "terrible", "fantastic"
]

# Spajanje na uslugu za razmjenu poruka
def connect_to_rabbitmq():
    while True:
        try:
            connection = pika.BlockingConnection(pika.ConnectionParameters(hostname))
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            print(f"[{timestamp}] {MEMBER_ID}: Uspješno spojeno na RabbitMQ")
            return connection
        except pika.exceptions.AMQPConnectionError:
            time.sleep(1)

# Petlja čekanja na dostupnost kanala
def wait_for_queue(channel):
    while True:
        try:
            channel.queue_declare(queue="member_to_censor", passive=True)
            break
        except pika.exceptions.ChannelClosedByBroker:
            time.sleep(1)
            connection = connect_to_rabbitmq()
            channel = connection.channel()
    return channel

# Stvaranje nasumične poruke iz seta riječi
def generate_random_message():
    num_words = random.randint(3, 8)
    return " ".join(random.choices(WORDS, k=num_words))


# Otvaranje kanala prema usluzi za razmjenjivanje poruka
connection = connect_to_rabbitmq()
channel = connection.channel()

# Čekanje na dostupnost usluge
channel = wait_for_queue(channel)

# Petlja simulacije slanja poruka
try:
    start_time = time.time()
    # Odmakni početak dok usluga ne bude dostupna
    time.sleep(10)

    while True:
        message = generate_random_message()
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"[{timestamp}] {MEMBER_ID}: {message}")

        payload = {
            "sender_id": MEMBER_ID,
            "timestamp": timestamp,
            "message": message
        }

        channel.basic_publish(
            exchange="",
            routing_key="member_to_censor",
            body=json.dumps(payload)
        )
        sys.stdout.flush()

        # Prekini rad u nekom nasumičnom trenutku
        if (time.time() - start_time > SHUTDOWN_OFFSET):
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            print(f"[{timestamp}] {MEMBER_ID}: Odspajanje")
            sys.stdout.flush()
            sys.exit(1)

        time.sleep(random.uniform(1, 2))

except KeyboardInterrupt:
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print(f"[{timestamp}] {MEMBER_ID}: Prekinuto slanje")
finally:
    connection.close()
