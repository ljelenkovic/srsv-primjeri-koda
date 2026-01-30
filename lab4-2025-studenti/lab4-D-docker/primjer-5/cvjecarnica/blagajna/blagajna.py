import pika, json

class Blagajna:
    def __init__(self):
        self.ukupno_promet = 0
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
        self.channel = self.connection.channel()
        self.channel.queue_declare(queue='potvrde_prodaje')

    def procesiraj_racun(self, ch, method, props, body):
        podaci = json.loads(body)
        self.ukupno_promet += 10 # Fiksna cijena
        print(f"--- BLAGAJNA --- Prodana: {podaci['stavka']} | Promet: {self.ukupno_promet} €")
        ch.basic_ack(delivery_tag=method.delivery_tag)

    def pokreni_blagajnu(self):
        self.channel.basic_consume(queue='potvrde_prodaje', on_message_callback=self.procesiraj_racun)
        print("Blagajna spremna za račune...")
        self.channel.start_consuming()

if __name__ == "__main__":
    blagajna = Blagajna()
    blagajna.pokreni_blagajnu()