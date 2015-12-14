#include <RH_ASK.h>
#include <Timer.h>
#include <SPI.h> // Not actualy used but needed to compile
#define VERBOSE
#define RECV_LED 2
#define LOSS_LED 3
#define STAT_LED 13
#define TEST_LEN 100
#define TEST_INTERVAL 201 // 10 millis extra per interval, just to be sure
#define ANT_LEN 34

RH_ASK driver;
Timer t;

void setup() {
    pinMode(STAT_LED, OUTPUT);
    pinMode(RECV_LED, OUTPUT);
    pinMode(LOSS_LED, OUTPUT);
    
    Serial.begin(9600);	// Debugging only
    if (!driver.init())
         Serial.println("init failed");
    digitalWrite(STAT_LED, HIGH);
}

uint32_t cnt = 0;
uint32_t lastcnt = 0;
uint8_t missed = 0;
uint32_t starttime = 0;

void loop() {
    t.update();
    uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
    uint8_t buflen = sizeof(buf);

    if (driver.recv(buf, &buflen)) {
      t.pulse(RECV_LED, 150, LOW);
      cnt =  *(uint32_t*)buf;
      Serial.print("Received msg #");
      Serial.println(cnt);  
    }
}
