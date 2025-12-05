// TRANSMITTER FUNCTION

#include <Arduino.h>
#include <RH_RF95.h>
#include "esp_camera.h"
#include "base64.h"
#include <SD.h>

// ANCHOR Pin definitions
// NOTE Led should be grounded.
#define LED_PIN 16

// TODO update these
#define RF_DIO0 2
#define RF_CS 15
#define RF_RST 4

#define SPI_SCK 14
#define SPI_MISO 12
#define SPI_MOSI 13

#define SD_CS 16

// NOTE Here are the pins
// MOSI => D11
// MISO => D12
// SCK => D13

// AI Thinker pin configuration:
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// ANCHOR Program settings definitions
#define RF_FREQ 903.0
#define RF_BAND 125000
#define RF_SPREAD 12
#define RF_CODING 5
#define RF_TX_PWR 7

#define DATALOG_NAME "/DATALOG.txt"

// ANCHOR Function preludes
// Sets up pins
void setup_pins();

// Run when setup fails. Blinks LED, bricks program.
void setup_failure();

// Setup the RF
void setup_rf();

// Setup the camera
void setup_camera();

// Setup the SD
void setup_sd();

// Send a message through RF
void send_msg(const char *msg);

// Log data to SD (expensive)
void log_sd(const char *log);

// ANCHOR Global vars
RH_RF95 rf95(RF_CS, RF_DIO0);

uint32_t picture_counter = 0;

void setup()
{
  Serial.begin(9600);
  delay(500);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  Serial.println("Setting up pins...");
  setup_pins();
  setup_sd();
  // setup_rf();
  setup_camera();
  delay(500);
  log_sd("[INFO] Setup finished.");
}

void loop()
{
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    return;
  }

  String encoded = base64::encode(fb->buf, fb->len);

  esp_camera_fb_return(fb);

  // send_msg(encoded.c_str());
  // rf95.waitPacketSent();

  delay(5000);
}

void setup_pins()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(RF_RST, OUTPUT);
  digitalWrite(RF_RST, HIGH);
  delay(100);
  digitalWrite(RF_RST, LOW);
  delay(100);
  digitalWrite(RF_RST, HIGH);
  delay(100);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
}

void setup_failure()
{
  unsigned long last_triggered = 0;
  bool led_state = LOW;
  while (true)
  {
    if (millis() - last_triggered > 300)
    {
      led_state = !led_state;
      digitalWrite(LED_PIN, led_state);
      last_triggered = millis();
    }
  }
  exit(EXIT_FAILURE);
}

void setup_rf()
{

  if (!rf95.init())
  {
    Serial.println("[E] RF init failed.");
    setup_failure();
  }

  rf95.setFrequency(RF_FREQ);
  rf95.setSignalBandwidth(RF_BAND);
  rf95.setSpreadingFactor(RF_SPREAD);
  rf95.setCodingRate4(RF_CODING);
  rf95.setTxPower(RF_TX_PWR, false);
  rf95.setPayloadCRC(false);

  Serial.println("RF inited at the following settings:");
  Serial.print(RF_FREQ);
  Serial.println("Hz Frequency");
  Serial.print(RF_BAND);
  Serial.println("Hz Bandwidth");
  Serial.print(RF_SPREAD);
  Serial.println(" Spreading Factor");
  Serial.print(RF_CODING);
  Serial.println(" Coding Rate (4)");
  Serial.print(RF_TX_PWR);
  Serial.println("dBm power");
}

void setup_camera()
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK)
  {
    Serial.println("[E] Camera init failed");
    setup_failure();
  }
}

void setup_sd()
{

  if (!SD.begin(SD_CS))
  {
    Serial.println("[E] SD init failed.");
    setup_failure();
  }

  if (SD.exists(DATALOG_NAME))
    SD.remove(DATALOG_NAME);

  if (SD.exists("/pic"))
  {
    SD.remove("/pic");
    SD.mkdir("/pic");
  }

  File datalog = SD.open("/DATALOG.txt", FILE_WRITE);
  if (datalog)
  {
    datalog.println("[INFO] DATALOG INITIALIZED");
    datalog.close();
  }
  else
  {
    Serial.println("[E] Failed to open datalog");
    setup_failure();
  }
}

void send_msg(const char *msg)
{
  char buf[RH_RF95_MAX_MESSAGE_LEN];

  const uint8_t len = sprintf(buf, msg) + 1;

  Serial.print("S[");
  Serial.print(len);
  Serial.print("]<");
  Serial.print(buf);
  Serial.println(">");

  rf95.send((uint8_t *)buf, len);
}

void log_sd(const char *log)
{
  for (uint8_t attempts = 0; attempts < 10; attempts++)
  {
    File datalog = SD.open("/DATALOG.txt", FILE_APPEND);
    if (datalog)
    {
      datalog.print("[");
      datalog.print(millis());
      datalog.print("] ");
      datalog.println(log);
      break;
    }
    datalog.close();
    delay(100);
  }
}

void log_picture(camera_fb_t *fb)
{
  char filename[CONFIG_FATFS_MAX_LFN];
  sprintf(filename, "/pic/%d.jpg", picture_counter++);
  File picture = SD.open(filename, FILE_WRITE);
  if (!picture)
  {
    Serial.println("[E] Failed to open image.");
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    return;
  }
  else
  {
    picture.write(fb->buf, fb->len);
  }
  picture.close();
  delay(100);
}