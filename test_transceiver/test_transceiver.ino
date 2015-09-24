#define DEBUG
#include <RH_ASK.h>
#include <SPI.h>
#include <sass_ask.h>

#ifdef DEBUG
char payloadString[64];
#endif

RH_ASK driver;
const uint8_t addr = 0x22;
uint16_t sync = 0;
bool h = true;
payload_type payload;

void setup()
{
    Serial.begin(9600);	  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    payload.addr = addr;
    payload.sync = sync & 0x7FFF;
    payload.okay = 0x1;
    payload.mode = PING;
    payload.cntd = 5;
    payload.msga[0] = (uint8_t)'h';
    payload.msga[1] = (uint8_t)'e';
    payload.msga[2] = (uint8_t)'j';
    delay(50);
    digitalWrite(13, LOW);
    Serial.println("ready");
}

void loop()
{
    if(h){
      driver.send((uint8_t *)&payload, sizeof(payload));
      h = false;
    }
    uint8_t len = sizeof(payload);
    if(driver.recv((uint8_t*)&payload, &len)){
      digitalWrite(13, HIGH);
      //driver.printBuffer("Got:", (uint8_t*)&payload, 16);
      #ifdef DEBUG
      makePayloadString(payload, payloadString);
      Serial.print(payloadString);
      #endif
      if(payload.cntd == 0)
        payload.cntd = 10;
      else
        payload.cntd--;
      driver.send((uint8_t *)&payload, sizeof(payload));
      driver.waitPacketSent();
      digitalWrite(13, LOW);
    }
}


