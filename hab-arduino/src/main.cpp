// RECEIVER FUNCTION

#include <Arduino.h>
#include <RH_RF95.h>

// ANCHOR Pin definitions
// NOTE Led should be grounded.
#define LED_PIN 3

// TODO update these
#define RF_DIO0 2
#define RF_CS 7
#define RF_RST 8

// NOTE Here are the pins
// MOSI => D11
// MISO => D12
// SCK => D13

// ANCHOR Program settings definitions
#define RF_FREQ 914.0
#define RF_BAND 125000
#define RF_SPREAD 12
#define RF_CODING 5
#define RF_TX_PWR 7

// ANCHOR Function preludes
// Sets up pins
void setup_pins();

// Run when setup fails. Blinks LED, bricks program.
void setup_failure();

// Setup the RF
void setup_rf();

// Send a message through RF
void send_msg(const char *msg);

// ANCHOR Global vars
RH_RF95 rf95(RF_CS, RF_DIO0);

void setup()
{
  Serial.begin(9600);
  while(!Serial) {}
  delay(1000);

  SPI.begin();
  Serial.println("Setting up pins...");
  setup_pins();
  setup_rf();
}

void loop()
{
  Serial.println("Waiting for input.");
  while (!Serial.available())
    delay(10);
  Serial.read();

  Serial.println("Ok, sending.");

  send_msg("REQ_PIC");
  rf95.waitPacketSent();

  if (rf95.waitAvailableTimeout(3000))
  {
    char buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv((uint8_t *)buf, &len))
    {
      Serial.print("R[");
      Serial.println(len);
      Serial.print("]<");
      Serial.print(buf);
      Serial.print("> RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
    }
  }
  else
  {
    Serial.println("No response received!");
  }
}

void setup_pins()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  pinMode(RF_RST, OUTPUT);
  digitalWrite(RF_RST, HIGH);
  delay(100);
  digitalWrite(RF_RST, LOW);
  delay(100);
  digitalWrite(RF_RST, HIGH);
  delay(100);
}

void setup_failure()
{
  unsigned long last_triggered = 0;
  bool led_state = HIGH;
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