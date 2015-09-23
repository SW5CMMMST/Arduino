#include <RH_ASK.h>
#include <SPI.h>
#define DEBUG
#include <sass_ask.h>

RH_ASK driver;
const uint8_t addr = 0x02;
uint16_t sync = 0;

void setup()
{
    Serial.begin(9600);	  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
}

void loop()
{
    payload_type payload;

    payload.addr = addr;
    payload.sync = sync & 0x7FFF;
    payload.okay = 0x1;
    payload.mode = PING;
    payload.cntd = 5;
    payload.msga[0] = (uint8_t)'h';
    payload.msga[1] = (uint8_t)'e';
    payload.msga[2] = (uint8_t)'j';

    if(driver.send((uint8_t *)&payload, PAYLOAD_LEN)) {
    	Serial.println("Payload sent");
    	sync = millis() - sync;
    	driver.waitPacketSent();
    }
    else
    	Serial.println("Payload NOT sent");
    
    delay(2000);
}
