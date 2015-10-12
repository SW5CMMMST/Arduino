#include <stdio.h>
#include <sass_ask.h>

const uint8_t PAYLOAD_SIZE_CONST = 16;

extern "C"{

const char* mode2string[8] = 
    {"IDLE", "INIT", "PING", "REQUEST", "RESPOND", "ALERT", "OK", "ERROR"};

void makePayloadString(payload_type x, char *y) {
	sprintf(y ,
		"Payload:\n\taddr: %d\n\tokay: %s\n\tsync: %d\n\tmode: %s\n\tcntd: %d\n\tmsga: %s\n\n", 
		x.addr, 
		x.okay ? "true" : "false", 
		x.sync, 
		mode2string[x.mode], 
		x.cntd,
		(char*)x.msga);
}

}