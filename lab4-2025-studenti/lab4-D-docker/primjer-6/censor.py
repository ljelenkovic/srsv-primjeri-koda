import pika
import time
import sys
import json
from datetime import datetime

# Ime usluge za razmjenu poruka
hostname = "rabbitmq"

# Set zabranjenih riječi za cenzuriranje
BANNED_WORDS = [
    "stupid", "idiot", "dumb", "hate", "fool", "moron", "terrible"
]

# Odmak za simulirani prekid u sekundama
CRASH_AFTER_SECONDS = 30
# Početak simulacije postavlja se nakon spajanja na uslugu razmjene poruka
start_time = None

# Spajanje na uslugu za razmjenu poruka
def connect_to_rabbitmq():
    while True:
        try:
            connection = pika.BlockingConnection(pika.ConnectionParameters(hostname))
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            print(f"[{timestamp}] Cenzor: Uspješno spojeno na RabbitMQ")
            return connection
        except pika.exceptions.AMQPConnectionError:
            time.sleep(1)

# Kod cenzuriranja, zamjenjuje zabranjene riječi nizom zvjezdica (*)
def censor_message(message):
    words = message.split()
    censored_words = []

    for word in words:
        word_lower = word.lower()
        if word_lower in BANNED_WORDS:
            censored_words.append("*" * len(word))
        else:
            censored_words.append(word)

    return " ".join(censored_words)

# Obrada nadolazećih poruka (od članova)
def callback(ch, method, properties, body):
    try:
        # Parsiranje poruke
        payload = json.loads(body.decode())
        sender_id = payload["sender_id"]
        original_timestamp = payload["timestamp"]
        original_message = payload["message"]
        
        # timestamp = datetime.now().strftime("%H:%M:%S")
        # print(f"[{timestamp}] Cenzor {sender_id} original: {original_message}")
        sys.stdout.flush()

        # Cenzuriranje poruke
        censored_message = censor_message(original_message)

        #if (censored_message != original_message):
            #print(f"[{timestamp}] Cenzor {sender_id} cenzurirano: {censored_message}")
        sys.stdout.flush()

        # Stvaranje cenzuriranje pošiljke za arhiver
        censored_payload = {
            "sender_id": sender_id,
            "original_timestamp": original_timestamp,
            "censored_timestamp": datetime.now().strftime("%H:%M:%S.%f")[:-3],
            "message": censored_message
        }

        # Prosljeđivanje arhiveru
        ch.basic_publish(
            exchange="",
            routing_key="censor_to_archiver",
            body=json.dumps(censored_payload)
        )
        sys.stdout.flush()

        # Simulacija prekida u predodređenom trenutku
        if (time.time() - start_time > CRASH_AFTER_SECONDS):
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            print(f"[{timestamp}] Cenzor: Simulacija greške")
            sys.stdout.flush()
            sys.exit(1)

    except Exception as e:
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"[{timestamp}] Cenzor: Greška u obradi {e}")
        sys.stdout.flush()


# Otvaranje kanala prema usluzi za razmjenjivanje poruka
connection = connect_to_rabbitmq()
channel = connection.channel()
start_time = time.time()

# Pripremanje redova za slanje i primanje poruka
channel.queue_declare(queue="member_to_censor")
channel.queue_declare(queue="censor_to_archiver")

# Pretplata reakcije na nadolazeće poruke (od članova)
channel.basic_consume(
    queue="member_to_censor",
    on_message_callback=callback,
    auto_ack=True
)

sys.stdout.flush()
# Pokreni obradu
channel.start_consuming()
