#include <Arduino.h>

#include <SX127x.h>
#include <SPI.h>
#include "esp_camera.h"
#define CAMERA_MODEL_XIAO_ESP32S3
#include "camera_pins.h"
#include "ocradlib.h"

const int ID = 1;
volatile bool state = false;

/*
NSS D0
MISO D1
MOSI D3
SCK D2
DIO0 D10
*/

// =====================

SX127x LoRa;

void initLora();
void sendLora(int id, bool state);

void CameraTask(void *);

// =====================

// =====================

void setup()
{
  Serial.begin(9600);
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  initLora();

  xTaskCreate(CameraTask, "CameraTask", 4096, NULL, 5, NULL);
}

void loop()
{
  sendLora(ID, state);
  vTaskDelay(5000 / portTICK_PERIOD_MS);
}

void initLora()
{

  SPI.begin(D2, D1, D3, D0);
  LoRa.setSPI(SPI, 16000000);

  Serial.println("Begin LoRa radio");

  if (!LoRa.begin(D0, -1, D10, -1, -1))
  {
    Serial.println("Something wrong, can't begin LoRa radio");
    while (1)
      ;
  }

  Serial.println("Set frequency to 868 Mhz");
  LoRa.setFrequency(868E6);

  Serial.println("Set TX power to +17 dBm");
  LoRa.setTxPower(17, SX127X_TX_POWER_PA_BOOST);

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

  Serial.println("\n-- LORA TRANSMITTER --\n");
}

void sendLora(int id, bool state)
{
  Serial.println("Transmitting...");

  char message[128];
  sprintf(message, "RBLK?id=%d&state=%d\0", id, state);

  int nBytes = strlen(message);

  LoRa.beginPacket();
  LoRa.write(message, nBytes);
  LoRa.endPacket();

  Serial.write(message, nBytes);
  Serial.println();

  LoRa.wait();

  Serial.print("Transmit time: ");
  Serial.print(LoRa.transmitTime());
  Serial.println(" ms");
  Serial.println();
}

void CameraTask(void *)
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  // config.jpeg_quality = 12;
  config.fb_count = 1;

  // config.fb_location = CAMERA_FB_IN_DRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    for (;;)
    {
      Serial.printf("Camera init failed with error 0x%x", err);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s == nullptr)
  {
    Serial.println("Failed to get sensor settings");
    for (;;)
      vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  s->set_exposure_ctrl(s, 1); // 0 = Disable auto-exposure, 1 = Enable auto-exposure
  s->set_aec2(s, 1);          // Enable advanced auto-exposure (optional)
  s->set_ae_level(s, 0);      // Set AE level (-2 to 2, where 0 is default)

  while (true)
  {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      continue;
    }

    bool car = false;
    for (int threshold = 50; threshold < 160; threshold += 20)
    {
      // Serial.printf("Threshold: %d\n", threshold);

      OCRAD_Descriptor *ocrdes = OCRAD_open();
      if (ocrdes == nullptr)
      {
        Serial.println("Failed to open OCRAD descriptor");
        continue;
      }

      OCRAD_Pixmap pixmap;
      pixmap.data = fb->buf;
      pixmap.height = fb->height;
      pixmap.width = fb->width;
      pixmap.mode = OCRAD_greymap;

      OCRAD_set_image(ocrdes, &pixmap, false);
      OCRAD_set_threshold(ocrdes, threshold);
      OCRAD_recognize(ocrdes, false);

      const int blocks = OCRAD_result_blocks(ocrdes);

      for (int b = 0; b < blocks; ++b)
      {
        const int lines = OCRAD_result_lines(ocrdes, b);
        for (int l = 0; l < lines; ++l)
        {
          int cnt = 0;
          const char *s = OCRAD_result_line(ocrdes, b, l);
          if (s && s[0])
          {
            for (int i = 0; s[i]; ++i)
            {
              if (isalnum(s[i]))
              {
                ++cnt;
              }
            }
          }

          // Serial.printf("Block %d, Line %d: %s\n", b, l, s);

          if (cnt >= 2)
          {
            car = true;
            break;
          }
        }

        if (car)
        {
          break;
        }
      }

      OCRAD_close(ocrdes);

      if (car)
      {
        break;
      }
    }

    Serial.printf("Car: %s\n", car ? "Yes" : "No");
    state = !car;

    esp_camera_fb_return(fb);
  }
}
