/* A simple example using the IRsmallDecoder library
 * for testing remote controls that use the SAMSUNG protocol.
 *
 * Notes:
 *  - For the SAMSUNG32 protocol, define IR_SMALLD_SAMSUNG32 instead of IR_SMALLD_SAMSUNG
 *  - The SAMSUNG protocol follows the standard described in this datasheet:
 *    https://www.mikrocontroller.net/attachment/55409/samsungRCProtokoll.pdf (page 5-30)
 *  - The SAMSUNG32 protocol follows the standard described in this application note:
 *    https://www.scribd.com/doc/283100715/S3F80KB-RemoteController-an-REV000-090108-0
 *
 * In this example, it's assumed that the IR receiver is connected to digital pin 2,
 * and that the pin supports external interrupts.
 * 
 * For more information on the usable pins of each board, see the library documentation at:
 * https://github.com/LuisMiCa/IRsmallDecoder
 * or read the README.pdf file in the extras folder of this library.
 */

#define IR_SMALLD_SAMSUNG     // 1st: Define which protocol to use:
//#define IR_SMALLD_SAMSUNG32 //      IR_SMALLD_SAMSUNG or IR_SMALLD_SAMSUNG32 (not both);
#include <IRsmallDecoder.h>   // 2nd: Include the library;
IRsmallDecoder irDecoder(2);  // 3rd: Create one decoder object with the correct interrupt pin;
irSmallD_t irData;            // 4th: Declare a decoder data structure;

void setup() {
  Serial.begin(115200);
  Serial.println("Waiting for a SAMSUNG remote control IR signal...");
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
