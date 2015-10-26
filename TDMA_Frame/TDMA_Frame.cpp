#include "TDMA_Frame.h"

Frame::Frame(void){
    next_free = &timeslots[0];
}

bool Frame::addDevice(uint8_t addr, long timeslot_len) { 
    if(addr == 0x0 || timeslot_len <= 0)
        return false;
    next_free->setDeviceAddr(addr);
    next_free->setLength(timeslot_len);  
    next_free++; 
    waitTime += timeslot_len;
    while(next_free->getDeviceAddr() != 0){
        next_free++;
    } 
    return true;
} 

//Need some way of reorganizing the frame (TL;DR probably don't use this function ATM)
bool Frame::removeDevice(uint8_t addr) {
    for(uint8_t i = 0; i <= MAX_FRAME_LEN-1; ++i){
        if(timeslots[i].getDeviceAddr() == addr){
            waitTime += timeslots[i].getLength();
            timeslots[i].setLength(0);
            timeslots[i].setDeviceAddr(0x0); 
            next_free = &timeslots[i];
            return true;
        }        
    }
    return false;
}

long Frame::getWaitTime() {
     if(waitTime == 0 /*|| something bad has happened*/) 
         calcWaitTime();
     return waitTime;
} 

//Should never be needed, but is here just in case...
bool Frame::calcWaitTime() {
    waitTime = 0;
    for(uint8_t i = 0; i <= MAX_FRAME_LEN-1; ++i){
        if(timeslots[i].getDeviceAddr() != 0x0){
            waitTime += timeslots[i].getLength();
        }        
    }
    return true; 
}         

Timeslot::Timeslot(void) {} 

long Timeslot::getLength() {
    return this->length;
} 

bool Timeslot::setLength(long len) {
    if(len <= 0)
        return false;
    this->length = len;
    return true;
}

bool Timeslot::setDeviceAddr(uint8_t addr) { 
    if(addr == 0x0)
        return false;
    this->device_addr = addr;
}

uint8_t Timeslot::getDeviceAddr() {
    return this->device_addr;
}                         