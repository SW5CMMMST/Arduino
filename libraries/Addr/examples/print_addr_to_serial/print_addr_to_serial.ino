#include <EEPROM.h> //Needed to compile
#include <Addr.h>

Addr a; // Initiates the Addr object

void setup(void)
{
	Serial.begin(9600); 						// Setup serial connection
	Serial.print("Device Addr from EEPROM: ");	// 
	Serial.println(a.get(), HEX);   			// Get the Address from a and print in hex
}

void loop(void) { /* Empty loop */ }

