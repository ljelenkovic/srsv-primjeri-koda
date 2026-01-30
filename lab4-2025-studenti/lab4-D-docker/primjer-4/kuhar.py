import random 
import time
from pika_utils import pika_send, pika_receive, pika_connect, pika_create_channel
from pika.adapters.blocking_connection import BlockingChannel
from pika.spec import Basic, BasicProperties
from datetime import datetime

jelo = ""

def obavi_narudzbu(kanal: BlockingChannel):
    global jelo
    pika_receive(kanal, "konobar-kuhar", zaprimi_narudzbu)

    greska = random.randint(1,100)
    if greska > 90:
        jelo += "a"

    time.sleep(3)
    pika_send(kanal, "kuhar-konobar", jelo)


def zaprimi_narudzbu(channel: BlockingChannel,
                method: Basic.Deliver,
                properties: BasicProperties,
                body: bytes,):
    global jelo
    message = body.decode()
    print(f" Kuhar je zaprimio narudzbu {message}")
    jelo = message
    channel.stop_consuming()

def main():
    print("Vrijeme pokretanja:", datetime.now())

    konekcija = pika_connect()
    kanal = pika_create_channel(konekcija)
    while(True):
        obavi_narudzbu(kanal)

if __name__ == "__main__":
    main()