#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

RH_ASK driver;

// Globals for transmitter
int count = 0;
const int led_pin = 13;
const int transmit_pin = 12;

void setup() {
  Serial.begin(9600);

  // Transmitter
  if (!driver.init())
         Serial.println("init failed");
  pinMode(led_pin, OUTPUT);
  Serial.println("Ready to send!");
  //StartSignal();
}

void loop(){
  for(int i = 0; i < 100; i++) {
    digitalWrite(led_pin, HIGH);
    char buf[16] = "";
    //itoa(count, buf, 10);
    for(int j = strlen(buf); j < 16; j++) {
      buf[j] = 'a' + j;
    }
    buf[15] = '\0';
    driver.send((uint8_t *)buf, strlen(buf));
    driver.waitPacketSent();
    
    Serial.print(buf);
    
    Serial.println(count);
    count++;
    digitalWrite(led_pin, LOW);
    delay(10);
  }

  while(true);
}

