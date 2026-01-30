import random 
from pika_utils import pika_send, pika_receive, pika_connect, pika_create_channel
from pika.adapters.blocking_connection import BlockingChannel
from pika.spec import Basic, BasicProperties
from datetime import datetime


jelo = ""

def obavi_narudzbu(kanal: BlockingChannel):
    global jelo
    pika_receive(kanal, "gost-konobar", zaprimi_narudzbu)

    pika_send(kanal, "konobar-kuhar", jelo)
    pika_receive(kanal, "kuhar-konobar", zaprimi_jelo)

    greska = random.randint(1,100)
    if greska > 70:
        jelo += "a"

    pika_send(kanal, "konobar-gost", jelo)

def zaprimi_narudzbu(channel: BlockingChannel,
                method: Basic.Deliver,
                properties: BasicProperties,
                body: bytes,):
    global jelo
    message = body.decode()
    print(f" Konobar je zaprimio narudzbu {message}")
    jelo = message
    channel.stop_consuming()


def zaprimi_jelo(channel: BlockingChannel,
                method: Basic.Deliver,
                properties: BasicProperties,
                body: bytes,):
    global jelo
    message = body.decode()
    print(f" Konobar je zaprimio jelo {message}")
    if message == jelo:
        channel.stop_consuming()
    else:
        print("Vrijeme pogreške:", datetime.now())
        raise ValueError(f"Krivo jelo zaprimljeno, očekivano: {jelo}, zaprimljeno: {message}")
    

def main():
    print("Vrijeme pokretanja:",datetime.now())

    konekcija = pika_connect()
    kanal = pika_create_channel(konekcija)
    while(True):
        obavi_narudzbu(kanal)

if __name__ == "__main__":
    main()