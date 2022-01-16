/* Toggle built-in LED with an IR remote control
 *
 * How to use this sketch: 
 *  - Uncomment the #define for the desired protocol,
 *    leave the others as comments (only one is allowed).
 *  - Upload to the Arduino board, connect the IR receiver and
 *  - press any key on the remote to toggle the builtin LED
 *
 * In this example it's assumed that the board has a builtin LED and the IR receiver is 
 * connected to digital pin 2, which must be usable for external interrupts.
 * 
 * For more information on the boards' usable pins, see the library documentation at:
 * https://github.com/LuisMiCa/IRsmallDecoder
 * or the README.pdf file in the extras folder of this library. 
 */

#define IR_SMALLD_NEC
//#define IR_SMALLD_NECx
//#define IR_SMALLD_RC5
//#define IR_SMALLD_SIRC12
//#define IR_SMALLD_SIRC15
//#define IR_SMALLD_SIRC20
//#define IR_SMALLD_SIRC
//#define IR_SMALLD_SAMSUNG
//#define IR_SMALLD_SAMSUNG32

#include <IRsmallDecoder.h>

IRsmallDecoder irDecoder(2);  //assuming that the IR receiver is connected to digital pin 2
int ledState = LOW;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (irDecoder.dataAvailable()) {
    ledState = (ledState == LOW) ? HIGH : LOW;  //toggle with conditional operator
    digitalWrite(LED_BUILTIN, ledState);
  }
}
