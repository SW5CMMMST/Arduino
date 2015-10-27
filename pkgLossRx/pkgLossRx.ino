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
      #ifdef VERBOSE
      Serial.print("Received msg #");
      Serial.println(cnt);
      #endif
      if(cnt == lastcnt + 1) {
        lastcnt = cnt;
      } else if(cnt < lastcnt || cnt == 0) {
        lastcnt = 0;
        missed = 0;
        starttime = millis();
        #ifdef VERBOSE
        Serial.println("Transmission reset");
        #endif
      } else {
        t.pulse(LOSS_LED, 500, LOW);
        uint8_t curmis = missed;
        missed += cnt - lastcnt - 1;
        curmis = missed - curmis;
        lastcnt = cnt;
        #ifdef VERBOSE
        Serial.print("Missed ");
        Serial.print(curmis);
        Serial.print(" pkgs - ");
        Serial.println(missed);
        #endif
      }
      #ifdef TEST_LEN
      if(cnt + 1 == TEST_LEN || millis() > (starttime + TEST_INTERVAL * TEST_LEN)){
        Serial.print(ANT_LEN);
        Serial.print(" \t ");
        Serial.print(missed);
        Serial.print(" \t ");
        Serial.println(TEST_LEN);
        digitalWrite(STAT_LED, LOW);
        while(1){
          if (driver.recv(buf, &buflen))
            if(*(uint32_t*)buf < TEST_LEN){
              //t.oscillate(STAT_LED, 100, HIGH, 5);
              lastcnt = 0;
              missed = 0;
              starttime = millis();
              break; 
            }
        } //Hack used to stop execution on arduino until transmitter resets
      }
      #endif     
    }
}
