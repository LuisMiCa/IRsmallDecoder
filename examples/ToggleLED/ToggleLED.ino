/* Toggle built-in LED with an IR remote control
 *
 * How to use this sketch: 
 *  - Uncomment the #define for the desired protocol;
 *    leave the others commented out (only one is allowed);
 *  - Upload to the Arduino board, connect the IR receiver and
 *  - Press any key on the remote to toggle the built-in LED
 *
 * In this example it's assumed that the board has a built-in LED and that the IR receiver is 
 * connected to digital pin 2, which must be usable for external interrupts.
 * 
 * For more information on the usable pins of each board, see the library documentation at:
 * https://github.com/LuisMiCa/IRsmallDecoder
 * or read the README.pdf file in the extras folder of this library.
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

IRsmallDecoder irDecoder(2);  // Assuming that the IR receiver is connected to digital pin 2
int ledState = LOW;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (irDecoder.dataAvailable()) {
    ledState = (ledState == LOW) ? HIGH : LOW;  // Toggle state with a conditional operator
    digitalWrite(LED_BUILTIN, ledState);
  }
}