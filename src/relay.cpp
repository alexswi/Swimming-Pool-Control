#include "relay.h"

void PumpSet(u_char pump,bool on){
  if(pump==1){
    pump1State = on; 
    digitalWrite(RELAY1,!on);
  }
  if(pump==2){
    pump2State = on; 
    digitalWrite(RELAY2,!on);
  }
}