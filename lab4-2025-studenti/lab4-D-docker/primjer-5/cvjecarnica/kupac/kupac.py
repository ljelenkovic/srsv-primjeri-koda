from xmlrpc.client import Error

import pika, json, time, uuid
import random


class Kupac:
    def __init__(self):

        # Inicijalizacija veze
        print(time.time_ns())
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
        self.channel = self.connection.channel()

        # Kreiranje privremenog reda za odgovore
        result = self.channel.queue_declare(queue='', exclusive=True)
        self.callback_queue = result.method.queue

        # Postavljanje osluškivanja odgovora
        self.channel.basic_consume(
            queue=self.callback_queue,
            on_message_callback=self.on_response,
            auto_ack=True
        )

        self.response = None
        self.corr_id = str(uuid.uuid4())
        print(f"Kupac {str(self.corr_id)[:4]}: 'Idem u shopping!!!'\n")

    def on_response(self, ch, method, props, body):
        """Što kupac radi kada dobije odgovor sa ponudom."""
        if self.corr_id == props.correlation_id:
            self.response = json.loads(body)


    def pitaj_za_ponudu(self):
        """Šalje upit zaposleniku i čeka odgovor."""
        self.response = None

        print(f"Kupac {str(self.corr_id)[:4]}: 'Što imate u ponudi?'\n")
        self.channel.basic_publish(
            exchange='',
            routing_key='upiti_za_ponudu',
            properties=pika.BasicProperties(
                reply_to=self.callback_queue,
                correlation_id=self.corr_id,
            ),
            body='Daj ponudu'
        )

        while self.response is None:
            self.connection.process_data_events(time_limit=1)

        return self.response

    def naruci(self, stavka):
        """Šalje konačnu narudžbu."""
        self.response = None
        narudzba = {"stavka": stavka, "kolicina": 1}
        self.channel.basic_publish(
            exchange='',
            routing_key='narudzbe',
            properties=pika.BasicProperties(
                reply_to=self.callback_queue,
                correlation_id=self.corr_id,
            ),
            body=json.dumps(narudzba)
        )
        time.sleep(2)
        if random.random() > 0.8:
            print(f"\n\nKupac {str(self.corr_id)[:4]}: 'Jao nemam novaca!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'\n{time.time_ns()}\n")
            raise ValueError
        print(f"Kupac {str(self.corr_id)[:4]}: 'Može jedna {stavka}, hvala!'\n")

        while self.response is None:
            self.connection.process_data_events(time_limit=1)





if __name__ == "__main__":
    kupac = Kupac()
    ponuda = kupac.pitaj_za_ponudu()

    if ponuda:
        odabir = random.choice(ponuda)
        kupac.naruci(odabir)
        print(f"Kupac {str(kupac.corr_id)[:4]}: Hvala. Dovidenja!")
        time.sleep(10)
        kupac.connection.close()


