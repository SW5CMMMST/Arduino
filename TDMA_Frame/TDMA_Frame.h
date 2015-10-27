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
#define MAX_FRAME_LEN 100 
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
        //uint8_t priority;
    };     
public:
    Frame(void);
    Frame(unsigned int selfTime);
    bool setSelfTime(unsigned int selfTime);
    bool addDevice(uint8_t addr, unsigned long timeslot_len);
    bool removeDevice(uint8_t addr);
    unsigned long getWaitTime();
    
private:
    Timeslot _timeslots[MAX_FRAME_LEN];  
    unsigned long _totalFrameTime = 0;
    unsigned int _selfTime = 0; 
    Timeslot* _next_free;
    bool _calcTotalFrameTime();
      
};
#endif