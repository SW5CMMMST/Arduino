/* * * * * * * * * * * * * * * * * * * *
 * Mathias Sass Michno     26 OCT 2016 *
 *                                     *
 * Aalborg Universitet       SW503-E15 *
 * * * * * * * * * * * * * * * * * * * *
 * Arduino library for implementing    *
 * TDMA frames with timeslots          *
 * * * * * * * * * * * * * * * * * * * */
#ifndef TDMA_Frame
#define TDMA_Frame
#define MAX_FRAME_LEN 32
#define EMPTY_SLOT_LEN 200 
#include <stdint.h>

class Frame { 
    class Timeslot {
    public:
        Timeslot(void); 
        long getLength(); 
        bool setLength(unsigned long len);
        uint8_t getDeviceAddr();
        bool setDeviceAddr(uint8_t addr);
    private:    
        unsigned long _length;
        uint8_t _device_addr;
    };     
public:
    Frame(void);
    Frame(unsigned int selfTime);
    bool setSelfTime(unsigned int selfTime);
    bool addDevice(uint8_t addr, unsigned long timeslot_len);
    bool removeDevice(uint8_t addr);
    long getWaitTime(); 
    void debugN1(uint8_t n);
    
private:
    Timeslot _timeslots[MAX_FRAME_LEN];  
    unsigned long _totalFrameTime = 0;
    unsigned int _selfTime = 0; 
    uint8_t _next_free_index;
    bool _calcTotalFrameTime();
    bool _maintainEmptySlot(bool inc);
      
};
#endif