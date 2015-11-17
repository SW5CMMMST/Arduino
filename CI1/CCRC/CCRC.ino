#define DEBUG
#define VERBOSE
#include <RH_ASK.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Addr.h>
#define DELTA_COM 300
#define DELTA_PROC 200
#define TIMESLOT_LEN DELTA_COM + DELTA_PROC
#define INIT_WAIT 3 * TIMESLOT_LEN
#define PAYLOAD_MAX_SIZE 16

typedef struct {
    uint8_t CurrentSlot;
    uint8_t SlotCount;
    uint8_t Address;
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

RH_ASK rh;
uint8_t address = 0x0;
payload inPayload;
payload outPayload;
unsigned long x = 0;
uint8_t payloadBufferSize = PAYLOAD_MAX_SIZE;
uint8_t payloadBuffer[PAYLOAD_MAX_SIZE];
networkStatus netStat; 
bool foundNetwork = false;

void setup() {
    Serial.begin(9600);
    // Init radiohead
    if (!rh.init())
             Serial.println(F("Init failed!"));

    Serial.println(F("Device started"));

    // Get the address of the device
    Addr a;
    address = a.get();
    Serial.print(F("Address is 0x"));
    Serial.println(address, HEX);

    // x ← 0
    resetClock(&x);

    foundNetwork = false;
    
    while(getClock(&x) <= INIT_WAIT && !foundNetwork) {
        if(rx()) {
            // FOUND NETWORK
            netStat.n = inPayload.head.SlotCount;
            netStat.i = inPayload.head.CurrentSlot;
            netStat.k = netStat.n - 1; // EmptySlot, is 0-indexed
            
            foundNetwork = true;
        }
    }

    if(!foundNetwork) {
        // Create new network
        netStat.n = 2;
        netStat.k = 0;
        netStat.i = 1; // Such that when we loop it increments to 
    }

    Serial.print(F("SlotCount: "));
    Serial.println(netStat.n);
    Serial.print(F("Slot: "));
    Serial.println(netStat.i);
    Serial.print(F("My spot: "));
    Serial.println(netStat.k);

    resetClock(&x);
}

void loop() {
    while(getClock(&x) <= DELTA_PROC) {
        // Do User Code
    }
    
    netStat.i = (netStat.i + 1) % netStat.n; // This is 0-indexed
    
    if(netStat.i == netStat.k) {
        // Transmit!
        tx();
        resetClock(&x);
        Serial.println("Transmitting!!!");
    } else {
        // Receive!
        foundNetwork = false;
        while(getClock(&x) <= TIMESLOT_LEN && !foundNetwork) {
            if(rx()) {
                foundNetwork = true;
                Serial.println("MODTAGET!!!");
            }
        }
        resetClock(&x);
        Serial.println("Recieving!!!");
    }
}

void decodePayload() {
    memcpy(&inPayload, &payloadBuffer, PAYLOAD_MAX_SIZE);
}

void printPayload(payload in) {
    Serial.print(F("CurrentSlot: "));
    Serial.println(in.head.CurrentSlot);

    Serial.print(F("SlotCount: "));
    Serial.println(in.head.SlotCount);

    Serial.print(F("Address: "));
    Serial.println(in.head.Address);
}

bool rx() {
    if(rh.recv(payloadBuffer,&payloadBufferSize)) {
        decodePayload();
        printPayload(inPayload);
        return true;
    } else {
        return false;
    }
}

void encodePayload() {
    memcpy(&payloadBuffer, &inPayload, PAYLOAD_MAX_SIZE);
}

void tx() {
    encodePayload();
    rh.send(payloadBuffer, payloadBufferSize);
    rh.waitPacketSent();
}

unsigned long getClock(unsigned long * x_0){
    return millis() - *x_0; 
}

void resetClock(unsigned long * x_0) {
    *x_0 = millis();
}

