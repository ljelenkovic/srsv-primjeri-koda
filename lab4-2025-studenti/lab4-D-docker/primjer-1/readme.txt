LAB4-D. Primjer stvaranja i ručnog pokretanja spremnika kroz Docker u Ubuntu na WSL-u

//1. Stvori Docker slike s programima koji izmjenjuju poruke
docker build -f Dockerfile-send -t sender-image .
docker build -f Dockerfile-recv -t receiver-image .
docker build -f Dockerfile-inter -t inter-image .
//2. Stvori mrežu kroz koju će spremnici komunicirati
docker network create rabbit-net
//3. Pokreni postojeći Rabbitmq spremnik
docker run -it --rm --name rabbitmq --network rabbit-net -p 5672:5672 -p 15672:15672 rabbitmq:3.13-management
//4. Pokreni spremnike s programima koji izmjenjuju poruke
docker run -it --rm --name python-receiver --network rabbit-net receiver-image recv.py
docker run -it --rm --name python-sender --network rabbit-net sender-image send.py
docker run -d --name python-inter --network rabbit-net --restart on-failure inter-image inter.py
