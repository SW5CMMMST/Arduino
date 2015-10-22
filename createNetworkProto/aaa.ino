#include <EEPROM.h>

#define DEBUG
#include <PJON_ASK.h>
#define RX_PIN 11
#define TX_PIN 12
#define SLOTLENGTH 300

#ifdef DEBUG
char payloadString[64];
#endif

typedef struct {
    //uint16_t sync : 15;
    //uint8_t  okay :  1;
    uint8_t  slot;
    uint8_t  slotCount;
    uint8_t  addr;
    //uint8_t  mode :  3;
    //uint8_t  cntd :  5;
    uint8_t  msga[12];
} payload_type; 

PJON_ASK network(RX_PIN, TX_PIN, EEPROM.read(0x00));
payload_type out_payload;
payload_type in_payload;
int curSlot;
int mySlot;
int slotCount;
uint8_t addr;

static void receiver_function(uint8_t length, uint8_t *payload) {
  memcpy(&in_payload, payload, length);
}

void makePayload(){
  out_payload.addr = addr;
  out_payload.slot = mySlot;
  out_payload.slotCount = slotCount;
}

void setupNetwork(){
  mySlot = 1;
  curSlot = 1;
  slotCount = 2;
  makePayload();
}

void connectToNetwork(){
  mySlot = in_payload.slotCount;
  slotCount = in_payload.slotCount + 1;
  curSlot = mySlot;
  makePayload();
  delay((in_payload.slot - 1)*SLOTLENGTH);
  network.update();
}

void setup() {
  addr = EEPROM.read(0x00);
  Serial.begin(9600);
  network.set_receiver(receiver_function);
  int response = network.receive(20000);
  if(response == ACK){
    connectToNetwork();
  }
  else{
    setupNetwork();
  }
}

void loop() {
  if(curSlot == mySlot){
    Serial.print("Sending (");
    Serial.print(mySlot, HEX);
    Serial.print(", ");
    Serial.print(slotCount, HEX);
    Serial.println(")");
    long time = millis();
    network.send(BROADCAST, (char*)&out_payload, sizeof(out_payload));
    network.update();
    delay(SLOTLENGTH-(millis()-time));
  } else {
    Serial.println("Empty slot");
    network.receive(SLOTLENGTH*1000L);
  }
  curSlot--;
  if(curSlot == -1){
    curSlot = slotCount - 1;
  }
}
