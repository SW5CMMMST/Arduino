#define DEBUG
#define VERBOSE
#include <RH_ASK.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Addr.h>
#define DELTA_COM 400
#define DELTA_PROC 400
#define TIMESLOT_LEN (DELTA_COM + DELTA_PROC)
#define INIT_WAIT (3 * TIMESLOT_LEN)
#define PAYLOAD_MAX_SIZE 16
#define GUARD_TIME_BEFORE_TX 15

typedef struct {
    uint8_t currentSlot;
    uint8_t slotCount;
    uint8_t address;
} payloadHead;

typedef struct {
    payloadHead head;
    uint8_t data[13]; // So total size is 16
} payload;

typedef struct {
    uint8_t n; // number of timeslots
    uint8_t k; // the timeslot of this device
    uint8_t i; // the timeslot currently in progress
} networkStatus;

/* GLOBAL VARIABLES */

RH_ASK rh;
uint8_t address = 0x0;
payload inPayload;
payload outPayload;
unsigned long x = 0;
uint8_t payloadBuffer[PAYLOAD_MAX_SIZE];
networkStatus netStat; 
bool foundNetwork = false;

/* END GLOBAL VARIABLES */

void setup() {
    Serial.begin(9600);
    // Init radiohead
    if (!rh.init())
        Serial.println(F("Init failed!"));

    Serial.println(F("Device started"));
    outPayload.data[1] = (uint8_t) '\0';
    // Get the address of the device
    Addr a;
    address = a.get();
    Serial.print(F("address is 0x"));
    Serial.println(address, HEX);

    // x ← 0
    resetClock(&x);

    foundNetwork = false;
    
    while(getClock(&x) <= INIT_WAIT && !foundNetwork) {
        if(rx()) {
            // FOUND NETWORK
            if(inPayload.head.slotCount > 1) {
                netStat.i = inPayload.head.currentSlot;
                netStat.n = inPayload.head.slotCount;            
                netStat.k = netStat.n - 1; // EmptySlot, is 0-indexed
                setPayloadHead(&outPayload, netStat.i,  netStat.n + 1, address);
                foundNetwork = true;
            } else {
                resetClock(&x);
            }
        }
    }

    if(!foundNetwork) {
        // Create new network
        netStat.n = 2;
        netStat.k = 0;
        netStat.i = 1; // Such that when we loop it increments to 
        outPayload.head.currentSlot = 0;
        outPayload.head.slotCount = 2;
        outPayload.head.address = address;
    }

    Serial.print(F("slotCount: "));
    Serial.println(netStat.n);
    Serial.print(F("Slot: "));
    Serial.println(netStat.i);
    Serial.print(F("My spot: "));
    Serial.println(netStat.k);

    resetClock(&x);
}

void loop() {
    while(getClock(&x) <= DELTA_PROC) {/* Do User Code */}
    nextSlot(); 
    Serial.print("Slot: ");
    Serial.print(netStat.i);
    Serial.print("\t");   
    if(netStat.i == netStat.k) {
        // Transmit!
        Serial.println("Tx");
        // Guard time
        delay(GUARD_TIME_BEFORE_TX);
        tx();
        resetClock(&x);
    } else {
        // Receive!
        foundNetwork = false;
        Serial.println("Rx");
        while(getClock(&x) <= TIMESLOT_LEN && !foundNetwork) {
            if(rx()){
                Serial.println("Eureka!");
                reSync();
                foundNetwork = true;
            }
        }
        resetClock(&x);
    }
}

int readPayloadFromBuffer(payload* payloadDest, uint8_t plSize) {
    Serial.print("READ FUCK FACE \t");
    Serial.println(plSize);
    Serial.print(payloadBuffer[1]);
    Serial.print(payloadBuffer[2]);
    memcpy(payloadDest, payloadBuffer, plSize);
    return 0;
}

void _printPayload(payload in) {
    Serial.println(F("Payload:"));
    Serial.print(F("\tcurrentSlot: "));
    Serial.println(in.head.currentSlot);

    Serial.print(F("\tslotCount: "));
    Serial.println(in.head.slotCount);

    Serial.print(F("\taddress: "));
    Serial.println(in.head.address);
}

bool rx() {
    uint8_t payloadBufferSize = 0;
    if(rh.recv(payloadBuffer,&payloadBufferSize)) {
        readPayloadFromBuffer(&inPayload, payloadBufferSize);
        _printPayload(inPayload);
        return true;
    } else {
        return false;
    }
}

void writePayloadToBuffer(payload* payloadSrc) {
    memcpy(payloadBuffer, payloadSrc, PAYLOAD_MAX_SIZE);
}

void tx() {
    writePayloadToBuffer(&outPayload);
    rh.send(payloadBuffer, PAYLOAD_MAX_SIZE);
    rh.waitPacketSent();
}

unsigned long getClock(unsigned long * x_0){
    return millis() - *x_0; 
}

void resetClock(unsigned long * x_0) {
    *x_0 = millis();
}

void setPayloadHead(payload* p, uint8_t curSlot, uint8_t slotCnt, uint8_t addr){
    p->head.currentSlot = curSlot;
    p->head.slotCount = slotCnt;
    p->head.address = addr;  
}

void nextSlot(){
    netStat.i = (netStat.i + 1) % netStat.n; // This is 0-indexed
    outPayload.head.currentSlot = netStat.i;  
}

void reSync(){
	//This is naively assuming that the transmitting device is right
    netStat.i = inPayload.head.currentSlot;
    if(inPayload.head.slotCount == 0)
        Serial.println("Something went wrong...");
    netStat.n = inPayload.head.slotCount;
}
