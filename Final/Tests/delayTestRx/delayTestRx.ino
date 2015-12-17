#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile

RH_ASK driver;

unsigned long timer = millis();

void setup()
{
  Serial.begin(9600);	// Debugging only
  if (!driver.init())
    Serial.println("init failed");
}

void loop()
{
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  if (driver.recv(buf, &buflen)) // Non-blocking
  {
    Serial.print(buflen);
    Serial.print(", ");
    Serial.println(millis() - timer);
  
    timer = millis();
  }