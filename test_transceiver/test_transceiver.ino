#define DEBUG
#include <RH_ASK.h>
#include <SPI.h>
#include <sass_ask.h>
#include <Timer.h>

#ifdef DEBUG
char payloadString[64];
#endif

Timer t;
RH_ASK driver;
const uint8_t addr = 0x66;
uint16_t sync = 0;
bool h = true;
payload_type payload;

void setup()
{
    Serial.begin(9600);	  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
    pinMode(13, OUTPUT);
    Serial.println("ready");
    payload.addr = addr;
    payload.sync = sync & 0x7FFF;
    payload.okay = 0x1;
    payload.mode = INIT;
    payload.cntd = 0;
    payload.msga[0] = (uint8_t)'h';
    payload.msga[1] = (uint8_t)'e';
    payload.msga[2] = (uint8_t)'j';
}

void loop()
{
    t.update();
    if(h){
      blink_status();
      payload.mode = INIT;
      driver.send((uint8_t *)&payload, sizeof(payload));
      Serial.println("initial pkg sent");
      h = false;
    }
    uint8_t len = sizeof(payload);
    if(driver.recv((uint8_t*)&payload, &len)){
      blink_status();
      //driver.printBuffer("Got:", (uint8_t*)&payload, 16);
      #ifdef DEBUG
      makePayloadString(payload, payloadString);
      Serial.print(payloadString);
      #endif
//      if(payload.cntd == 0)
//        payload.cntd = 10;
//      else
      payload.cntd++;
      payload.mode = PING;
      driver.send((uint8_t *)&payload, sizeof(payload));
      driver.waitPacketSent();
    }
    
}

void blink_status(){
  t.pulse(13, 333, LOW);  
}

