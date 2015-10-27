#define DEBUG
#include <TDMA_Frame.h>
#include <RH_ASK.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Addr.h>
#include <Timer.h>
#include <sass_ask.h>
#define RECV_LED 2
#define LOSS_LED 3
#define STAT_LED 13
#define INIT_WAIT 5000  //Milliseconds to wait before initing own network
#define COST 200

#ifdef DEBUG
char payloadString[64];
#endif

Frame f(COST);
Timer t;
RH_ASK driver;
Addr a;
uint16_t sync = 0;
bool do_init = true;
payload_type payload_in;
payload_type payload_out;
int tx_PID = 0;
int init_PID = 0;

void setup() {
  Serial.begin(9600);    // Debugging only
    if (!driver.init())
         Serial.println("init failed");
  pinMode(STAT_LED, OUTPUT);
  pinMode(RECV_LED, OUTPUT);
  pinMode(LOSS_LED, OUTPUT);
  Serial.println("ready");
  setupTestPayload(&payload_out);
  t.after(INIT_WAIT, initNetwork); 
}

void loop() {
  t.update();
  rx(&payload_in);
}

void rx(payload_type* payload_buffer){
  uint8_t len = sizeof(payload_type);
  if(driver.recv((uint8_t*)payload_buffer, &len)){
    //DO stuff here
    if(do_init){
      t.stop(init_PID);
      do_init = false;
    }
    t.pulse(RECV_LED, 150, LOW);
    if(payload_buffer->mode == INIT){
      Serial.print("New device!  Addr: ");
      Serial.println(payload_buffer->addr);
    }
    #ifdef DEBUG
    makePayloadString(*payload_buffer, payloadString);
    Serial.print(payloadString);
    #endif
  }
}

void tx(){
  driver.send((uint8_t *)&payload_out, sizeof(payload_type));
  driver.waitPacketSent();
}

void setupTestPayload(payload_type* payload){
  payload->addr = a.get();
  payload->sync = 100 & 0x7FFF;
  payload->okay = true;
  payload->mode = PING;
  payload->cntd = 0;
  payload->msga[0] = (uint8_t)'h';
  payload->msga[1] = (uint8_t)'e';
  payload->msga[2] = (uint8_t)'j'; 
}

void initNetwork(){
  f.addDevice(a.get(), COST);
  tx_PID = t.every(f.getWaitTime(), tx);
  t.stop(init_PID);
  do_init = false;
}
