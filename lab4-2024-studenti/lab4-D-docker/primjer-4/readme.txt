Ručno pokretanje containera:

    Pokretanje rabbitmq containera:
        $ docker run -it --rm --name rabbitmq -p 5672:5672 -p 15672:15672 rabbitmq:3.13-management

    Pokretanje receiver i sender containera:
        $ docker build -t python-client .
        $ docker run --net="host" --name receiver --rm -v $(pwd):/app -w /app -i -t python-client receiver.py
        $ docker run --net="host" --name sender --rm -v $(pwd):/app -w /app -i -t python-client sender.py

Pokretanje uz pomoć "docker compose":
    $ docker compose up

Zaustavljanje nakon pokretanja "docker compose":
    (CTRL + C)

    $ docker compose down