#ifndef Addr_h
#define Addr_h

#include <EEPROM.h> 
#include <inttypes.h>
#define EEPROM_LOCATION 0x00

class Addr
{
public:
    Addr(void);
    uint8_t get();
    bool    set(uint8_t addr);

private:
    uint8_t _deviceAddr;
    uint8_t _read();
    bool    _isAddrSet();
};

#endif