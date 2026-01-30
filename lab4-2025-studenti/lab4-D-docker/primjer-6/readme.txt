Simulacija složenijeg, fizički i funkcionalno raspoređenog sustava pomoću Docker spremnika.

Sustav se sastoji od 3 funkcionalne cjeline (programa) koje zajedno simuliraju rad foruma.
- member.py sadrži kod pojedinog člana foruma koji šalje poruke
- censor.py sadrži kod središnjeg moderatora koji cenzurira nadolazeće poruke ovisno o sadržaju
- archiver.py sadrži kod arhivera koji pohranjuje cenzurirane poruke (u stvarnosti ih ispisuje u terminal)

Docker okruženje definirano je pomoću compose.yaml datoteke.

Svaka funkcionalna cjelina izvršava se u posebnom spremniku.
Spremnici komuniciriaju neizravno pomoću RabbitMQ pouzdane usluge za razmjenu poruka.
Mogu postojati više spremnika različitih članova foruma.
Kod članova foruma u simulaciji nasumično se prekida nakon određenog vremena (simulacija odspajanja / prekida sesije).
Kod moderatora u simulaciji nasumično se prekida nakon određenog vremena (simulacija greške),
u svrhu provjere ponovnog pokretanja spremnika.

Upute za postavljanje okoline i pokretanje:
($ se ne piše, prethodno je potrebno instalirati Docker)

- Priprema slike za Python spremnik (-t za ime):
$ docker build -t python-client .

- Pokretanje (-d za rad u pozadini, spremnici nisu vezani uz terminale):
$ docker compose up -d

- Ispisivanje tijeka rada sustava (-f za kontinuirano praćenje ispisa):
$ docker compose logs -f member censor archiver

- Spojena naredba (&&) za zaustavljanje, pokretanje i praćenje s 3 različita člana foruma (--scale member=3):
$ docker compose down && docker compose up -d --scale member=3 && docker compose logs -f member censor archiver

- Zaustavljanje izvedbe:
$ docker compose down