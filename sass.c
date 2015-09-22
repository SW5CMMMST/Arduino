#include <stdio.h>
#include <stdlib.h>

#define DEBUG

typedef enum {IDLE, INIT, PING, REQUEST, RESPOND, ALERT, OK, ERROR} mode_type;

typedef struct {
    uint16_t sync : 15;
    uint8_t  okay :  1;
    uint8_t  addr;
    uint8_t  mode :  3;
    uint8_t  cntd :  5;
    uint8_t  msga[12];
} payload_type;       

#ifdef DEBUG
const char* mode2string[8] = 
    {"IDLE", "INIT", "PING", "REQUEST", "RESPOND", "ALERT", "OK", "ERROR"};
#endif
                                                                           
int main (int argc, char const *argv[])
{
    payload_type p;
    p.sync = 0x7FFF;
    p.okay = 0x1;
    p.addr = 0xFF;
    p.mode = 0x7;
    p.cntd = 0x1F;
    p.msga[0] = 0xF;
    
    printf("Size of payload: %lu bytes\n", sizeof(p));
   
    printf(" Payload:\n\taddr: %d\n\tokay: %s\n\tsync: %d\n\tmode: %s\n\tcntd: %d\n\tmsga[0]: %d\n\n",
        p.addr, p.okay ? "true" : "false", p.sync, mode2string[p.mode], p.cntd, p.msga[0]);
    return 0;
}
