#define DEBUG
#define VERBOSE
#include <TDMA_Frame.h>
#include <RH_ASK.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Addr.h>
#include <Timer.h>
#include <Payload.h>
#define RECV_LED 2
#define LOSS_LED 3
#define STAT_LED 13
#define INIT_WAIT 500  //Milliseconds to wait before initing own network
#define COST 200

#ifdef DEBUG
char payloadString[40];
#endif

Frame f(COST);
Timer t;
RH_ASK driver;
Addr a;
uint16_t sync = 0;
bool do_init = true;
int tx_PID = 0;
int init_PID = 0;
int app_PID = 0;
Payload p_in  = Payload(0,0,0);
Payload p_out = Payload(0,0,a.get());

void setup() {
  Serial.begin(9600);    // Debugging only
    if (!driver.init())
         Serial.println(F("Init failed!"));
  pinMode(STAT_LED, OUTPUT);
  pinMode(RECV_LED, OUTPUT);
  pinMode(LOSS_LED, OUTPUT);
  Serial.println(F("Ready"));
  t.after(INIT_WAIT, initNetwork);
}

void loop() {
  t.update();
  rx();
}
  
void rx(){
  uint8_t len = 0;
  uint8_t buff[24];
  if(driver.recv(buff, &len)){
    p_in.setSlot(buff[0]);
    p_in.setSlotCnt(buff[1]);
    p_in.setAddr(buff[2]);
    for(uint8_t i = 3; i < len; i++){
      p_in.addData(buff[i]);  
    }   
    //DO stuff here
    if(do_init){
      t.stop(init_PID);
      do_init = false;
    }
    t.pulse(RECV_LED, 150, LOW);
    if(p_in.getSlot() == p_in.getSlotCnt()){
      #ifdef VERBOSE
      Serial.print(F("New device!  Addr: "));
      Serial.println(p_in.getAddr());
      #endif
    }
  }
}

void tx(){
  driver.send(p_out.getPayloadAsStream(), p_out.getStreamLength());
  driver.waitPacketSent();
  #ifdef VERBOSE
  Serial.println(F("Payload transmitted!"));
  #endif
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
  #endif
  t.stop(init_PID);
}

void expandNetwork(uint8_t addr, long cost){
  f.addDevice(addr, cost);  
}
