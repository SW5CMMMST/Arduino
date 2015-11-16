#include "Payload.h"               
 
Payload::Payload(uint8_t slot, uint8_t slotCnt, uint8_t addr){
    this->_slot = slot;
    this->_slotCnt = slotCnt;
    this->_addr = addr;
}  

uint8_t* Payload::getPayloadAsStream(){
    return (uint8_t*)this;
}   

size_t Payload::getStreamLength(){
    return sizeof(uint8_t) * NUM_HEAD_ELEMENTS + this->_data.length();
}    

bool Payload::setSlot(uint8_t slot){
    //TODO validate
    this->_slot = slot;
    return true;
}   

bool Payload::setSlotCnt(uint8_t slotCnt){
    //TODO validate
    this->_slotCnt = slotCnt;
    return true;
}   

bool Payload::setAddr(uint8_t addr){
    //TODO validate
    this->_addr = addr;
    return true;
}   

bool Payload::setData(String data){
    //TODO validate
    this->_data = data;
    return true;
}

bool Payload::addData(String data){
    //TODO validate
    this->_data += data;
    return true;
}

bool Payload::addData(char dataChar){
    //TODO validate
    this->_data += dataChar;
    return true;
}        

uint8_t Payload::getSlot(){
    return this->_slot;
}

uint8_t Payload::getSlotCnt(){
    return this->_slotCnt;
}

uint8_t Payload::getAddr(){
    return this->_addr;
}         

String Payload::getData(){
    return this->_data;
}     