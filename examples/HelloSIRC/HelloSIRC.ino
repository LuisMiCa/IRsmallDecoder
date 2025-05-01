/* A simple example using the IRsmallDecoder library
 * for testing remote controls that use the Sony SIRC protocol.
 *
 * 
 * This example uses an advanced implementation of the SIRC decoder, which
 * takes advantage of the fact that most SIRC remotes send three signal
 * frames every time one key is pressed. That allows the decoder to:
 *  - identify the number of bits used in the protocol (12, 15, or 20);      
 *  - ignore the two additional repetitions; 
 *  - use the repetitions to check for errors;
 *  - determine if a key is being held. 
 *
 * If the decoder detects a 12- or 15-bit signal, the ext data member is set to 0.
 * If it detects a 20-bit signal, it saves the extended data in the ext member variable.
 * 
 * In this example, it's assumed that the IR receiver is connected to digital pin 2,
 * and that the pin supports external interrupts.
 * 
 * For more information on the usable pins of each board, see the library documentation at:
 * https://github.com/LuisMiCa/IRsmallDecoder
 * or read the README.pdf file in the extras folder of this library.
 */

#define IR_SMALLD_SIRC        // 1st: Define which protocol to use;
#include <IRsmallDecoder.h>   // 2nd: Include the library;
IRsmallDecoder irDecoder(2);  // 3rd: Create one decoder object with the correct digital pin;
irSmallD_t irData;            // 4th: Declare a decoder data structure;

void setup() {
  Serial.begin(115200);
  Serial.println("Waiting for a SIRC remote control IR signal...");
  Serial.println("held\taddr\tcmd \tExt");
}

void loop() {
  if (irDecoder.dataAvailable(irData)) {  // 5th: If the decoder has new data available,
    Serial.print(irData.keyHeld);         // 6th: do something with that data...
    Serial.print("   \t");
    Serial.print(irData.addr, HEX);
    Serial.print("   \t");
    Serial.print(irData.cmd, HEX);
    Serial.print("   \t");
    Serial.println(irData.ext, HEX);
  }
}