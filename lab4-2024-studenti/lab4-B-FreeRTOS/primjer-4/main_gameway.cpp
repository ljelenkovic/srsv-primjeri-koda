#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SX127x.h>
#include <WiFi.h>
#include <HTTPClient.h>

const int ID = 1;

// =====================

/*
D1 SCK
D8 MISO
D10 MOSI
D2 NSS
D3 DIO0
*/

SX127x LoRa;

SemaphoreHandle_t last_message_mutex = xSemaphoreCreateMutex();
size_t recv_time = 0;
char last_message[128] = {0};
void LoraListenTask(void *pvParameters);

// =====================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void DisplayInfoTask(void *pvParameters);

// =====================

struct WifiLogins
{
  const char *ssid;
  const char *password;
};

WifiLogins wifiLogins[] = {};//obrisano

volatile static int currentWifi = -1;
const int wifiCount = sizeof(wifiLogins) / sizeof(wifiLogins[0]);

void WiFiReconnectTask(void *pvParameters);

// =====================

void setup()
{
  Serial.begin(9600);
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  xTaskCreate(LoraListenTask, "LoraListenTask", 16 * 4096, NULL, 1, NULL);
  xTaskCreate(WiFiReconnectTask, "WiFiReconnectTask", 4096, NULL, 1, NULL);
  xTaskCreate(DisplayInfoTask, "DisplayInfoTask", 4096, (void *)"Hello, world!", 1, NULL);
}

void loop()
{
}

void LoraListenTask(void *pvParameters)
{
  SPI.begin(D1, D8, D10, D2);
  LoRa.setSPI(SPI);

  if (!LoRa.begin(D2, -1, D3, -1, -1))
  {
    Serial.println("Something wrong, can't begin LoRa radio");
    while (1)
      ;
  }

  Serial.println("Set frequency to 915 Mhz");
  LoRa.setFrequency(868E6);

  Serial.println("Set RX gain to power saving gain");
  LoRa.setRxGain(SX127X_RX_GAIN_POWER_SAVING, SX127X_RX_GAIN_AUTO);

  Serial.println("Set modulation parameters:\n\tSpreading factor = 7\n\tBandwidth = 125 kHz\n\tCoding rate = 4/5");
  LoRa.setSpreadingFactor(7);
  LoRa.setBandwidth(125000);
  LoRa.setCodeRate(5);

  Serial.println("Set packet parameters:\n\tExplicit header type\n\tPreamble length = 12\n\tPayload Length = 15\n\tCRC on");
  LoRa.setHeaderType(SX127X_HEADER_EXPLICIT);
  LoRa.setPreambleLength(12);
  LoRa.setPayloadLength(15);
  LoRa.setCrcEnable(true);

  Serial.println("Set syncronize word to 0x71");
  LoRa.setSyncWord(0x71);

  Serial.println("\n-- LORA RECEIVER --\n");

  while (1)
  {
    LoRa.request();

    LoRa.wait();

    const uint8_t msgLen = LoRa.available();

    if (msgLen >= 128)
    {
      Serial.println("Message too long");
      LoRa.purge();
      continue;
    }

    char message[msgLen] = {0};
    LoRa.read(message, msgLen);

    uint8_t counter = LoRa.read();

    Serial.write(message, msgLen);
    Serial.print("  ");
    Serial.println(counter);

    Serial.print("RSSI: ");
    Serial.print(LoRa.packetRssi());
    Serial.print(" dBm | SNR: ");
    Serial.print(LoRa.snr());
    Serial.println(" dB");

    uint8_t status = LoRa.status();
    if (status == SX127X_STATUS_CRC_ERR)
    {
      Serial.println("CRC error");
      continue;
    }
    else if (status == SX127X_STATUS_HEADER_ERR)
    {
      Serial.println("Packet header error");
      continue;
    }
    Serial.println();

    int id, state;
    int mch = sscanf(message, "RBLK?id=%d&state=%d", &id, &state);

    // test if packet begins with RBLK
    if (mch == 2)
    {

      if (currentWifi == -1 || WiFi.status() != WL_CONNECTED)
      {
        Serial.println("No WiFi connection");
        continue;
      }

      HTTPClient http;

      char url[100];
      snprintf(url, 99, "http://obrisano:8000/srsv?id=%d&state=%d\0", id, state);

      Serial.print("Sending GET request to ");
      Serial.println(url);

      http.begin(url);
      int httpCode = http.GET();

      if (httpCode > 0)
      {
        Serial.printf("HTTP GET request code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK)
        {
          String payload = http.getString();
          Serial.println(payload);
        }

        xSemaphoreTake(last_message_mutex, portMAX_DELAY);
        sniprintf(last_message, 127, "id=%d s=%d", id, state);
        xSemaphoreGive(last_message_mutex);
        recv_time = millis();
      }
      else
      {
        Serial.printf("HTTP GET request failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
    }
    else
    {
      Serial.println("Invalid message");
    }
  }
}

void WiFiReconnectTask(void *pvParameters)
{
  while (1)
  {
    for (int i = 0; i < wifiCount; i++)
    {
      WiFi.begin(wifiLogins[i].ssid, wifiLogins[i].password);

      Serial.print("Connecting to ");
      Serial.print(wifiLogins[i].ssid);
      Serial.print(" with password ");
      Serial.println(wifiLogins[i].password);

      // try for 5 seconds
      for (int j = 0; j < 50; j++)
      {
        if (WiFi.status() == WL_CONNECTED)
        {
          Serial.println("Connected");
          currentWifi = i;
          break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }

      if (WiFi.status() == WL_CONNECTED)
        break;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("No WiFi connection");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      currentWifi = -1;
      continue;
    }

    while (WiFi.status() == WL_CONNECTED)
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    currentWifi = -1;
  }
}

void DisplayInfoTask(void *pvParameters)
{
  const char *info = (const char *)pvParameters;
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  while (1)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    // wifi status
    display.print("WiFi: ");
    if (currentWifi == -1)
    {
      display.println("Disconnected");
    }
    else
    {
      display.println(wifiLogins[currentWifi].ssid);
    }

    display.print("LoRa last: ");

    xSemaphoreTake(last_message_mutex, portMAX_DELAY);
    display.println(last_message);
    xSemaphoreGive(last_message_mutex);
    display.print("LoRa ago (s): ");
    display.println((millis() - recv_time) / 1000);

    display.display();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
