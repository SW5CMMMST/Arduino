/*  Mode defines  */
#define DEBUG

/*  Library includes  */
#include <RH_ASK.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Addr.h>

/*  Symbolic constants  */
#ifdef DEBUG
#define DELTA_COM 400
#define DELTA_PROC 100
#else
#define DELTA_COM 200
#define DELTA_PROC 50
#endif
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
  uint8_t mode;
};

struct payload {
  struct payloadHead header;
  uint8_t data[PAYLOAD_MAX_SIZE - sizeof(payloadHead)]; // So total size is 16
};

struct networkStatus {
  uint8_t n; // number of timeslots
  uint8_t k; // the timeslot of this device
  uint8_t i; // the timeslot currently in progress
};

typedef enum { NORMAL = 1, JOIN = 11, VERIFY = 22 } mode;

/*  Global Variables  */
RH_ASK rh;
uint8_t address = 0x0;
struct payload inPayload;
uint8_t inPayloadSize = 0;
struct payload outPayload;
uint8_t outPayloadSize = 0;
uint32_t x = 0;
struct networkStatus netStat;
uint8_t usercodeData[PAYLOAD_MAX_SIZE - sizeof(payloadHead)];
uint8_t usercodeDataSize = 0;

#ifdef TEST
uint32_t y = 0;
#endif

