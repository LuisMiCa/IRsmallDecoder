/* keyHeld with NEC
 *  
 * This example is part of the IRsmallDecoder library and is intended to demonstrate 
 * a possible usage for the keyHeld data member.
 *  
 * How to use this sketch: 
 *  - Connect the IR receiver (see library documentation);
 *  - Upload the sketch and open the Serial Monitor;
 *  - "Teach the Arduino" which keys you want to use to increase or decrease a value: 
 *    point the NEC remote at the receiver and press any key of your choosing, then press a different key;
 *  - Use the selected keys to increase or decrease the value, one unit at a time for each keypress,
 *    or, if you hold the key: 
 *      1 unit for each of the first 9 repeat codes,
 *      5 units for each of the next 18 repeat codes, and
 *      25 units for each of the subsequent repeat codes.
 *
 * In this example it's assumed that the IR receiver is connected to digital pin 2
 * and that the pin supports external interrupts.
 * 
 * For more information on the usable pins of each board, see the library documentation at:
 * https://github.com/LuisMiCa/IRsmallDecoder
 * or read the README.pdf file in the extras folder of this library.
 */

#define IR_SMALLD_NEC
#include <IRsmallDecoder.h>
IRsmallDecoder irDecoder(2);  // Assuming that the IR receiver is connected to digital pin 2
irSmallD_t irData;

int value = 0;
int keyInc, keyDec;
int rptCounter = 0;
int increment = 1;

void setup() {
  Serial.begin(115200);

  Serial.print("Press the \"UP\" key on the remote ");
  while (!irDecoder.dataAvailable(irData)) ;  // Waiting for one keypress
  keyInc = irData.cmd;
  Serial.print("(key cmd=");
  Serial.print(keyInc, HEX);
  Serial.println(")");

  Serial.print("Press the \"DOWN\" key on the remote ");
  do {
    while (!irDecoder.dataAvailable(irData)) ;  // Waiting for another keypress
  } while (irData.cmd == keyInc);  // If it's the same key, go back and wait for another
  keyDec = irData.cmd;
  Serial.print("(key cmd=");
  Serial.print(keyDec, HEX);
  Serial.println(")");

  Serial.println();
  Serial.println("Press the \"UP\" key to increase the value;");
  Serial.println("Press and hold it to increase faster and faster;");
  Serial.println("Press the \"DOWN\" key to decrease the value;");
  Serial.println("Press and hold it to decrease faster and faster.");
  Serial.println();
}

void loop() {
  if (irDecoder.dataAvailable(irData)) {
    if (irData.keyHeld && ((irData.cmd == keyInc) || (irData.cmd == keyDec))) {
      rptCounter++;
      if (rptCounter < 10) increment = 1;
      else if (rptCounter < 28)
        increment = 5;
      else
        increment = 25;
    } else {
      rptCounter = 0;
      increment = 1;
    }

    if (irData.cmd == keyInc) {
      value += increment;
      if (value > 1000) value = 1000;
      Serial.println(value);
    } else if (irData.cmd == keyDec) {
      value -= increment;
      if (value < 0) value = 0;
      Serial.println(value);
    }
  }
}
