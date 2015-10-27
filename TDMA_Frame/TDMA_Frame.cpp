#include "TDMA_Frame.h"

Frame::Frame(void){
    _next_free = &_timeslots[0];
}

Frame::Frame(unsigned int selfTime) {
    Frame();
    _selfTime = selfTime;
}
 
bool Frame::setSelfTime(unsigned int selfTime) {
    _selfTime = selfTime;
    return true;
}   

bool Frame::addDevice(uint8_t addr, unsigned long timeslot_len) { 
    if(addr == 0x0 || timeslot_len <= 0)
        return false;
    _next_free->setDeviceAddr(addr);
    _next_free->setLength(timeslot_len);  
    _next_free++; 
    _totalFrameTime += timeslot_len;
    while(_next_free->getDeviceAddr() != 0){
        _next_free++;
    } 
    return true;
} 

//Need some way of reorganizing the frame (TL;DR probably don't use this function ATM)
bool Frame::removeDevice(uint8_t addr) {
    for(uint8_t i = 0; i <= MAX_FRAME_LEN-1; ++i){
        if(_timeslots[i].getDeviceAddr() == addr){
            _totalFrameTime += _timeslots[i].getLength();
            _timeslots[i].setLength(0);
            _timeslots[i].setDeviceAddr(0x0); 
            _next_free = &_timeslots[i];
            return true;
        }        
    }
    return false;
}

unsigned long Frame::getWaitTime() {
     if(_totalFrameTime == 0 /*|| something bad has happened*/) 
         _calcTotalFrameTime();
     return _totalFrameTime - _selfTime;
} 

//Should never be needed, but is here just in case...
bool Frame::_calcTotalFrameTime() {
    _totalFrameTime = 0;
    for(uint8_t i = 0; i <= MAX_FRAME_LEN-1; ++i){
        if(_timeslots[i].getDeviceAddr() != 0x0){
            _totalFrameTime += _timeslots[i].getLength();
        }        
    }
    return true; 
}         

Frame::Timeslot::Timeslot(void) {} 

long Frame::Timeslot::getLength() {
    return this->_length;
} 

bool Frame::Timeslot::setLength(unsigned long len) {
    if(len <= 0)
        return false;
    this->_length = len;
    return true;
}

bool Frame::Timeslot::setDeviceAddr(uint8_t addr) { 
    if(addr == 0x0)
        return false;
    this->_device_addr = addr;
}

uint8_t Frame::Timeslot::getDeviceAddr() {
    return this->_device_addr;
}                         