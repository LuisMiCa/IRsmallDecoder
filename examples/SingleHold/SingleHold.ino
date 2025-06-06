/* Single Hold 
 *  
 * This example is part of the IRsmallDecoder library and is intended to demonstrate 
 * a possible usage for the keyHeld data member.
 *  
 * How to use this sketch: 
 *  - Connect the IR receiver (see library documentation);
 *  - Uncomment the #define for the desired protocol;
 *    leave the others commented out (only one is allowed);
 *  - Upload the sketch and open the Serial Monitor;
 *  - "Teach the Arduino" which keys you want to use;
 *  - Hold one of the selected keys to turn on/off the built-in LED.
 *    If you keep holding, it won't do anything else.
 *
 * In this example it's assumed that the board has a builtin LED and the IR receiver is 
 * connected to digital pin 2, which must be usable for external interrupts.
 * 
 * For more information on the usable pins of each board, see the library documentation at:
 * https://github.com/LuisMiCa/IRsmallDecoder
 * or read the README.pdf file in the extras folder of this library.
 */

#define IR_SMALLD_NEC
//#define IR_SMALLD_NECx
//#define IR_SMALLD_RC5
//#define IR_SMALLD_SIRC
//#define IR_SMALLD_SAMSUNG
//#define IR_SMALLD_SAMSUNG32

#include <IRsmallDecoder.h>

IRsmallDecoder irDecoder(2);  // Assuming that the IR receiver is connected to digital pin 2
irSmallD_t irData;
int keyOn, keyOff;
bool keyReleased = true;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  Serial.print("Press the key that will turn the LED \"ON\" ");
  while (!irDecoder.dataAvailable(irData)) ;  // Waiting for one keypress
  keyOn = irData.cmd;
  Serial.print("(key cmd=");
  Serial.print(keyOn, HEX);
  Serial.println(")");

  Serial.print("Press the key that will turn the LED \"OFF\" ");
  do {
    while (!irDecoder.dataAvailable(irData)) ;  // Waiting for another keypress
  } while (irData.cmd == keyOn);  // If it's the same key, go back and wait for another
  keyOff = irData.cmd;
  Serial.print("(key cmd=");
  Serial.print(keyOff, HEX);
  Serial.println(")");

  Serial.println();
  Serial.println("Hold the \"ON\" key to turn on the Built-in LED;");
  Serial.println("Hold the \"OFF\" key to turn off the Built-in LED;");

  Serial.println();
}

void loop() {
  if (irDecoder.dataAvailable(irData)) {
    if (irData.keyHeld) {
      if (keyReleased) {
        if (irData.cmd == keyOn) {
          Serial.print("ON ");
          digitalWrite(LED_BUILTIN, HIGH);
        } else if (irData.cmd == keyOff) {
          Serial.print("OFF ");
          digitalWrite(LED_BUILTIN, LOW);
        }
        keyReleased = false;
      }
    } else keyReleased = true;
  }
}
