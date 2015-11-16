#include "Addr.h"

Addr::Addr(void){
    this->_deviceAddr = _read();
}

uint8_t Addr::get(){
    return this->_deviceAddr;
} 

bool Addr::set(uint8_t addr){ 
    //TODO check if addr is unique and valid!
    EEPROM.write(EEPROM_LOCATION, addr); 
    this->_deviceAddr = addr;
    return true;
}

bool Addr::_isAddrSet(){
    if(EEPROM.read(EEPROM_LOCATION) != 0x00)
        return true;
    return false; //Add more check
}   

uint8_t Addr::_read(){
    if(_isAddrSet())
        return EEPROM.read(EEPROM_LOCATION);
    return 0x00; //Return invalid address if no address set in EEPROM
} 