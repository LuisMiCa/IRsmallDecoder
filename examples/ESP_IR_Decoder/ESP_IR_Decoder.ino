/* This example is part of the IRsmallDecoder library and is intended to demonstrate
 * how to use this library with an ESP8266 or ESP32 microcontroller.
 *
 * Steps:
 *  1 - Connect the IR receiver (see library documentation);
 *  2 - Uncomment the #define for the desired protocol,
 *      (leave the others as comments, only one is allowed);
 *  3 - Upload this sketch and open the Serial Monitor;
 *  4 - Push buttons on the remote control. 
 *      You should see the decoded data on the Serial Monitor.
 *
 * In this example, it is assumed that the IR receiver is connected to the GPIO pin 5. 
 * (Or D5 if you're using an Arduino Nano ESP32 with the default pin numbering. For more information see:
 * https://support.arduino.cc/hc/en-us/articles/10483225565980-Select-pin-numbering-for-Nano-ESP32-in-Arduino-IDE).
 *
 * For details on the boards' usable pins, refer to the library documentation:  
 * https://github.com/LuisMiCa/IRsmallDecoder or check the README.pdf file in the extras folder.  
 */


// »»»»»» Select one Protocol:
#define IR_SMALLD_NEC
// #define IR_SMALLD_NECx
// #define IR_SMALLD_RC5
// #define IR_SMALLD_SIRC12
// #define IR_SMALLD_SIRC15
// #define IR_SMALLD_SIRC20
// #define IR_SMALLD_SIRC
// #define IR_SMALLD_SAMSUNG
// #define IR_SMALLD_SAMSUNG32

// »»»»»» Choose a pin where you'll connect the IR module:
#define IR_DECODER_PIN 5

// »»»»»» Include the library:
#include <IRsmallDecoder.h>

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("Waiting for a remote control IR signal...");
  Serial.println("held\tAddr\tCmd");
}

void loop() {
  // »»»»»» Create one static decoder:
  static IRsmallDecoder irDecoder(IR_DECODER_PIN);

  // »»»»»» Create a decoder data structure:
  irSmallD_t irData;

  // »»»»»» Check for a decoded signal and use it:
  if (irDecoder.dataAvailable(irData)) {
    Serial.print("» ");
    #if defined(IR_SMALLD_SIRC12) || defined(IR_SMALLD_SIRC15) || defined(IR_SMALLD_SIRC20)
      Serial.print("-");  // SIRC12/15/20 do not have keyHeld
    #else
      Serial.print(irData.keyHeld);
    #endif
    Serial.print("\t");
    Serial.print(irData.addr, HEX);
    Serial.print("\t");
    Serial.print(irData.cmd, HEX);
    #if defined(IR_SMALLD_SIRC20) || defined(IR_SMALLD_SIRC)
      Serial.print("\t");
      Serial.print(irData.ext, HEX);  // SIRC20 and SIRC have extended data
    #endif
    Serial.println();
  }
}


// Note:
//   ESP-based MCUs do not support global object instantiation for the decoder because the IR receiver
//   depends on hardware resources that are not initialized before setup() runs. Using a global object
//   may lead to undefined behavior or crashes. To avoid this issue, we must declare the decoder as a 
//   static variable inside the loop() function or, as an alternative, dynamically allocate the decoder 
//   inside setup() instead.
//
// Alternative Example:
//   #define IR_SMALLD_NEC
//   #include <IRsmallDecoder.h>
//   IRsmallDecoder *irDecoder;
//   void setup() {
//     Serial.begin(115200);
//     irDecoder = new IRsmallDecoder(5);  // Assuming the IR receiver is connected to pin 5
//   }
//   void loop() {
//     irSmallD_t irData;
//     if (irDecoder->dataAvailable(irData)) {
//       Serial.println(irData.cmd, HEX);
//     }
//   }
//
// Using Other Examples:
//   If you wish, you can try the other examples from this library on an ESP-based MCU,  
//   but you'll need to make some changes to the example code.  
//   The easiest way is to move the global declaration of the decoder to the beginning of the loop()  
//   function and declare it as static. The decoder data structure may remain as a global variable.