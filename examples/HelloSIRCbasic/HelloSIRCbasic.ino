/* A simple example using the IRsmallDecoder library for testing remote controls
 * that use the Sony SIRC12, SIRC15 or SIRC20 protocols. 
 *
 * Notes: 
 *  - Unlike IR_SMALLD_SIRC, IR_SMALLD_SIRC12/15/20 do not have the keyHeld data member;
 *  - IR_SMALLD_SIRC20 has an additional data member (ext);
 *  - Each keypress on a SIRC remote, usually sends three signal frames. The two additional 
 *    signal repetitions are ignored in the IR_SMALLD_SIRC, but not in the IR_SMALLD_SIRC12/15/20;
 *  - IR_SMALLD_SIRC12 will receive signals from SIRC15 and SIRC20, but the codes will not be correct;
 *  - In a similar way, IR_SMALLD_SIRC15 will receive signals from SIRC20, but not from SIRC12.
 *	
 * In this example, it's assumed that the IR receiver is connected to digital pin 2,
 * and that the pin supports external interrupts.
 * 
 * For more information on the usable pins of each board, see the library documentation at:
 * https://github.com/LuisMiCa/IRsmallDecoder
 * or read the README.pdf file in the extras folder of this library.
 */


#define IR_SMALLD_SIRC12      // 1st: Define which protocol to use
//#define IR_SMALLD_SIRC15    //      (only one of these three can be defined at a time);
//#define IR_SMALLD_SIRC20

#include <IRsmallDecoder.h>   // 2nd: Include the library;
IRsmallDecoder irDecoder(2);  // 3rd: Create one decoder object with the correct digital pin;
irSmallD_t irData;            // 4th: Declare a decoder data structure;

void setup() {
  Serial.begin(115200);
  #if defined(IR_SMALLD_SIRC12) 
    Serial.println("Waiting for a SIRC12 remote control IR signal...");
    Serial.println("Addr\tCmd");
  #elif defined(IR_SMALLD_SIRC15)
    Serial.println("Waiting for a SIRC15 remote control IR signal...");
    Serial.println("Addr\tCmd");
  #elif defined(IR_SMALLD_SIRC20)
    Serial.println("Waiting for a SIRC20 remote control IR signal...");
    Serial.println("Addr\tCmd \tExt");
  #endif
}

void loop() {
  if (irDecoder.dataAvailable(irData)) {  // 5th: If the decoder has new data available,
    Serial.print(irData.addr, HEX);       // 6th: do something with that data...
    Serial.print("   \t");
    #if defined(IR_SMALLD_SIRC12) || defined(IR_SMALLD_SIRC15)
      Serial.println(irData.cmd, HEX);
    #else
      Serial.print(irData.cmd, HEX);
      Serial.print("  \t");
      Serial.println(irData.ext, HEX);
    #endif
  }
}
