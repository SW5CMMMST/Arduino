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
#define MAX_FRAME_LEN 256 
#include <stdint.h>

class Frame { 
    class Timeslot {
    public:
        Timeslot(void); 
        long getLength(); 
        bool setLength(long len);
        uint8_t getDeviceAddr();
        bool setDeviceAddr(uint8_t addr);
    private:    
        long length;
        uint8_t device_addr;
        //uint8_t priority;
    };     
public:
    Frame(void);
    bool addDevice(uint8_t addr, long timeslot_len);
    bool removeDevice(uint8_t addr);
    long getWaitTime();
    
private:
    Timeslot timeslots[MAX_FRAME_LEN];  
    long waitTime = 0; 
    Timeslot* next_free;
    bool calcWaitTime();
      
};

class Timeslot {
public:
    Timeslot(void); 
    long getLength(); 
    bool setLength(long len);
    uint8_t getDeviceAddr();
    bool setDeviceAddr(uint8_t addr);
private:    
    long length;
    uint8_t device_addr;
    //uint8_t priority;
};
#endif