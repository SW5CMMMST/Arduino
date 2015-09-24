#include <stdint.h>
#include <RH_ASK.h>

extern "C"{

typedef enum {IDLE, INIT, PING, REQUEST, RESPOND, ALERT, OK, ERROR} mode_type;

typedef struct {
    uint16_t sync : 15;
    uint8_t  okay :  1;
    uint8_t  addr;
    uint8_t  mode :  3;
    uint8_t  cntd :  5;
    uint8_t  msga[12];
} payload_type;       

bool sendPayload(RH_ASK, payload_type);
bool recvPayload(RH_ASK, payload_type *);
void makePayloadString(payload_type, char *);


}