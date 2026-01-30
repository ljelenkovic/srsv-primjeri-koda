FROM python
RUN python3 -m pip install pika --upgrade

WORKDIR /app
COPY . .

CMD ["blagajna.py"]
ENTRYPOINT ["python3"]
