#pragma once
#include <stdint.h>
#include <string> 
#define NUM_HEAD_ELEMENTS 3

class Payload { 

public:
    Payload(uint8_t slot, uint8_t slotCnt, uint8_t addr);
    uint8_t* getPayloadAsStream();
    size_t   getStreamLength();
    bool setSlot(uint8_t slot);
    bool setSlotCnt(uint8_t slotCnt);
    bool setAddr(uint8_t addr);
    bool setData(std::string data);
    bool addData(std::string data);
    uint8_t getSlot();
    uint8_t getSlotCnt();
    uint8_t getAddr();
    std::string getData();
    
private:
    uint8_t _slot;
    uint8_t _slotCnt;
    uint8_t _addr; 
    std::string _data;  
};    
