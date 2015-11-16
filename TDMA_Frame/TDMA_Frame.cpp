#include "TDMA_Frame.h" 
#include <Arduino.h>

Frame::Frame(void){ 
    this->_next_free_index = 0;
}

Frame::Frame(unsigned int selfTime) {
    Frame();
    this->_selfTime = selfTime;
}
        
void Frame::debugN1(uint8_t n) {  
    Serial.print(F("Addr of "));
    Serial.print(n);  
    Serial.print(F(" timeslot: "));
    Serial.print(this->_timeslots[n].getDeviceAddr(), HEX);  
    Serial.print(F(" | slot length: "));
    Serial.println(this->_timeslots[n].getLength(), DEC);  
} 

bool Frame::setSelfTime(unsigned int selfTime) {
    this->_selfTime = selfTime;
    return true;
}   

bool Frame::addDevice(uint8_t addr, unsigned long timeslot_len) { 
    //Check if free points to emptyslot
    if(this->_timeslots[_next_free_index].getDeviceAddr() == 0x0 && this->_timeslots[_next_free_index].getLength() != 0) {
        this->_timeslots[_next_free_index].setDeviceAddr(addr);
        this->_timeslots[_next_free_index].setLength(timeslot_len);
        this->_next_free_index++;
        this->_totalFrameTime += timeslot_len;
        this->_maintainEmptySlot(true);  
        
    }
    
     
    Serial.print(F("Addr sat in new dev: ")); 
    Serial.println(this->_timeslots[_next_free_index].getDeviceAddr());
    
    Serial.print(F("Len sat in new dev: ")); 
    Serial.println( this->_timeslots[_next_free_index].getLength()); 
     
    Serial.print(F("New total frame time: "));
    Serial.println(this->_totalFrameTime);
    while( this->_timeslots[_next_free_index].getLength() != 0){
        this->_next_free_index++;
    }
    
    return true;                                                                        
} 

//Need some way of reorganizing the frame (TL;DR probably don't use this function ATM)
bool Frame::removeDevice(uint8_t addr) {
    for(uint8_t i = 0; i <= MAX_FRAME_LEN-1; ++i){
        if(this->_timeslots[i].getDeviceAddr() == addr){
            this->_totalFrameTime -= this->_timeslots[i].getLength();
            this->_timeslots[i].setLength(0);
            this->_timeslots[i].setDeviceAddr(0x0); 
            this->_next_free_index = i;
            this->_maintainEmptySlot(false); 
            return true;
        }        
    }
    return false;
}

bool Frame::_maintainEmptySlot(bool inc){
    if(inc) {
        this->addDevice(0x0, EMPTY_SLOT_LEN);
        this->_totalFrameTime += EMPTY_SLOT_LEN;
        this->_next_free_index--;
        return true;
    }
    _next_free_index--;
    if(this->_timeslots[_next_free_index].getDeviceAddr() == 0x0) {
        this->_timeslots[_next_free_index].setLength(0);
        this->_totalFrameTime -= EMPTY_SLOT_LEN;
        return true;
    } else {
        _next_free_index++;
        Serial.println(F("Something went terribly wrong..."));
        return false;
    }
    
}

long Frame::getWaitTime() {
     if(this->_totalFrameTime == 0 /*|| something bad has happened*/) 
         _calcTotalFrameTime();
     return this->_totalFrameTime - this->_selfTime;
} 

//Should never be needed, but is here just in case...
bool Frame::_calcTotalFrameTime() {
    this->_totalFrameTime = 0;
    for(uint8_t i = 0; i <= MAX_FRAME_LEN-1; ++i){
        this->_totalFrameTime += this->_timeslots[i].getLength();     
    }
    return true; 
}         

Frame::Timeslot::Timeslot(void) {} 

long Frame::Timeslot::getLength() {
    return this->_length;
} 

bool Frame::Timeslot::setLength(unsigned long len) {
    this->_length = len;
    return true;
}

bool Frame::Timeslot::setDeviceAddr(uint8_t addr) { 
    this->_device_addr = addr;   
    return true;
}

uint8_t Frame::Timeslot::getDeviceAddr() {
    return this->_device_addr;
}                         