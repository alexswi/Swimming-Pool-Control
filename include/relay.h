#ifndef relay_h
#define relay_h
#include <Arduino.h>

extern bool pump1State;
extern bool pump2State;

#define RELAY1 21
#define RELAY2 22


void PumpSet(u_char pump,bool on);


#endif // relay_h