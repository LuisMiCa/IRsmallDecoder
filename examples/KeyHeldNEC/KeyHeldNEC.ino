/* keyHeld with NEC
 *  
 * This example is part of the the IRsmallDecoder library and is intended to demonstrate 
 * a possible usage for the keyHeld data member.
 *  
 * How to use this sketch: 
 *  - Connect the IR receiver (see library documentation);
 *  - Upload the sketch and open the Serial Monitor;
 *  - "Teach the Arduino" which keys you want to use to increase or decrease a value: 
 *    point the NEC remote to the receiver and press any key of your choosing then press a different key;
 *  - Use the selected keys to increase/decrease the value, one unit at a time for each keypress,
 *    or, if you hold the key: 
 *      1 unit for each of the first 9 repeat codes; 
 *      5 units for each of the next 18 repeat codes;
 *      25 units for each of the subsequent repeat codes.
 *
 * In this example it's assumed that the IR receiver is connected to digital pin 2 and 
 * the pin is usable for external interrupts.
 * 
 * For more information on the boards' usable pins, see the library documentation at:
 * https://github.com/LuisMiCa/IRsmallDecoder
 * or the README.pdf file in the extras folder of this library. 
 */

#define IR_SMALLD_NEC
#include <IRsmallDecoder.h>
IRsmallDecoder irDecoder(2);  //assuming that the IR receiver is connected to digital pin 2
irSmallD_t irData;

int value = 0;
int keyInc, keyDec;
int rptCounter = 0;
int increment = 1;

void setup() {
  Serial.begin(250000);

  Serial.print("Press the \"UP\" key on the remote ");
  while (!irDecoder.dataAvailable(irData)) ;  //waiting for one keypress
  keyInc = irData.cmd;
  Serial.print("(key cmd=");
  Serial.print(keyInc, HEX);
  Serial.println(")");

  Serial.print("Press the \"DOWN\" key on the remote ");
  do {
    while (!irDecoder.dataAvailable(irData)) ;  //waiting for another keypress
  } while (irData.cmd == keyInc);  //if it's the same key, go back and wait for another
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
