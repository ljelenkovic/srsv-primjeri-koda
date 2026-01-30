import pika, json, time, socket


class Zaposlenik:
    def __init__(self):
        self.ime = socket.gethostname()
        self.ponuda = ["Ruža", "Tulipan", "Orhideja", "Ljiljan", "Suncokret"]

        self.connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
        self.channel = self.connection.channel()

        self.channel.queue_declare(queue='upiti_za_ponudu')
        self.channel.queue_declare(queue='narudzbe')
        self.channel.queue_declare(queue='potvrde_prodaje')

    def odgovori_na_upit(self, ch, method, props, body):
        time.sleep(2)
        print(f"[{self.ime}] Šaljem ponudu kupcu...\n")
        ch.basic_publish(
            exchange='',
            routing_key=props.reply_to,
            properties=pika.BasicProperties(correlation_id=props.correlation_id),
            body=json.dumps(self.ponuda)
        )
        ch.basic_ack(delivery_tag=method.delivery_tag)

    def izvrsi_narudzbu(self, ch, method, props, body):
        podaci = json.loads(body)
        print(f"[{self.ime}] Radim na narudžbi: {podaci['stavka']}")
        time.sleep(2)  # Simulacija rada

        potvrda_za_kupca = f"Tvoj buket ({podaci['stavka']}) je spreman!"
        ch.basic_publish(
            exchange='',
            routing_key=props.reply_to,
            properties=pika.BasicProperties(correlation_id=props.correlation_id),
            body=json.dumps(potvrda_za_kupca)
        )

        ch.basic_publish(exchange='', routing_key='potvrde_prodaje', body=body)
        print(f"[{self.ime}] Narudžba {podaci['stavka']} poslana na blagajnu.\n")
        ch.basic_ack(delivery_tag=method.delivery_tag)

    def pocni_raditi(self):
        # Postavljanje prioriteta i osluškivanja
        self.channel.basic_qos(prefetch_count=1)
        self.channel.basic_consume(queue='upiti_za_ponudu', on_message_callback=self.odgovori_na_upit)
        self.channel.basic_consume(queue='narudzbe', on_message_callback=self.izvrsi_narudzbu)

        print(f"Zaposlenik {self.ime} je otvorio radnju...\n")
        self.channel.start_consuming()


if __name__ == "__main__":
    radnik = Zaposlenik()
    radnik.pocni_raditi()