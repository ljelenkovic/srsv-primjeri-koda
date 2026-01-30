FROM python
WORKDIR /app
COPY receiver.py .

RUN python3 -m pip install pika --upgrade

CMD ["python3", "-u" ,"receiver.py"]
