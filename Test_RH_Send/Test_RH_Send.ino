#include <RH_ASK.h>
#include <SPI.h>
#define LED_PIN 13

RH_ASK driver;

void setup() {
  Serial.begin(9600);

  if (!driver.init())
         Serial.println("init failed");
  pinMode(led_pin, OUTPUT);
}

void loop(){
  for(int i = 0; i < 100; i++) {
    digitalWrite(LED_PIN, HIGH);
    char * buf = "abcdefghijklmno";
    driver.send((uint8_t *)buf, strlen(buf));
    driver.waitPacketSent();
    digitalWrite(LED_PIN, LOW);
    delay(10);
  }

  // To avoid sending more packets than we want to, halts the Arduino
  while(true);
}

