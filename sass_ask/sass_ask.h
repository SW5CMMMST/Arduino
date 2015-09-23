const uint8_t PAYLOAD_LEN = 16;

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
#define getPayloadString(x, y) sprintf(y ," Payload:\n\taddr: %d\n\tokay: %s\n\tsync: %d\n\tmode: %s\n\tcntd: %d\n\tmsga[0]: %d\n\n", x.addr, x.okay ? "true" : "false", x.sync, mode2string[x.mode], x.cntd, x.msga[0]);
#endif