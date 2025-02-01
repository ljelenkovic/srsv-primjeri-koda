#include <Arduino.h>

#include <SX127x.h>
#include <SPI.h>
#include "Adafruit_VL53L0X.h"

const int ID = 2;
volatile bool metal = false, distance = false;
const int metal_threshold = 10;

QueueHandle_t log_queue = xQueueCreate(30, sizeof(char[256]));

/*
SCK D8
MISO D9
MOSI D10
NSS D0
DIO0 D2
*/

// =====================

SX127x LoRa;

void initLora();
void sendLora(int id, bool state);

// int getMetalMeasurement();
// void MetalTask(void *);
void DistanceTask(void *);
void LoraTask(void *);
void LoggerTask(void *);

// =====================

Adafruit_VL53L0X lox;

// =====================

// metal detektor ne koristimo

// const int nmeas = 1024;
// const int npulse = 10;

// const byte pin_pulse = D7;
// const byte pin_cap = A3;

// =====================

void setup()
{
  Serial.begin(9600);

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // xTaskCreate(MetalTask, "MetalTask", 4096, NULL, 5, NULL);
  xTaskCreate(DistanceTask, "DistanceTask", 4096, NULL, 5, NULL);
  xTaskCreate(LoraTask, "LoraTask", 4096, NULL, 4, NULL);
  xTaskCreate(LoggerTask, "LoggerTask", 4096, NULL, 1, NULL);

  Serial.println("Setup done");
}

void loop()
{
}

void LoggerTask(void *)
{
  char message[256];
  while (true)
  {
    if (xQueueReceive(log_queue, message, 10))
    {
      Serial.println(message);
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void LoraTask(void *)
{
  initLora();

  while (true)
  {
    sendLora(ID, metal || distance);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void initLora()
{
  SPI.begin(D8, D9, D10);
  LoRa.setSPI(SPI, 16000000);

  xQueueSendToBack(log_queue, "Begin LoRa radio", 10);

  if (!LoRa.begin(D0, -1, D2))
  {
    xQueueSendToBack(log_queue, "Something wrong, can't begin LoRa radio", 10);
    while (1)
      ;
  }

  xQueueSendToBack(log_queue, "Set frequency to 868 Mhz", 10);
  LoRa.setFrequency(868E6);

  xQueueSendToBack(log_queue, "Set TX power to +17 dBm", 10);
  LoRa.setTxPower(17, SX127X_TX_POWER_PA_BOOST);

  xQueueSendToBack(log_queue, "Set modulation parameters:\n\tSpreading factor = 7\n\tBandwidth = 125 kHz\n\tCoding rate = 4/5", 10);
  LoRa.setSpreadingFactor(7);
  LoRa.setBandwidth(125000);
  LoRa.setCodeRate(5);

  xQueueSendToBack(log_queue, "Set packet parameters:\n\tExplicit header type\n\tPreamble length = 12\n\tPayload Length = 15\n\tCRC on", 10);
  LoRa.setHeaderType(SX127X_HEADER_EXPLICIT);
  LoRa.setPreambleLength(12);
  LoRa.setPayloadLength(15);
  LoRa.setCrcEnable(true);

  xQueueSendToBack(log_queue, "Set syncronize word to 0x71", 10);
  LoRa.setSyncWord(0x71);

  xQueueSendToBack(log_queue, "\n-- LORA TRANSMITTER --\n", 10);
}

void sendLora(int id, bool state)
{
  xQueueSendToBack(log_queue, "Transmitting...", 10);

  char message[256];
  sprintf(message, "RBLK?id=%d&state=%d\0", id, state);

  int nBytes = strlen(message);

  LoRa.beginPacket();
  LoRa.write(message, nBytes);
  LoRa.endPacket();

  xQueueSendToBack(log_queue, message, 10);

  LoRa.wait();
  xQueueSendToBack(log_queue, "Transmission done", 10);
}

// void MetalTask(void *)
// {
// pinMode(pin_pulse, OUTPUT);
// digitalWrite(pin_pulse, LOW);
// pinMode(pin_cap, INPUT);
//   vTaskDelay(1000 / portTICK_PERIOD_MS);
//   int baseline = getMetalMeasurement();
//   vTaskDelay(100 / portTICK_PERIOD_MS);
//   baseline = getMetalMeasurement();
//   vTaskDelay(100 / portTICK_PERIOD_MS);
//   baseline = getMetalMeasurement();
//   vTaskDelay(100 / portTICK_PERIOD_MS);
//   baseline = getMetalMeasurement();
//   vTaskDelay(100 / portTICK_PERIOD_MS);
//   baseline = getMetalMeasurement();

//   while (true)
//   {
//     int measurement = getMetalMeasurement();
//     metal = abs(measurement - baseline) < metal_threshold;

//     Serial.printf("Metal: %d, baseline: %d, status %d\n", measurement, baseline, metal);

//     vTaskDelay(1000 / portTICK_PERIOD_MS);
//   }
// }

void DistanceTask(void *)
{
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  Wire.begin();
  Wire.setPins(D4, D5);
  Wire.begin();

  if (!lox.begin())
  {
    while (1)
    {
      xQueueSendToBack(log_queue, F("Failed to boot VL53L0X"), 10);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }

  lox.startRangeContinuous();

  while (true)
  {
    if (lox.isRangeComplete())
    {
      int mm_distance = lox.readRange();
      distance = mm_distance >= 300;

      char message[256];
      sprintf(message, "Distance: %d mm, status %d", mm_distance, distance);
      xQueueSendToBack(log_queue, message, 10);
    }
    else
    {
      xQueueSendToBack(log_queue, "Distance not ready", 10);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// int getMetalMeasurement()
// {
//   long long sum = 0;
//   for (int imeas = 0; imeas < nmeas + 2; imeas++)
//   {
//     pinMode(pin_cap, OUTPUT);
//     digitalWrite(pin_cap, LOW);
//     delayMicroseconds(20);
//     pinMode(pin_cap, INPUT);

//     for (int ipulse = 0; ipulse < npulse; ipulse++)
//     {
//       digitalWrite(pin_pulse, HIGH);
//       delayMicroseconds(10);
//       digitalWrite(pin_pulse, LOW);
//       delayMicroseconds(10);
//     }

//     int val = analogRead(pin_cap);
//     sum += val;
//   }

//   return sum >> 11;
// }
