#include <RH_ASK.h>
#include <SPI.h> 

RH_ASK driver;

void setup()
{
  Serial.begin(9600);
  if (!driver.init())
    Serial.println("init failed");
}

uint8_t len = 1;
const char *msg = "0123456789abcdefghijklmnopqrstuvwzyzABCDEFGHINJKLMNOPQRSTUVWXYZ";

void loop()
{
  driver.send((uint8_t *)msg, len);
  driver.waitPacketSent();
  len = (len % 60) + 1;
  delay(100);
}
