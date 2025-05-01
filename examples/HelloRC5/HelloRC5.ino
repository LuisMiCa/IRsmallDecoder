/* A simple example using the IRsmallDecoder library
 * for testing remote controls that use the RC5 protocol.
 *
 * Note: This works with both RC5 and RC5-extended protocols.
 *
 * In this example, it's assumed that: the IR receiver is connected to digital pin 2,
 * the pin supports external interrupts, and it's compatible with CHANGE mode.
 * 
 * For more information on the usable pins of each board, see the library documentation at:
 * https://github.com/LuisMiCa/IRsmallDecoder
 * or read the README.pdf file in the extras folder of this library.
 */

#define IR_SMALLD_RC5         // 1st: Define the protocol to use;
#include <IRsmallDecoder.h>   // 2nd: Include the library;
IRsmallDecoder irDecoder(2);  // 3rd: Create one decoder object using the correct digital pin;
irSmallD_t irData;            // 4th: Declare a decoder data structure;

void setup() {
  Serial.begin(115200);
  Serial.println("Waiting for a RC5 remote control IR signal...");
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