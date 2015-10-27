#include <EEPROM.h>
#include <PJON_ASK.h>

#define RX_PIN 11
#define TX_PIN 12
#define SLOTLENGTH 300  // Slot length in miliseconds

typedef struct {
    uint8_t  slot;      // Current slot also doubles as count down to the empty slot
    uint8_t  slotCount; // Number of slots in the frame
    uint8_t  addr;      // Unique address of the sender
    uint8_t  msga[12];  // Other content
} payload_type; 

// Initialize PJON library
PJON_ASK network(RX_PIN, TX_PIN, EEPROM.read(0x00));
// Message buffers
payload_type out_payload;
payload_type in_payload;

int curSlot;
int mySlot;
int slotCount;
uint8_t addr;

// Copies the recieved message into the buffer
static void receiver_function(uint8_t len, uint8_t *payload) {
  memcpy(&in_payload, payload, len);
}

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
  network.send(BROADCAST, (char*)&out_payload, sizeof(out_payload));
  // Wait for the empty slot
  delay((in_payload.slot - 1)*SLOTLENGTH);
  // Send the package
  network.update();
}

void setup() {
  // Read the hardcoded unique address from EEPROM
  addr = EEPROM.read(0x00);
  // Setup serial communication via usb
  Serial.begin(9600);
  // Set the reciver function to be called by the library
  network.set_receiver(receiver_function);
  // Look for 20 seconds for an existing network
  int response = network.receive(20000);
  if(response == ACK){
    // Network exsists connect to it
    connectToNetwork();
  }
  else{
    // Network doesn't exists create a new
    setupNetwork();
  }
}

void loop() {
  if(curSlot == mySlot){
    long time = millis();
    network.send(BROADCAST, (char*)&out_payload, sizeof(out_payload));
    network.update();
    delay(SLOTLENGTH-(millis()-time));//wait to make sure full sloth length is used
    //not optimal it uses delay.
  } else {
    network.receive(SLOTLENGTH*1000L);
  }
  curSlot--;
  if(curSlot == -1){
    curSlot = slotCount - 1;
  }
}
