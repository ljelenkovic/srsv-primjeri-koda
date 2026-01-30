FROM python

WORKDIR /app
COPY sender.py .

RUN python3 -m pip install pika --upgrade

CMD ["python3", "-u", "sender.py"]
