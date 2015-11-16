#define DEBUG
#define VERBOSE
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
#define COST 2000

#ifdef DEBUG
char payloadString[40];
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
         Serial.println(F("Init failed!"));
  pinMode(STAT_LED, OUTPUT);
  pinMode(RECV_LED, OUTPUT);
  pinMode(LOSS_LED, OUTPUT);
  Serial.println(F("Ready"));
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
      #ifdef VERBOSE
      Serial.print(F("New device!  Addr: "));
      Serial.println(payload_buffer->addr);
      #endif
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
  #ifdef VERBOSE
  Serial.println(F("Payload transmitted!"));
  #endif
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
  #ifdef VERBOSE
  Serial.println(F("Initiating network..."));
  #endif
  if(f.addDevice(a.get(), COST))
    Serial.println(F("Added myself"));
  tx_PID = t.every(f.getWaitTime(), tx);
  do_init = false;
  #ifdef VERBOSE
  Serial.println(F("Network initiated!"));
  Serial.print(F("Wait time: "));
  Serial.println(f.getWaitTime());
  f.debugN1(0);
  f.debugN1(1);
  #endif
  t.stop(init_PID);
}

void expandNetwork(uint8_t addr, long cost){
  f.addDevice(addr, cost);
   
}
