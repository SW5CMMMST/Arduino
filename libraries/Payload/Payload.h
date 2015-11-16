#pragma once
#include <stdint.h>
#include <Arduino.h>
#define NUM_HEAD_ELEMENTS 3

class Payload { 

public:
    Payload(uint8_t slot, uint8_t slotCnt, uint8_t addr);
    uint8_t* getPayloadAsStream();
    size_t   getStreamLength();
    bool setSlot(uint8_t slot);
    bool setSlotCnt(uint8_t slotCnt);
    bool setAddr(uint8_t addr);
    bool setData(String data);
    bool addData(String data);
    bool addData(char dataChar);
    uint8_t getSlot();
    uint8_t getSlotCnt();
    uint8_t getAddr();
    String getData();
    
private:
    uint8_t _slot;
    uint8_t _slotCnt;
    uint8_t _addr; 
    String _data;  
};    
