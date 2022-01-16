/* Temporarily disable the decoder
 *  
 * This example is part of the the IRsmallDecoder library and is intended to demonstrate 
 * a possible usage for the enable and disable methods of the decoder.
 *  
 * How to use this sketch: 
 *  - Connect the IR receiver (see library documentation);
 *  - Uncomment the #define for the desired protocol,
 *    (leave the others as comments, only one is allowed);
 *  - Upload the sketch and open the Serial Monitor;
 *  - "Teach the Arduino" which key you want to use to disable the decoder;
 *  - The selected key will temporarily disable the decoder;
 *  - Any other key on the remote will toggle the builtin LED;
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
#define TIMEOUT 8

IRsmallDecoder irDecoder(2);  //assuming that the IR receiver is connected to digital pin 2
irSmallD_t irData;
int ledState = LOW;
int keyDisable;
int timer = 0;
unsigned long previousMillis = 0;
unsigned long currentMillis;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(250000);
  Serial.println("Press the remote key that will temporarily disable the IR decoder.");
  while (!irDecoder.dataAvailable(irData)) ;  //waiting for one keypress
  keyDisable = irData.cmd;
  Serial.print("(Key Cmd=");
  Serial.print(keyDisable, HEX);
  Serial.println(")");
  Serial.println("When the decoder is enabled, any other key will toggle the onboard LED.");
}

void loop() {
  currentMillis = millis();
  if ((timer > 0) && (currentMillis - previousMillis >= 1000)) {
    previousMillis = currentMillis;
    timer = timer - 1;
    Serial.println(timer);
    if (timer == 0) {
      irDecoder.enable();
      Serial.println("IR Decoder enabled");
    }
  }
  if (irDecoder.dataAvailable(irData)) {
    if (irData.cmd == keyDisable) {
      irDecoder.disable();
      timer = TIMEOUT;
      Serial.print("IR Decoder disabled for ");
      Serial.print(timer);
      Serial.println(" seconds.");
    } else {
      ledState = (ledState == LOW) ? HIGH : LOW;
      digitalWrite(LED_BUILTIN, ledState);
    }
  }
}