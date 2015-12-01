/*  Mode defines  */
#define DEBUG

/*  Library includes  */
#include <RH_ASK.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Addr.h>

/*  Symbolic constants  */
#define DELTA_COM 200
#define DELTA_PROC 50
#define TIMESLOT_LEN (DELTA_COM + DELTA_PROC)
#define INIT_WAIT (5 * TIMESLOT_LEN)
#define PAYLOAD_MAX_SIZE 16
#define GUARD_TIME_BEFORE_TX 30

/* User code constants */
#define DO_SENSOR_POOLING
#define SENDER_ADDRESS 0xAD
#define RECEIVER_OUTPIN 3

#define SENDER_SENSOR_1 2
#define RECEIVER_1_ADDRESS 0x13

#define SENDER_SENSOR_2 3
#define RECEIVER_2_ADDRESS 0x89

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
uint8_t inPayloadSize = 0;
struct payload outPayload;
uint8_t outPayloadSize = 0;
unsigned long x = 0;
struct networkStatus netStat;
uint8_t usercodeData[13];
uint8_t usercodeDataSize = 0;

#ifdef TEST
unsigned long y = 0;
#endif

/*  Setup function  */
void setup() {
#if (defined DEBUG) || (defined TEST)
    // DELAY START
    delay(5000);
    Serial.begin(9600);
    pinMode(13, OUTPUT);
#endif
    // Init radiohead
    if (!rh.init()) {
#ifdef DEBUG
        Serial.println(F("Init failed!"));
#endif
        while(true){}
    }

    outPayload.data[1] = (uint8_t) '\0';
    // Get the address of the device
    Addr a;
    address = a.get();
#ifdef DEBUG
    Serial.println(F("Device started"));
    Serial.print(F("Address is 0x"));
    Serial.println(address, HEX);
#endif
    resetClock(&x);
#ifdef TEST
    Serial.print("Device ");
    Serial.println(address, HEX);
    resetClock(&y);
#endif
    bool foundNetwork = false;

    while(getClock(&x) <= INIT_WAIT) {
        if(rx()) {
            protocolMaintance(inPayload.header.currentSlot, inPayload.header.slotCount, inPayloadSize);
            foundNetwork = true;
            waitForNextTimeslot(inPayloadSize);
            break;
        }
    }

    if(foundNetwork) {
      nextSlot();
      while(netStat.i != netStat.n){
        while(getClock(&x) <= DELTA_COM){
          if(rx()){
            protocolMaintance(inPayload.header.currentSlot, inPayload.header.slotCount, inPayloadSize);
            break;
          }
        }
        waitForNextTimeslot(inPayloadSize);
        netStat.i = (netStat.i%netStat.n)+1;
      }
      while(getClock(&x) <= DELTA_PROC){
      }

      netStat.k = netStat.n;
      netStat.n = netStat.n + 1;
      outPayloadSize = sizeof(payloadHead);
      tx(NULL, 0);
      waitForNextTimeslot(outPayloadSize);
    } else {
                // Create new network
        netStat.n = 2;
        netStat.k = 0;
        netStat.i = 1; // Such that when we loop it increments to
        outPayload.header.currentSlot = 0;
        outPayload.header.slotCount = 2;
        outPayload.header.address = address;

        resetClock(&x);
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
    printTask("connecting", getClock(&y));
#endif

}

/*  Main loop  */
void loop() {
#ifdef TEST
    resetClock(&y);
#endif

    userCodeRunonce();
    while(getClock(&x) <= DELTA_PROC) {
        userCodeRepeat();
    }

#ifdef TEST
    printTask("usercode", getClock(&y));
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
   resetClock(&y);
#endif
        // Transmit!
#ifdef DEBUG
        Serial.println("Tx");
#endif
        // Guard time
        delay(GUARD_TIME_BEFORE_TX);
        if(usercodeDataSize > 0) {
          outPayloadSize = sizeof(payloadHead) + usercodeDataSize;
          tx(usercodeData, usercodeDataSize);
        } else {
            outPayloadSize = sizeof(payloadHead);
            tx(NULL, 0);
        }
#ifdef TEST
    printTask("Tx", getClock(&y));
#endif
        waitForNextTimeslot(outPayloadSize);
    } else {
#ifdef TEST
   resetClock(&y);
#endif
        // Receive!
        bool foundNetwork = false;
#ifdef DEBUG
        Serial.println("Rx");
        long t_0 = millis();
#endif
        while(getClock(&x) <= TIMESLOT_LEN && !foundNetwork) {
#ifdef DO_SENSOR_POOLING
            if(address == SENDER_ADDRESS)
                userSensorPool();
#endif
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
    if(netStat.i == netStat.n - 1) {
        printTask("EmptySlot", getClock(&y));
    } else {
        printTask("Rx", getClock(&y));
    }
#endif
        if(foundNetwork) {
            waitForNextTimeslot(inPayloadSize);
        } else {
            resetClock(&x);
        }
    }
}

void protocolMaintance(int i, int n, int len){
  if(netStat.n < n){
    netStat.n = n;
  }
  netStat.i = i;
  int transDur = 66 + (len * 6);
  int transTime = x - transDur;
  int slotTime = transTime - DELTA_PROC;
  x = x - slotTime;
}

void waitForNextTimeslot(uint32_t payloadSize) {
    /* Here we wait until the timeslot is over.
     * To sync with the network we wait untill their timeslot is over.
     * Worst case wait time: f(x) = 6.0101 ∗ x + 65.7826
     * f(PAYLOAD_MAX_SIZE) = f(16) = 161.9416 [ms]
     * f(PAYLOAD_MAX_SIZE) + GUARD_TIME_BEFORE_TX = 191.9416 [ms]
     * Which is strictly less than DELTA_COM (200 [ms])
     *
     * To calculate the start of the next slot we take the size of the message
     * recived, and calculate how much is left of the timeslot.
     * Then we hope we are correct and it syncs up.
     */

     // inPayloadSize is always in the range [3;16]
     // So sentTime is: [83,8129; 161,9416] with floats and [84;162] with integers
     // Rounding to integers, accurate within +-0.2
     uint32_t sentTime = 66 + (6 * payloadSize);

     // Will be from 200 -  84 - 30 = 86
     //           to 200 - 162 - 30 = 8
     uint32_t timeLeft = DELTA_COM - GUARD_TIME_BEFORE_TX - sentTime;

     resetClock(&x);
#ifdef DEBUG
     Serial.print("Waiting for: ");
     Serial.print(timeLeft);
     Serial.println(" [ms]");
#endif
#ifdef TEST
    printTask("BuzyWaiting", timeLeft);
#endif
     while(getClock(&x) <= timeLeft);
     resetClock(&x);
}

void readsPayloadFromBuffer(struct payload* payloadDest, uint8_t* payloadBuffer, uint8_t plSize) {
    inPayloadSize = plSize;
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
#ifdef DEBUG
        rh.printBuffer("Got:", payloadBuffer, payloadBufferSize);
#endif
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
    if(inPayload.header.slotCount > netStat.n) {
#ifdef DEBUG
        Serial.print(F("New device joined with addr: "));
        Serial.println(inPayload.header.address);
#endif
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
void printTask(const char* mode, unsigned long time){
    Serial.print(mode);
    Serial.print("\t");
    Serial.println(time);
}
#endif

/* Place user code which should be executed once pr. timeslot */
void userCodeRunonce() {
    if(address == SENDER_ADDRESS) {
        pinMode(SENDER_SENSOR_1, INPUT_PULLUP);
        pinMode(SENDER_SENSOR_2, INPUT_PULLUP);
        usercodeData[0] = RECEIVER_1_ADDRESS;
        usercodeData[2] = RECEIVER_2_ADDRESS;
        usercodeDataSize = 4;

        // Reset right after transmission
        if(netStat.i == netStat.k) {
            usercodeData[1] = LOW;
            usercodeData[3] = LOW;
        }

        userSensorPool();
    } else {
        pinMode(RECEIVER_OUTPIN, OUTPUT);
        for(int i = 0; i < sizeof(inPayload.data); i++) {
            if(inPayload.data[i] == address) {
                digitalWrite(RECEIVER_OUTPIN, inPayload.data[i + 1] == 1 ? HIGH : LOW);
            }
        }
        usercodeDataSize = 0;
    }
}

/* Place user code which should be executed repeatly here */
void userCodeRepeat() {
    // Intentionally left blank
}

/* Place user code which should be executed repeatly here concurrently while reciving */
void userSensorPool() {
    if(digitalRead(SENDER_SENSOR_1) == LOW) {
        usercodeData[1] = HIGH;
    }

    if(digitalRead(SENDER_SENSOR_2) == LOW) {
        usercodeData[3] = HIGH;
    }
}

