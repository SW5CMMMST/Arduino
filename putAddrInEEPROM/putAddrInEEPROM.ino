#include <EEPROM.h>
#define DEVICE_ADDR 0x89

void setup()
{
  //Clear the EEPROM
  for ( int i = 0 ; i < EEPROM.length() ; i++ )
    EEPROM.write(i, 0);

  EEPROM.write(0, DEVICE_ADDR);
  digitalWrite(13, HIGH);
}

void loop(){ /** Empty loop. **/ }
