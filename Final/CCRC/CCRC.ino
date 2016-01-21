/*  Mode defines  */
#define DEBUG
#define MULTICONNECT
#define MULTISTART

/*  Library includes  */
#include <RH_ASK.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Addr.h>

/*  Symbolic constants  */
#ifdef DEBUG
#define DELTA_COM 500
#define DELTA_PROC 100
#else
#define DELTA_COM 200
#define DELTA_PROC 50
#endif
#define TIMESLOT_LEN (DELTA_COM + DELTA_PROC)
#define INIT_WAIT (5 * TIMESLOT_LEN)
#define PAYLOAD_MAX_SIZE 16
#define GUARD_TIME_BEFORE_TX 30
#define EXPONENTIALBACKOFFMAX 5
#define KILL_CHANCE_IN_PERCENT 10

/* User code constants */
#define DO_SENSOR_POOLING
#define SENDER_ADDRESS 0x89
#define RECEIVER_OUTPIN 3

#define SENDER_SENSOR_1 2
#define RECEIVER_1_ADDRESS 0x13

#define SENDER_SENSOR_2 3
#define RECEIVER_2_ADDRESS 0x89


#define PULSETOTOGGLE

/* Demo stuff */
#define SEND_LED 5
#define RECEIVE_LED 6

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

typedef enum { NORMAL = 1, JOIN = 2, VERIFY = 3 } mode;

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

bool verifyNext = false;
bool verifiedLast = false;
uint8_t addressToVerify = 0x0;
uint8_t exponentialBackoffC = 0;
#ifdef PULSETOTOGGLE
uint8_t pinState = 0;
#endif

#ifdef MULTISTART
uint8_t failedCreateAttemptCounter = 0;
#endif

#ifdef TEST
uint32_t y = 0;
#endif

/*  Setup function  */
void setup() {
  Serial.begin(9600);
#ifdef DEBUG
  Serial.println(F("Device started!!"));
  for (int i = 0; i < 20; i++) {
    Serial.print(F("."));
    delay(250);
  }
  Serial.println("");
#endif

  if (!rh.init()) {
#ifdef DEBUG
    Serial.println(F("Init failed!"));
#endif
    while (true) {}
  }

  pinMode(13, OUTPUT);
  pinMode(SEND_LED, OUTPUT);
  pinMode(RECEIVE_LED, OUTPUT);

  outPayload.data[1] = (uint8_t) '\0';
  // Get the address of the device
  Addr a;
  address = a.get();

  randomSeed(analogRead(0) + address);

#ifdef DEBUG
  Serial.println(F("Device started"));
  Serial.print(F("Address is 0x"));
  Serial.println(address, HEX);
#endif
#ifdef TEST
  Serial.print("Device ");
  Serial.println(address, HEX);
  resetClock(&y);
#endif
  StartUp();
  outPayload.header.mode = NORMAL;
}

