Zadatak predočava upravljanje spremnicima ostvareno kroz program Docker.
S pomoću datoteke docker-compose.yaml, koriste se dockerfile datoteke čime se grade i pokreću slike triju aplikacija, svake u zasebnom spremniku.
Pokreću se tri vrste aplikacije, gost, konobar i kuhar. Aplikacije razgovaraju kanalima napravljenim s RabbitMQ, odnosno pika u Pythonu.
U aplikaciji gost pojavljuje se zahtjev koji se šalje konobaru, konobar ga prosljeđuje kuharu. Obrada zahtjeva u kuharu traje 3 sekunde nakon čega se odgovor vraća gostu obrnutim redoslijedom.
Tijekom prijenosa poruke, u konobaru ili kuharu može doći do šuma što uzrokuje grešku i prethodna usluga se ponovno pokreće. Do ponovnog pokretanja dolazi i ako u kanalu dođe do blokade i detektira se predugo čekanje.
U ispisu je vidljiv tijek razmijene poruka kao i vremena pokretanja te gašenja programa.
Spremnici se grade i pokreću sljedećom naredbom: docker compose -f .\docker-compose.yaml up --build.