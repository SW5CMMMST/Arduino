#define DEBUG
#define VERBOSE
#include <RH_ASK.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Addr.h>
#define DELTA_COM 400
#define DELTA_PROC 200
#define TIMESLOT_LEN (DELTA_COM + DELTA_PROC)
#define INIT_WAIT (5 * TIMESLOT_LEN)
#define PAYLOAD_MAX_SIZE 16
#define GUARD_TIME_BEFORE_TX 30

struct payloadHead {
    uint8_t currentSlot;
    uint8_t slotCount;
    uint8_t address;
};

struct payload {
    struct payloadHead header;
    uint8_t data[13]; // So total size is 16
};

struct networkStatus {
    uint8_t n; // number of timeslots
    uint8_t k; // the timeslot of this device
    uint8_t i; // the timeslot currently in progress
};

/* GLOBAL VARIABLES */

RH_ASK rh;
uint8_t address = 0x0;
struct payload inPayload;
struct payload outPayload;
unsigned long x = 0;
struct networkStatus netStat; 
bool foundNetwork = false;
uint32_t counter = 0;

/* END GLOBAL VARIABLES */

void setup() {
    // DELAY START
    delay(5000);
    Serial.begin(9600);
    // Init radiohead
    if (!rh.init())
        Serial.println(F("Init failed!"));

    pinMode(13, OUTPUT);

    Serial.println(F("Device started"));
    outPayload.data[1] = (uint8_t) '\0';
    // Get the address of the device
    Addr a;
    address = a.get();
    Serial.print(F("address is 0x"));
    Serial.println(address, HEX);
    resetClock(&x);

    foundNetwork = false;
    
    while(getClock(&x) <= INIT_WAIT && !foundNetwork) {
        if(rx()) {
            digitalWrite(13, HIGH);
            // FOUND NETWORK
            Serial.println(F("Found Network, joining!!"));
            if(inPayload.header.slotCount > 1) {
                netStat.i = inPayload.header.currentSlot;
                netStat.n = inPayload.header.slotCount + 1;
                netStat.k = inPayload.header.slotCount - 1; // EmptySlot, is 0-indexed
                setPayloadHead(&outPayload, netStat.i,  netStat.n, address);
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
        outPayload.header.currentSlot = 0;
        outPayload.header.slotCount = 2;
        outPayload.header.address = address;
        Serial.println("Found no network, starting own");
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
    Serial.println(F("===================================="));
    if(0 == netStat.i) {
      Serial.println(F(""));
      Serial.println(F("++++++++++++++++++++++++++++++++++++"));
      Serial.println(F(""));
      Serial.println(F("===================================="));
    }
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
        long t_0 = millis();
        while(getClock(&x) <= TIMESLOT_LEN && !foundNetwork) {
            if(rx()){
                Serial.print(F("Actual time taken to recive: "));
                Serial.println(millis() - t_0);
                reSync();
                foundNetwork = true;
            }
        }
        resetClock(&x);
    }
}

void readsPayloadFromBuffer(struct payload* payloadDest, uint8_t* payloadBuffer, uint8_t plSize) {
    payloadDest->header.currentSlot = payloadBuffer[0];
    payloadDest->header.slotCount = payloadBuffer[1];    
    payloadDest->header.address = payloadBuffer[2];    
}

void _printPayload(struct payload in) {
    Serial.println(F("Payload:"));
    Serial.print(F("\tcurrentSlot: "));
    Serial.println(in.header.currentSlot);

    Serial.print(F("\tslotCount: "));
    Serial.println(in.header.slotCount);

    Serial.print(F("\taddress: "));
    Serial.println(in.header.address);
}

bool rx() {
    uint8_t payloadBuffer[PAYLOAD_MAX_SIZE];
    memset(payloadBuffer, 'a', sizeof(payloadBuffer));
    uint8_t payloadBufferSize = sizeof(payloadBuffer);
    if(rh.recv(payloadBuffer,&payloadBufferSize)) {
        rh.printBuffer("Got:", payloadBuffer, payloadBufferSize);
        readsPayloadFromBuffer(&inPayload, payloadBuffer, payloadBufferSize);
        // _printPayload(inPayload);
        return true;
    } else {
        return false;
    }
}

void tx() {
    uint8_t payloadBuffer[PAYLOAD_MAX_SIZE];
    memset(payloadBuffer, 'a', sizeof(payloadBuffer));
    payloadBuffer[0] = outPayload.header.currentSlot;
    payloadBuffer[1] = outPayload.header.slotCount;
    payloadBuffer[2] = address;
    payloadBuffer[3] = (++counter) % 256;
    rh.printBuffer("Sent:", payloadBuffer, PAYLOAD_MAX_SIZE - 12);
    rh.send(payloadBuffer, PAYLOAD_MAX_SIZE - 12);
    rh.waitPacketSent();
}

unsigned long getClock(unsigned long * x_0){
    return millis() - *x_0; 
}

void resetClock(unsigned long * x_0) {
    if(millis() < *x_0)
        Serial.println(F("Timer overflowed ..."));

    *x_0 = millis();
}

void setPayloadHead(struct payload* p, uint8_t curSlot, uint8_t slotCnt, uint8_t addr){
    p->header.currentSlot = curSlot;
    p->header.slotCount = slotCnt;
    p->header.address = addr;  
}

void nextSlot(){
    netStat.i = (netStat.i + 1) % netStat.n; // This is 0-indexed
    outPayload.header.currentSlot = netStat.i;  
}

void reSync(){
    if(inPayload.header.slotCount == 0)
        Serial.println("Something went wrong...");
    if(inPayload.header.slotCount > netStat.n) {
      outPayload.header.slotCount = inPayload.header.slotCount;
      netStat.n = inPayload.header.slotCount;
    }

    if(inPayload.header.currentSlot != netStat.i) {
        Serial.println(F("Resynced!"));
        netStat.i = inPayload.header.currentSlot;
    }
}


