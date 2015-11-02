#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile

#include <EEPROM.h>
// #include <PJON_ASK.h>

#define RX_PIN 11
#define TX_PIN 12
#define SLOTLENGTH 300  // Slot length in miliseconds
#define MAX_RECEIVE_TIME SLOTLENGTH - 50

typedef struct {
    uint8_t  slot;      // Current slot also doubles as count down to the empty slot
    uint8_t  slotCount; // Number of slots in the frame
    uint8_t  addr;      // Unique address of the sender
    uint8_t  msga[5];   // Other content
} payload_type; 

// Initialize PJON library
//PJON_ASK network(RX_PIN, TX_PIN, EEPROM.read(0x00));

RH_ASK driver;
// Message buffers
payload_type out_payload;
payload_type in_payload;

int curSlot;
int mySlot;
int slotCount;
uint8_t addr;

/* Copies the recieved message into the buffer
void receiver_function(uint8_t len, uint8_t *payload) {
  memcpy(&in_payload, payload, len);
  Serial.print("Received Message: ");
  char buff[16];
  for(int i = 0; i < len; i++){
    sprintf(buff, "0x%.2X ", payload[i]);
    Serial.print(buff);
  }
  Serial.println();
} */

// Initializes the payload to use the current data
void makePayload(){
  out_payload.addr = addr;
  out_payload.slot = mySlot;
  out_payload.slotCount = slotCount;
}

// Setup variables needed to create a new network
void setupNetwork(){
  mySlot = 1;
  curSlot = 1;
  slotCount = 2;
  makePayload();
}

// Use the empty slot to insert this device into the network
void connectToNetwork(){
  // Take the variable data from the recieved message
  mySlot = in_payload.slotCount;
  slotCount = in_payload.slotCount + 1;
  curSlot = mySlot;
  // Setup payload and make it ready for sending
  makePayload();
  //network.send(BROADCAST, (char*)&out_payload, sizeof(out_payload));
  
  // Wait for the empty slot
  delay((in_payload.slot - 1)*SLOTLENGTH);
  driver.send((uint8_t*)&out_payload, sizeof(out_payload));
  driver.waitPacketSent();
  // Send the package
  //network.update();
}

void setup() {
  // Read the hardcoded unique address from EEPROM
  addr = EEPROM.read(0x00);
  // Setup serial communication via usb
  Serial.begin(9600);
  // Setup in and output 
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(13, OUTPUT);
  // Set the reciver function to be called by the library
  //network.set_receiver(receiver_function);
  // Look for 20 seconds for an existing network
  Serial.println("Looking for network...");
  //int response = network.receive(5000000L);
  long timer = millis();
  int response = 0;
  payload_type buf;
  while(timer + 10000 > millis()){
    uint8_t len = sizeof(payload_type);
    if(driver.recv((uint8_t*)&buf, &len)){
      response = 1;
      //receiver_function(len, &buf);
      break;
    }
  }
  if(response == 1){
    Serial.println("Network found!");
    // Network exsists connect to it
    connectToNetwork();
  } else {
    Serial.println("No network found! Creating new network.");
    // Network doesn't exists create a new
    setupNetwork();
  }
  Serial.print("Setup done. My slot: ");
  Serial.println(mySlot);
}

void loop() {
  Serial.print("Slot: ");
  Serial.println(curSlot);
  long time = millis();
  if(curSlot == mySlot){
    digitalWrite(13, HIGH);
    // Set payload for transmission
    //network.send(BROADCAST, (char*)&out_payload, sizeof(out_payload));
    // Send the package
    //network.update();

    driver.send((uint8_t*)&out_payload, sizeof(out_payload));
    driver.waitPacketSent();
  } else {
    // Recieve 
    payload_type buf;
    while(time + MAX_RECEIVE_TIME > millis()){
      uint8_t len = sizeof(payload_type);
      if(driver.recv((uint8_t*)&buf, &len)){
        //receiver_function(len, buf);
        break;
      }
    }
  }
  // Wait until next timeslot
  int timeleft = SLOTLENGTH-(millis()-time);
  if(timeleft<0) delay(timeleft);
  // Update the timeslot
  curSlot--;
  if(curSlot == -1){
    curSlot = slotCount - 1;
  }
  digitalWrite(13, LOW);
}