void StartUp() {
  netStat.i = 0;
  netStat.k = 0;
  netStat.n = 0;

  setPayloadHead(&outPayload, netStat.i, netStat.n, 0, NORMAL, sizeof(payloadHead));
  setPayloadHead(&inPayload, netStat.i, netStat.n, 0, NORMAL, sizeof(payloadHead));

  bool foundNetwork = false;
  resetClock(&x);
#ifdef MULTISTART
  // Get random for exp back
  uint32_t rng = random((1 << failedCreateAttemptCounter) - 1);
#ifdef DEBUG
  Serial.print(F("Random extra startup time is: "));
  Serial.print(rng);
  Serial.print(F(" * "));
  Serial.print(TIMESLOT_LEN);
  Serial.println(F(" [ms]"));
#endif
  while (getClock(&x) <= (INIT_WAIT + (rng * TIMESLOT_LEN)) && !foundNetwork) {
#else
  while (getClock(&x) <= INIT_WAIT && !foundNetwork) {
#endif
    if (rx()) {
      foundNetwork = true;
      waitForNextTimeslot(inPayloadSize); // This also resets x
    }
  }

  if (foundNetwork) {
#ifdef MULTICONNECT
    if (!connectToNetworkMultiConnect()) {
#ifdef DEBUG
      Serial.println(F("Starting over!!"));
      StartUp();
      return;
#endif
    }
#else
    connectToNetwork();
#endif
  } else {
    // Create new network
    netStat.n = 2;
    netStat.k = 0;
    netStat.i = 1; // Such that when we loop it increments to
    setPayloadHead(&outPayload, netStat.i, netStat.n, address, NORMAL, sizeof(payloadHead));
    resetClock(&x);
#ifdef DEBUG
    Serial.println("Found no network, starting own");
    Serial.print(F("test2: Starting network after: "));
    Serial.print(millis() - 5000);
    Serial.println(F(" [ms] Local time"));
#endif
  }

#ifdef TEST
  printTask("connecting", getClock(&y));
#endif
}

bool connectToNetworkMultiConnect() {
  netStat.i = inPayload.header.currentSlot;
  netStat.n = inPayload.header.slotCount; // Don't increment n yet.
  //netStat.k = inPayload.header.slotCount - 1; // EmptySlot, is 0-indexed
  setPayloadHead(&outPayload, netStat.i, netStat.n, address, JOIN, sizeof(payloadHead));

  /* Wait for emptyslot */
  bool foundNetwork = false;

  nextSlot();
  while (netStat.i != netStat.n - 1) {
    while (getClock(&x) <= DELTA_PROC); // Wait for user-code
    nextSlot();
    foundNetwork = false;

    while (getClock(&x) <= TIMESLOT_LEN && !foundNetwork) {
      if (rx()) {
        foundNetwork = true;
        if (inPayload.header.mode != NORMAL ) {
          nextSlot();
#ifdef DEBUG
          Serial.println(F("Someone else joined, starting over"));
#endif
          return false;
        }
      }
    }
    if (foundNetwork) {
      waitForNextTimeslot(inPayloadSize); // Resets x
    } else {
      resetClock(&x);
    }
  }

  /* Ensure that network info is up-to date */
  outPayload.header.slotCount = inPayload.header.slotCount;
  netStat.n = inPayload.header.slotCount;

  /* Annouce yourself */
  while (getClock(&x) <= DELTA_PROC);
#ifdef DEBUG
  Serial.println(F("Announcing self"));
#endif
  delay(GUARD_TIME_BEFORE_TX);
  outPayloadSize = sizeof(payloadHead);
  tx(NULL, 0);
  waitForNextTimeslot(outPayloadSize);

  nextSlot();
  /* Wait for emptyslot while verifying connection */
  bool verified = false;
  int cnt = 0;
  while ((netStat.i != netStat.n - 1 || cnt < 3) && !verified) {
    while (getClock(&x) <= DELTA_PROC); // Wait for user-code
    nextSlot();
    foundNetwork = false;
    while (getClock(&x) <= TIMESLOT_LEN && !foundNetwork) {
      if (rx()) {
        if (inPayload.header.mode == VERIFY) {
          if (inPayload.data[inPayloadSize - sizeof(payloadHead) - 1] == address) {
            verified = true;
#ifdef DEBUG
            Serial.print(F("Join verified by device: "));
            Serial.println(inPayload.header.address, HEX);
#endif
          }
        }
        foundNetwork = true;
      }
    }
    if (foundNetwork) {
      cnt++;
      waitForNextTimeslot(inPayloadSize); // Resets x
    } else {
#ifdef DEBUG
      Serial.println(F("Timeouted while listening waiting to verify"));
#endif
      resetClock(&x);
    }
  }

  /* If verified join, else retry */
  if (verified) {
    /* Ensure that network info is up-to date */
    outPayload.header.slotCount = inPayload.header.slotCount;
    netStat.n = inPayload.header.slotCount;
    netStat.k = netStat.n - 1;
    netStat.n = netStat.n + 1;
    verifiedLast = true; /* To prevent device from verifing in its first transmission */
#ifdef DEBUG
    Serial.print(F("n: "));
    Serial.println(netStat.n);

    Serial.print(F("k: "));
    Serial.println(netStat.k);

    Serial.print(F("test2: Joining network after: "));
    Serial.print(millis() - 5000);
    Serial.println(F(" [ms] Local time"));
#endif
    setPayloadHead(&outPayload, netStat.i,  netStat.n, address, JOIN, sizeof(payloadHead));
    return true;
  } else {
#ifdef DEBUG
    Serial.println(F("Exp-backoff"));
#endif

    if (exponentialBackoffC < EXPONENTIALBACKOFFMAX) {
      exponentialBackoffC++;
    }


    uint32_t framesToWaitBeforeRetrying = random(1 << exponentialBackoffC); // Random returnes 0 to argument - 1
#ifdef DEBUG
    Serial.println(F("Timeouted while listening waiting to verify"));
    Serial.print(F("Chooseing random number in range [0, "));
    Serial.print((1 << exponentialBackoffC) - 1);
    Serial.print(F("] => "));
    Serial.println(framesToWaitBeforeRetrying);
    Serial.print(F("Waiting: "));
    Serial.print((framesToWaitBeforeRetrying * TIMESLOT_LEN * (netStat.n + 1)));
    Serial.println(F("M$"));
#endif

    delay(framesToWaitBeforeRetrying * TIMESLOT_LEN * (netStat.n + 1));
    return false;
  }
}

void connectToNetwork() {
  netStat.i = inPayload.header.currentSlot;
  netStat.n = inPayload.header.slotCount + 1;
  netStat.k = inPayload.header.slotCount - 1; // EmptySlot, is 0-indexed
  setPayloadHead(&outPayload, netStat.i,  netStat.n, address, JOIN, sizeof(payloadHead));
#ifdef DEBUG
  digitalWrite(13, HIGH);
  Serial.println(F("Found Network, joining!!"));
#endif

  bool foundNetwork = false;
  while (netStat.i != netStat.n - 1) {
    nextSlot();
    while (getClock(&x) <= TIMESLOT_LEN && !foundNetwork) {
      if (rx()) {
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
  waitForNextTimeslot(outPayloadSize);

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

#ifdef MULTICONNECT
    if (verifyNext && !verifiedLast) {
      verifiedLast = true;
      outPayload.header.mode = VERIFY;

      /* find last unused byte in payload and put address there, set size again */
      if (usercodeDataSize == PAYLOAD_MAX_SIZE - sizeof(payloadHead)) {
#ifdef DEBUG
        Serial.println(F("All space for usercode is used, removing last byte to fit verify"));
#endif
        usercodeData[PAYLOAD_MAX_SIZE - sizeof(payloadHead) - 1] = addressToVerify;
      } else {
        /* Put address after last byte of usercodeData */
        usercodeData[usercodeDataSize] = addressToVerify;
        usercodeDataSize++;
      }
    } else {
      verifiedLast = false;
    }

#endif
    // Guard time
    while (getClock(&x) <= DELTA_PROC + GUARD_TIME_BEFORE_TX);
    if (usercodeDataSize > 0) {
      outPayloadSize = sizeof(payloadHead) + usercodeDataSize;
      tx(usercodeData, usercodeDataSize);
    } else {
      outPayloadSize = sizeof(payloadHead);
      tx(NULL, 0);
    }
#ifdef MULTICONNECT
    if (verifyNext) {
      usercodeDataSize--;
      outPayload.header.mode = NORMAL;
      verifyNext = false;
    }
#endif
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
        ProtocolMaintenance();
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
#ifdef MULTISTART
      if (netStat.n == 2) {

#ifdef DEBUG
        Serial.println(F("I AM ALONE"));
#endif
        if (random(100) < KILL_CHANCE_IN_PERCENT) {
#ifdef DEBUG
          Serial.println(F("Time to kill myself"));
#endif
          failedCreateAttemptCounter += failedCreateAttemptCounter < 5 ? 1 : 0;
          netStat.n = 0;
          netStat.i = 0;
          netStat.k = 0;
          StartUp();
          outPayload.header.mode = NORMAL;
        }
      }
#endif
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
     To sync with the network we wait untill their timeslot is over.
     Worst case wait time: f(x) = 6.0101 ∗ x + 65.7826
     f(PAYLOAD_MAX_SIZE) = f(16) = 161.9416 [ms]
     f(PAYLOAD_MAX_SIZE) + GUARD_TIME_BEFORE_TX = 191.9416 [ms]
     Which is strictly less than DELTA_COM (200 [ms])

     To calculate the start of the next slot we take the size of the message
     recived, and calculate how much is left of the timeslot.
     Then we hope we are correct and it syncs up.
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

bool rx() {
  digitalWrite(RECEIVE_LED, HIGH);
  uint8_t payloadBuffer[PAYLOAD_MAX_SIZE];
  memset(payloadBuffer, 'a', sizeof(payloadBuffer));
  uint8_t payloadBufferSize = sizeof(payloadBuffer);
  if (rh.recv(payloadBuffer, &payloadBufferSize)) {
#ifdef DEBUG
    rh.printBuffer("Got:", payloadBuffer, payloadBufferSize);
#endif
    readsPayloadFromBuffer(&inPayload, payloadBuffer, payloadBufferSize);
    digitalWrite(RECEIVE_LED, LOW);
    return true;
  } else {
    digitalWrite(RECEIVE_LED, LOW);
    return false;
  }

}

void tx(uint8_t * data, uint8_t dataSize) {
  digitalWrite(SEND_LED, HIGH);
  uint8_t payloadBuffer[PAYLOAD_MAX_SIZE];
  memset(payloadBuffer, 'a', sizeof(payloadBuffer));
  payloadBuffer[0] = outPayload.header.currentSlot;
  payloadBuffer[1] = outPayload.header.slotCount;
  payloadBuffer[2] = outPayload.header.address;
  payloadBuffer[3] = outPayload.header.mode;
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
  digitalWrite(SEND_LED, LOW);
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

void setPayloadHead(struct payload * p, uint8_t curSlot, uint8_t slotCnt, uint8_t addr, uint8_t md, uint8_t sz) {
  p->header.currentSlot = curSlot;
  p->header.slotCount = slotCnt;
  p->header.address = addr;
  p->header.mode = md;
  usercodeDataSize = sz;
}

void nextSlot() {
  netStat.i = (netStat.i + 1) % netStat.n; // This is 0-indexed
  outPayload.header.currentSlot = netStat.i;
#ifdef DEBUG
  Serial.print(F("i = "));
  Serial.println(netStat.i);
#endif
}

void ProtocolMaintenance() {
  if (inPayload.header.slotCount > netStat.n) {
#ifdef DEBUG
    Serial.print(F("New device joined with addr: "));
    Serial.println(inPayload.header.address);

    Serial.print(F("test2: Device "));
    Serial.print(inPayload.header.address, HEX);
    Serial.print(F(" joined after "));
    Serial.print(millis() - 5000);
    Serial.println(F(" [ms] Local time"));
#endif
    outPayload.header.slotCount = inPayload.header.slotCount;
    netStat.n = inPayload.header.slotCount;
  }

  if (inPayload.header.currentSlot != netStat.i) {
#ifdef DEBUG
    Serial.println(F("ProtocolMaintenanceed!"));
#endif
    netStat.i = inPayload.header.currentSlot;
  }

  if (inPayload.header.mode == JOIN) {
    verifyNext = true;
    addressToVerify = inPayload.header.address;
  }
}

#ifdef TEST
void printTask(const char* mode, uint32_t time) {
  Serial.
  Serial.print(mode);
  Serial.print("\t");
  Serial.println(time);
}
#endif

/* Place user code which should be executed once pr. timeslot */
void userCodeRunonce() {
  if (address == SENDER_ADDRESS) {
    pinMode(SENDER_SENSOR_1, INPUT_PULLUP);
    pinMode(SENDER_SENSOR_2, INPUT_PULLUP);
    
    pinMode(2, INPUT_PULLUP);
    pinMode(4, INPUT_PULLUP);
    usercodeData[0] = RECEIVER_1_ADDRESS;
    usercodeData[2] = RECEIVER_2_ADDRESS;
    usercodeDataSize = 5;

    if (netStat.i == netStat.k) {
      usercodeData[1] = LOW;
      usercodeData[3] = LOW;
      usercodeData[4] = 0;
      usercodeData[5] = 0;

    }


    userSensorPool();
  } else {
    pinMode(RECEIVER_OUTPIN, OUTPUT);
    for (int i = 0; i < sizeof(inPayload.data); i++) {
      if (inPayload.data[i] == address) {
#ifdef DEBUG
        Serial.println(F("Turning on LED"));
#endif
#ifdef PULSETOTOGGLE

        if (inPayload.data[i + 1] == HIGH) {
          if (pinState == 0) {
            pinState = 1;
            digitalWrite(RECEIVER_OUTPIN, HIGH);
          } else if (pinState == 2) {
            digitalWrite(RECEIVER_OUTPIN, LOW);
            pinState = 0;
          }
        } else {
          if (pinState == 1) {
            pinState = 2;
          }
        }
#else
        digitalWrite(RECEIVER_OUTPIN, inPayload.data[i + 1] == 1 ? HIGH : LOW);
#endif
      }

      if (inPayload.data[i] == 0x2D) {
        Serial.println("NEXT");
        inPayload.data[i] = 0;
      }

      if (inPayload.data[i] == 0x1D) {
        Serial.println("PREV");
        inPayload.data[i] = 0;
      }
    }
    usercodeDataSize = 0;
  }
}

/* Place user code which should be executed repeatly here */
void userCodeRepeat() {
  userSensorPool();
}

/* Place user code which should be executed repeatly here concurrently while reciving */
void userSensorPool() {
  if (digitalRead(SENDER_SENSOR_1) == LOW) {
    usercodeData[1] = HIGH;
  }

  if (digitalRead(SENDER_SENSOR_2) == LOW) {
    usercodeData[3] = HIGH;
  }

  if (digitalRead(2) == LOW) {
    usercodeData[4] = 0x1D;
  }

  if (digitalRead(4) == LOW) {
    usercodeData[4] = 0x2D;
  }

}

