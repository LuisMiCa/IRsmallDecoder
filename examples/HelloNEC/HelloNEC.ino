/* A simple example using the IRsmallDecoder library
 * for testing remote controls that use the NEC protocol.
 *
 * Note: For the NEC extended protocol, define IR_SMALLD_NECx instead of IR_SMALLD_NEC 
 *
 * In this example it's assumed that the IR receiver is connected to digital pin 2, 
 * and that the pin supports external interrupts.
 * 
 * For more information on the usable pins of each board, see the library documentation at:
 * https://github.com/LuisMiCa/IRsmallDecoder
 * or read the README.pdf file in the extras folder of this library.
 */

#define IR_SMALLD_NEC         // 1st: Define which protocol to use:
//#define IR_SMALLD_NECx      //      IR_SMALLD_NEC or IR_SMALLD_NECx, (not both);
#include <IRsmallDecoder.h>   // 2nd: Include the library;
IRsmallDecoder irDecoder(2);  // 3rd: Create one decoder object with the correct digital pin;
irSmallD_t irData;            // 4th: Declare a decoder data structure;

void setup() {
  Serial.begin(115200);
  Serial.println("Waiting for a NEC remote control IR signal...");
  Serial.println("held\t addr\t cmd");
}

void loop() {
  if (irDecoder.dataAvailable(irData)) {  // 5th: If the decoder has new data available,
    Serial.print(irData.keyHeld);         // 6th: do something with that data...
    Serial.print("\t ");
    Serial.print(irData.addr, HEX);
    Serial.print("\t ");
    Serial.println(irData.cmd, HEX);
  }
}