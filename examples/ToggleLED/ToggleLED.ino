/* Built-in LED toggle with an IR remote control:
 * Uncomment the #define for the desired protocol,
 * leave the others as comments (only one is allowed).
 * Upload to the Arduino board, connect the IR sensor and
 * press any key on the remote to toggle the builtin LED
 */

#define IR_SMALLD_NEC
//#define IR_SMALLD_NECx
//#define IR_SMALLD_RC5
//#define IR_SMALLD_SIRC12
//#define IR_SMALLD_SIRC15
//#define IR_SMALLD_SIRC20
//#define IR_SMALLD_SIRC

#include <IRsmallDecoder.h>

IRsmallDecoder irDecoder(2); 
irSmallD_t irData;
int ledState=LOW; 

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if(irDecoder.dataAvailable(irData)){       
    ledState = (ledState == LOW)? HIGH : LOW; //toggle with conditional operator
    digitalWrite(LED_BUILTIN, ledState);
  }
}
