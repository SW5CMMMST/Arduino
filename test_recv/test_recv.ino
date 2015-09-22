#include <RH_ASK.h>
#include <SPI.h>
#define DEBUG
#include <sass_ask.h>

RH_ASK driver;
const uint8_t addr = 0x03;

void setup()
{
    Serial.begin(9600);	  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
}

void loop()
{
    payload_type payload;
    uint8_t plen;
    char payloadString[64];
    
    if(driver.recv((uint8_t *)&payload, &plen)) {
    	if(plen == 16){
    	  Serial.print("Payload revceived ");
        getPayloadString(payload, payloadString);
        Serial.print(payloadString);
    	}
      Serial.println("Bad payload!");
    }
}
