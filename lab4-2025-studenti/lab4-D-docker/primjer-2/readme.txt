ARHITEKTURA
-----------
1. Producer - generira periodičke evente (3 tipa: 1s, 5s, 10s)
2. Processor - obrađuje evente, uzrokuje crash nakon N evenata (default: N=20)
3. Stats - prati statistiku i mjeri vrijeme oporavka

Komunikacija: Producer → RabbitMQ → Processor → RabbitMQ → Stats

STRUKTURA KODA
--------------
common/common.h           - zajedničke utility funkcije (vrijeme, logging, strukture)
common/rabbitmq_client.h  - wrapper za RabbitMQ C API
producer/producer.cpp     - generira evente u 3 threada
processor/processor.cpp   - konzumira evente, simulira rad, uzrokuje crash nakon N evenata
stats/stats.cpp           - agregira statistiku, računa vrijeme oporavka
compose.yaml              - Docker Compose konfiguracija (servisi, restart policy)

POKRETANJE
----------
1. Docker provjera:
   docker ps

2. Build i pokretanje:
   docker compose up --build

3. Zaustavljanje:
   Ctrl+C ili docker compose down

TESTIRANJE
----------
- Sustav će automatski crashati processor nakon 20 eventa
- Docker će ga automatski restartati
- Stats servis će logirati vrijeme oporavka

Crash test:
   docker kill --signal=SIGKILL lab4_processor

RabbitMQ Management UI:
   http://localhost:15672
   Username: lab4
   Password: lab4pass

KONFIGURACIJA
-------------
compose.yaml:
  CRASH_AFTER_N_EVENTS: "20"

REZULTATI
---------
Pri zaustavljanju (Ctrl+C), stats servis ispisuje:
- Ukupno obrađenih eventa
- Prosječno/min/max vrijeme reakcije
- Broj crasheva
- Prosječno/min/max vrijeme oporavka