/*  Setup function  */
void setup() {
  // DELAY START
#ifdef DEBUG
  delay(5000);
#endif
  Serial.begin(9600);
  // Init radiohead
  if (!rh.init()) {
#ifdef DEBUG
    Serial.println(F("Init failed!"));
#endif
    while (true) {}
  }

  pinMode(13, OUTPUT);

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

  while (getClock(&x) <= INIT_WAIT && !foundNetwork) {
    if (rx()) {
      foundNetwork = true;
      waitForNextTimeslot(inPayloadSize); // This also resets x
    }
  }

  if (foundNetwork) {
#ifdef DEBUG
    digitalWrite(13, HIGH);
    Serial.println(F("Found Network, joining!!"));
#endif
    netStat.i = inPayload.header.currentSlot;
    netStat.n = inPayload.header.slotCount + 1;
    netStat.k = inPayload.header.slotCount - 1; // EmptySlot, is 0-indexed
    setPayloadHead(&outPayload, netStat.i,  netStat.n, address);

    while (netStat.i != netStat.n - 1) {
      nextSlot();
      while (getClock(&x) <= TIMESLOT_LEN) {
        if (rx()) {
          // reSync();
          foundNetwork = true;
        }
      }
      waitForNextTimeslot(inPayloadSize); // Resets x
    }

    resetClock(&x);
    while (getClock(&x) <= DELTA_PROC); // Wait for user-code
    delay(GUARD_TIME_BEFORE_TX);
    outPayloadSize = sizeof(payloadHead);
    tx(NULL, usercodeDataSize);

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
  while (getClock(&x) <= DELTA_PROC) {
    userCodeRepeat();
  }

#ifdef TEST
  printTask("usercode", getClock(&y));
#endif
  nextSlot();
#ifdef DEBUG
  Serial.println(F("===================================="));
  if (0 == netStat.i) {
    Serial.println(F(""));
    Serial.println(F("++++++++++++++++++++++++++++++++++++"));
    Serial.println(F(""));
    Serial.println(F("===================================="));
  }
  Serial.print("Slot: ");
  Serial.print(netStat.i);
  Serial.print("\t");
#endif
  if (netStat.i == netStat.k) {

#ifdef TEST
    resetClock(&y);
#endif
    // Transmit!
#ifdef DEBUG
    Serial.println("Tx");
#endif
    // Guard time
    delay(GUARD_TIME_BEFORE_TX);
    if (usercodeDataSize > 0) {
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
    while (getClock(&x) <= TIMESLOT_LEN && !foundNetwork) {
#ifdef DO_SENSOR_POOLING
      if (address == SENDER_ADDRESS)
        userSensorPool();
#endif
      if (rx()) {
#ifdef DEBUG
        Serial.print(F("Actual time taken to recive: "));
        Serial.println(millis() - t_0);
#endif
        reSync();
        foundNetwork = true;
      }
    }
#ifdef TEST
    if (netStat.i == netStat.n - 1) {
      printTask("EmptySlot", getClock(&y));
    } else {
      printTask("Rx", getClock(&y));
    }
#endif
    if (foundNetwork) {
      waitForNextTimeslot(inPayloadSize);
    } else {
      resetClock(&x);
    }
  }
}

void waitForNextTimeslot(uint32_t payloadSize) {
  int32_t sentTime = sentTimeCalculator(payloadSize);

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
  while (getClock(&x) <= timeLeft);
  resetClock(&x);
}

uint32_t sentTimeCalculator(uint32_t payloadSize) {
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
  // So sentTime is: [83,8129; 161,9416] with floats and [84;162] with int32_tegers
  // Rounding to int32_tegers, accurate within +-0.2
  uint32_t sentTime = 66 + (6 * payloadSize);
  return sentTime;
}

void readsPayloadFromBuffer(struct payload * payloadDest, uint8_t* payloadBuffer, uint8_t plSize) {
  inPayloadSize = plSize;
  payloadDest->header.currentSlot = payloadBuffer[0];
  payloadDest->header.slotCount = payloadBuffer[1];
  payloadDest->header.address = payloadBuffer[2];
  payloadDest->header.mode = payloadBuffer[3];
  for (int32_t i = 0; i < plSize - sizeof(payloadHead); i++) {
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
  if (rh.recv(payloadBuffer, &payloadBufferSize)) {
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
  for (int i = 0; i < dataSize; i++) {
    payloadBuffer[sizeof(payloadHead) + i] = data[i];
  }
#ifdef DEBUG
  rh.printBuffer("Sent:", payloadBuffer, sizeof(payloadHead) + dataSize);
#endif
  rh.send(payloadBuffer, sizeof(payloadHead) + dataSize);
  uint32_t sentTime = sentTimeCalculator(sizeof(payloadHead) + dataSize) - 20;
  for (int i = 0; i < 10; i++) {
    userSensorPool();
    delay(sentTime / 10);
  }
  rh.waitPacketSent();
}


uint32_t getClock(uint32_t * x_0) {
  // This also handles uint32_t overflow, which happnes after 2^32 milliseconds approx 49 days.
  // Because:        (uint32_t)((uint32_t) 4294967297 - (uint32_t) 4294967290 ) == 7 // UINT32_MAX + 2
  // Is the same as: (uint32_t)((uint32_t)          1 - (uint32_t) 4294967290 ) == 7
  return millis() - *x_0;
}

void resetClock(uint32_t * x_0) {
  *x_0 = millis();
}

void setPayloadHead(struct payload * p, uint8_t curSlot, uint8_t slotCnt, uint8_t addr) {
  p->header.currentSlot = curSlot;
  p->header.slotCount = slotCnt;
  p->header.address = addr;
}

void nextSlot() {
  netStat.i = (netStat.i + 1) % netStat.n; // This is 0-indexed
  outPayload.header.currentSlot = netStat.i;
}

void reSync() {
  if (inPayload.header.slotCount > netStat.n) {
#ifdef DEBUG
    Serial.print(F("New device joined with addr: "));
    Serial.println(inPayload.header.address);
#endif
    outPayload.header.slotCount = inPayload.header.slotCount;
    netStat.n = inPayload.header.slotCount;
  }

  if (inPayload.header.currentSlot != netStat.i) {
#ifdef DEBUG
    Serial.println(F("Resynced!"));
#endif
    netStat.i = inPayload.header.currentSlot;
  }
}

#ifdef TEST
void printTask(const char* mode, uint32_t time) {
  Serial.print(mode);
  Serial.print("\t");
  Serial.println(time);
}
#endif

struct doubleClickState {
  int previousButtonState,
      buttonState,
      upDown,
      sensor;
  uint32_t doubleClickTimeout,
           upDownTime;
};

#define DOUBLE_CLICK_MAX_TIME 1250
#define DEBOUNCE_TIME 25

// #define USER_DEBUG

struct doubleClickState dc1 = { -1, 0, HIGH, SENDER_SENSOR_1, 0, 0 };
struct doubleClickState dc2 = { -1, 0, HIGH, SENDER_SENSOR_2, 0, 0 };
int outState[4] = { 0, 0, 0, 0 };

/* Place user code which should be executed once pr. timeslot */
void userCodeRunonce() {
  if (address == SENDER_ADDRESS) {
    pinMode(SENDER_SENSOR_1, INPUT_PULLUP);
    pinMode(SENDER_SENSOR_2, INPUT_PULLUP);
    usercodeData[0] = RECEIVER_1_ADDRESS;
    usercodeData[2] = RECEIVER_2_ADDRESS;
    usercodeDataSize = 4;

    usercodeData[1] = outState[SENDER_SENSOR_1];
    usercodeData[3] = outState[SENDER_SENSOR_2];

    pinMode(5, OUTPUT);

    userSensorPool();
  } else {
    pinMode(RECEIVER_OUTPIN, OUTPUT);
    for (int i = 0; i < sizeof(inPayload.data); i++) {
      if (inPayload.data[i] == address) {
        digitalWrite(RECEIVER_OUTPIN, inPayload.data[i + 1] == 1 ? HIGH : LOW);
      }
    }
    usercodeDataSize = 0;
  }
}

/* Place user code which should be executed repeatly here */
void userCodeRepeat() {
  // Intentionally left blank
  userSensorPool();
}

/* Place user code which should be executed repeatly here concurrently while reciving */
void userSensorPool() {
  if (address == SENDER_ADDRESS) {
    checkForDoubleClick(&dc1);
    checkForDoubleClick(&dc2);
  }
}

void checkForDoubleClick(struct doubleClickState * doubleClick) {
  if ((*doubleClick).previousButtonState != (*doubleClick).buttonState) {
    (*doubleClick).previousButtonState = (*doubleClick).buttonState;
#ifdef USER_DEBUG
    Serial.print("Button[");
    Serial.print((*doubleClick).sensor);
    Serial.print("] is now: ");
    Serial.println((*doubleClick).buttonState);
#endif
  }

  /* Beck if doubleclick time was exceded  */
  if ((*doubleClick).buttonState > 1 && (millis() - (*doubleClick).doubleClickTimeout) > DOUBLE_CLICK_MAX_TIME) {
#ifdef USER_DEBUG
    Serial.print("Button[");
    Serial.print((*doubleClick).sensor);
    Serial.println("] double click was too slow!");
#endif
    (*doubleClick).buttonState = 0;
    (*doubleClick).doubleClickTimeout = millis();
  }

  /* Button state chanced! with debounce */
  int temp = digitalRead((*doubleClick).sensor);
  if (temp != (*doubleClick).upDown && (millis() - (*doubleClick).upDownTime) > DEBOUNCE_TIME) {
    (*doubleClick).upDown = temp;
    (*doubleClick).upDownTime = millis();
    //Serial.println((*doubleClick).upDown ? "NOT PRESSED" : "PRESSED" );

    ((*doubleClick).buttonState)++;

    /* Handles edge case, with invalid state */
    if ((*doubleClick).upDown == HIGH && (*doubleClick).buttonState % 2 == 1)
      (*doubleClick).buttonState = 0;

    /* Handles edge case where 2nd button click is held down for too long */
    if ((*doubleClick).upDown == LOW && (*doubleClick).buttonState % 2 == 0)
      (*doubleClick).buttonState = 0;

    /* First click is done, start timeout counter */
    if ((*doubleClick).buttonState == 1) {
      (*doubleClick).doubleClickTimeout = millis();
    }

    /* Has been turned off and on again */
    if ((*doubleClick).buttonState == 4) {
#ifdef USER_DEBUG
      Serial.print("Button[");
      Serial.print((*doubleClick).sensor);
      Serial.println("] double clicked!");
#endif
      (*doubleClick).buttonState = 0;
      outState[(*doubleClick).sensor] = !outState[(*doubleClick).sensor];
      digitalWrite(5, outState[(*doubleClick).sensor]);
    }
  }
}
