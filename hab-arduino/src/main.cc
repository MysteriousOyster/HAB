#include "Arduino.h"
#include "RH_RF95.h"
#include "SD.h"
#include "SoftwareSerial.h"

// NOTE This shalt be defined by production's time.
// #define PRODUCTION

#ifdef PRODUCTION
#define POWER 23
#else
#define POWER 5
#endif

#define FILE_NAME "PICKLES-DATALOG.TXT"
#define LED_PIN 9
#define BLINK_INTERVAL 300
// Reed switch should be GND---REED SWITCH---REED_PIN
#define REED_PIN 3

// TODO MAKE THIS FLIPPING THING WORK!
#define RF_CHIP_SELECT 1
#define RF_RESET 1
#define RF_INTTERUPT 1
#define RF_FREQUENCY 915.0

// TODO MAKE THIS FLIPPING THING WORK!
#define U_TX 1
#define U_RX 1
#define IMAGE_START_MARKER 0xAA // Start of a new image transmission
#define IMAGE_END_MARKER   0x55 // End of the image data

const int SD_CS_PIN = 10;

File datalog;

void blink_led();
void setup_failure();

RH_RF95 rf(RF_CHIP_SELECT, RF_INTTERUPT);

SoftwareSerial cam(U_RX, U_TX);

void setup() {
    Serial.begin(9600);
    cam.begin(9600);
    pinMode(LED_PIN, OUTPUT);
    pinMode(REED_PIN, INPUT_PULLUP);
    pinMode(RF_RESET, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    delay(1000);

    if(!SD.begin(SD_CS_PIN)) {
        Serial.println("SD init failed!");
        setup_failure();
    }

    SD.remove(FILE_NAME);

    datalog = SD.open(FILE_NAME);

    if(!datalog) {
        Serial.println("SD open failed!");
        setup_failure();
    }

    digitalWrite(RF_RESET, HIGH);
    delay(10);
    digitalWrite(RF_RESET, LOW);
    delay(10);
    digitalWrite(RF_RESET, HIGH);

    if(!rf.init()) {
        Serial.println("RF init failed!");
        setup_failure();
    }

    if(!rf.setFrequency(RF_FREQUENCY)) {
        Serial.println("RF freqset failed!");
        setup_failure();
    }

    rf.setTxPower(POWER, false);
}

void loop() {
    while(cam.available()) {
        const char c = cam.read();
        Serial.print(c);
    }
}

void blink_led() {
    static unsigned long target_millis;

    if(millis() >= target_millis) {
        digitalWrite(LED_PIN, digitalRead(LED_PIN));
        target_millis = millis() + BLINK_INTERVAL;
    } else {
        target_millis = millis() + BLINK_INTERVAL;
    }
}

void setup_failure() {
    digitalWrite(LED_PIN, LOW);
    while(true) {
        blink_led();
    }
}