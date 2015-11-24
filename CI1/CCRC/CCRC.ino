/*  Mode defines  */
#define DEBUG
#define TEST

/*  Library includes  */
#include <RH_ASK.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Addr.h>

/*  Symbolic constants  */
#define DELTA_COM 400
#define DELTA_PROC 200
#define TIMESLOT_LEN (DELTA_COM + DELTA_PROC)
#define INIT_WAIT (5 * TIMESLOT_LEN)
#define PAYLOAD_MAX_SIZE 16
#define GUARD_TIME_BEFORE_TX 30

/*  Data structures  */
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

/*  Global Variables  */
RH_ASK rh;
uint8_t address = 0x0;
struct payload inPayload;
struct payload outPayload;
unsigned long x = 0;
struct networkStatus netStat; 
bool foundNetwork = false;
uint32_t counter = 0;

/*  Setup function  */
void setup() {
    // DELAY START
    delay(5000);
#ifdef DEBUG
    Serial.begin(9600);
#endif
    // Init radiohead
    if (!rh.init()) {
#ifdef DEBUG
        Serial.println(F("Init failed!"));
#endif
        while(true){}
    }

    pinMode(13, OUTPUT);
    if(address == 0xAD) { 
        pinMode(2, INPUT);
        pinMode(3, INPUT);
    } else {
        pinMode(3, OUTPUT);
    }

    outPayload.data[1] = (uint8_t) '\0';
    // Get the address of the device
    Addr a;
    address = a.get();
#ifdef DEBUG
    Serial.println(F("Device started"));
#endif
#ifdef DEBUG
    Serial.print(F("Address is 0x"));
    Serial.println(address, HEX);
#endif
    resetClock(&x);
#ifdef TEST
    printTask(true, 0);
#endif
    foundNetwork = false;
    
    while(getClock(&x) <= INIT_WAIT && !foundNetwork) {
        if(rx()) {
            digitalWrite(13, HIGH);
            // FOUND NETWORK
#ifdef DEBUG
            Serial.println(F("Found Network, joining!!"));
#endif
            if(inPayload.header.slotCount > 1) {
                netStat.i = inPayload.header.currentSlot;
                netStat.n = inPayload.header.slotCount + 1;
                netStat.k = inPayload.header.slotCount - 1; // EmptySlot, is 0-indexed
                setPayloadHead(&outPayload, netStat.i,  netStat.n, address);
                foundNetwork = true;
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
#ifdef DEBUG
        Serial.println("Found no network, starting own");
#endif
    }

#ifdef DEBUG
    Serial.print(F("slotCount: "));
    Serial.println(netStat.n);
    Serial.print(F("Slot: "));
    Serial.println(netStat.i);
    Serial.print(F("My spot: "));
    Serial.println(netStat.k);
#endif

#ifdef TEST
    printTask(false, 0);
#endif
    resetClock(&x);
}

/*  Main loop  */
void loop() {
    bool runOnce = false;

#ifdef TEST
    printTask(true, 3);
#endif
    while(getClock(&x) <= DELTA_PROC) {
        if(!runOnce) {
            for(int i = 0; i < sizeof(inPayload.data); i++) {
                if(inPayload.data[i] == address) {
                    digitalWrite(3, inPayload.data[i + 1] == 1 ? HIGH : LOW);
                } 
            }
            runOnce = true;
        }
    }
       
#ifdef TEST
    printTask(false, 3);
#endif
    nextSlot(); 
#ifdef DEBUG
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
#endif
    if(netStat.i == netStat.k) {
        
#ifdef TEST
    printTask(true, 2);
#endif
        // Transmit!
#ifdef DEBUG
        Serial.println("Tx");
#endif
        // Guard time
        delay(GUARD_TIME_BEFORE_TX);
        if(address == 0xAD) { 
            uint8_t data[PAYLOAD_MAX_SIZE - sizeof(payloadHead)];
            uint8_t dataSize = 0;
            
            data[dataSize++] = 0x13;
            data[dataSize++] = !digitalRead(2);

            data[dataSize++] = 0x89;
            data[dataSize++] = !digitalRead(3);
            tx(data, dataSize);
        } else {
            tx(NULL, 0);
        }
#ifdef TEST
    printTask(false, 2);
#endif
        resetClock(&x);
    } else {
#ifdef TEST
    printTask(true, 1);
#endif
        // Receive!
        foundNetwork = false;
#ifdef DEBUG
        Serial.println("Rx");
#endif
        long t_0 = millis();
        while(getClock(&x) <= TIMESLOT_LEN && !foundNetwork) {
            if(rx()){
#ifdef DEBUG
                Serial.print(F("Actual time taken to recive: "));
                Serial.println(millis() - t_0);
#endif
                reSync();
                foundNetwork = true;
            }
        }
#ifdef TEST
    printTask(false, 1);
#endif
        resetClock(&x);
    }
}

void readsPayloadFromBuffer(struct payload* payloadDest, uint8_t* payloadBuffer, uint8_t plSize) {
    payloadDest->header.currentSlot = payloadBuffer[0];
    payloadDest->header.slotCount = payloadBuffer[1];    
    payloadDest->header.address = payloadBuffer[2];
    for(int i = 0; i < plSize - sizeof(payloadHead); i++) {
        payloadDest->data[i] = payloadBuffer[sizeof(payloadHead) + i]; 
    }
}

#ifdef DEBUG
void _printPayload(struct payload in) {
    Serial.println(F("Payload:"));
    Serial.print(F("\tcurrentSlot: "));
    Serial.println(in.header.currentSlot);

    Serial.print(F("\tslotCount: "));
    Serial.println(in.header.slotCount);

    Serial.print(F("\taddress: "));
    Serial.println(in.header.address);
}
#endif

bool rx() {
    uint8_t payloadBuffer[PAYLOAD_MAX_SIZE];
    memset(payloadBuffer, 'a', sizeof(payloadBuffer));
    uint8_t payloadBufferSize = sizeof(payloadBuffer);
    if(rh.recv(payloadBuffer,&payloadBufferSize)) {
        rh.printBuffer("Got:", payloadBuffer, payloadBufferSize);
        readsPayloadFromBuffer(&inPayload, payloadBuffer, payloadBufferSize);
        return true;
    } else {
        return false;
    }
}

void tx(uint8_t * data, uint8_t dataSize) {
    uint8_t payloadBuffer[PAYLOAD_MAX_SIZE];
    memset(payloadBuffer, 'a', sizeof(payloadBuffer));
    payloadBuffer[0] = outPayload.header.currentSlot;
    payloadBuffer[1] = outPayload.header.slotCount;
    payloadBuffer[2] = address;
    for(int i = 0; i < dataSize; i++) {
        payloadBuffer[sizeof(payloadHead) + i] = data[i];
    }
#ifdef DEBUG
    rh.printBuffer("Sent:", payloadBuffer, sizeof(payloadHead) + dataSize);
#endif
    rh.send(payloadBuffer, sizeof(payloadHead) + dataSize);
    rh.waitPacketSent();
}

unsigned long getClock(unsigned long * x_0){
    return millis() - *x_0; 
}

void resetClock(unsigned long * x_0) {
#ifdef DEBUG
    if(millis() < *x_0)
        Serial.println(F("Timer overflowed ..."));
#endif
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
#ifdef DEBUG
    if(inPayload.header.slotCount == 0)
        Serial.println("Something went wrong...");
#endif
    if(inPayload.header.slotCount > netStat.n) {
      outPayload.header.slotCount = inPayload.header.slotCount;
      netStat.n = inPayload.header.slotCount;
    }

    if(inPayload.header.currentSlot != netStat.i) {
#ifdef DEBUG
        Serial.println(F("Resynced!"));
#endif
        netStat.i = inPayload.header.currentSlot;
    }
}

#ifdef TEST
void printTask(bool start, uint8_t taskID){
    if(start) {
        Serial.print(addr);
        Serial.print("\t");
        Serial.print(millis());
        Serial.print("\t");
        return;
    }
    Serial.print(millis());
    Serial.print("\t");
    Serial.println(taskID);
}
#endif
