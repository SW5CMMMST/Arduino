// ask_receiver.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to receive messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) receiver with an Rx-B1 module

#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile

RH_ASK driver;

void setup()
{
  Serial.begin(9600);  // Debugging only
  if (!driver.init())
    Serial.println("init failed");

  Serial.println("fak");
}

int prev = -1;
int totalMissed = 0;
int totalRecived = 0;

void loop()
{
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  if (driver.recv(buf, &buflen)) {
    int thisId = atoi((char *) buf);

    // Only used to start
    if (prev == -1) {
      prev = thisId;
    }

    // Transmisster reset
    if (prev > thisId)
    {
      Serial.println("Reset!!!");
      totalMissed = 0;
      totalRecived = 0;
    } else {
      // Print each number missed
      for (int i = prev; i < thisId - 1; i++) {
        totalMissed++;
        totalRecived++;
        Serial.print("Missed: "); 
        Serial.print(i + 1);
        Serial.print(" (");
        Serial.print(totalMissed);
        Serial.print("/");
        Serial.print(totalRecived);
        Serial.print(" = ");
        Serial.print(((double) totalMissed / (double)( totalRecived + totalMissed)) * 100);
        Serial.println("%)");
      }
    }

    Serial.print("Recived: ");
    Serial.print(thisId);
    Serial.print(" (");
    Serial.print(totalMissed);
    Serial.print("/");
    Serial.print(totalRecived);
    Serial.print(" = ");
    Serial.print(((double) totalMissed / (double)( totalRecived + totalMissed)) * 100);
    Serial.println("%)");
    totalRecived++;
    prev = thisId;
  }
}

