import random 
import time
from pika_utils import pika_send, pika_receive, pika_create_channel, pika_connect
from pika.adapters.blocking_connection import BlockingChannel
from pika.spec import Basic, BasicProperties
from datetime import datetime

jelo = ""


def naruci_jelo(kanal: BlockingChannel):
    global jelo
    jelo = random.choice(["Lazanje", "Kremšnita", "Toretline", "Carbonara"])

    pika_send(kanal, "gost-konobar", jelo)
    pika_receive(kanal, "konobar-gost", zaprimi_jelo)


def zaprimi_jelo(channel: BlockingChannel,
                method: Basic.Deliver,
                properties: BasicProperties,
                body: bytes,):
    global jelo
    message = body.decode()
    print(f" Gost je zaprimio jelo {message}")
    if message == jelo:
        channel.stop_consuming()
    else:
        print("Vrijeme pogreške:", datetime.now())
        raise ValueError(f"Krivo jelo zaprimljeno, očekivano: {jelo}, zaprimljeno: {message}")
    

def main():
    print("Vrijeme pokretanja:", datetime.now())

    konekcija = pika_connect()
    kanal = pika_create_channel(konekcija)
    for i in range(10):
        naruci_jelo(kanal)
        time.sleep(random.randint(2, 6))

if __name__ == "__main__":
    main()