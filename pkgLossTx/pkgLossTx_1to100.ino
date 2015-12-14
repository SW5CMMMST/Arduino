#include <Timer.h>
#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile
#define STAT_LED 13
#define SEND_LED 2
#define TEST_LEN 100
#define TEST_INTERVAL 300

Timer t;
RH_ASK driver;
int pid = 0;
uint32_t cnt = 0;

void setup() {
    pinMode(STAT_LED, OUTPUT);
    pinMode(SEND_LED, OUTPUT);
    Serial.begin(9600);	  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
    Serial.println("Ready!");
    digitalWrite(STAT_LED, HIGH);
    pid = t.every(TEST_INTERVAL, sendNext);
}

void loop() {
    t.update();
    if(cnt >= TEST_LEN){
      t.stop(pid);
      digitalWrite(13, LOW);
    }
}

void sendNext() {
  t.pulse(SEND_LED, 150, LOW);
  uint8_t msg[4];
  *(uint32_t*)msg = cnt++;
  driver.send(msg, sizeof(msg));
  driver.waitPacketSent();
  #ifdef VERBOSE
  Serial.print("Pkg # ");
  Serial.println(*(uint32_t*)msg);
  #endif
}
