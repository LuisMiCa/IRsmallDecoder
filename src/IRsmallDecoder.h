/* IRsmallDecoder v1.2.1
 *
 * This is an Arduino library for receiving and decoding infrared signals from remote controls.
 * It uses compact, fast, and reliable decoders that operate without relying on timers.
 * Ideal for projects that only need to receive signals from a single remote and don't require IR transmission.
 *
 * Copyright (c) 2020 Luis Carvalho
 * This Library is licensed under the MIT License. See the LICENSE file for details.
 *
 * Notes:
 * ► In the INO file, one of following directives must be used:
 *    #define IR_SMALLD_NEC
 *    #define IR_SMALLD_NECx
 *    #define IR_SMALLD_RC5
 *    #define IR_SMALLD_SIRC12
 *    #define IR_SMALLD_SIRC15
 *    #define IR_SMALLD_SIRC20
 *    #define IR_SMALLD_SIRC
 *    #define IR_SMALLD_SAMSUNG
 *    #define IR_SMALLD_SAMSUNG32
 *    
 *   before the
 *    #include <IRsmallDecoder.h>
 *
 * ► RC5 works for both normal and extended versions;
 * ► SIRC12, SIRC15 and SIRC20 use a basic (smaller and faster) implementation but without some features...
 * ► SIRC handles SIRC 12, 15 and 20 bits, by taking advantage of the fact that most Sony remotes send
 *   three frames each time one button is pressed. It uses triple frame verification and checks if keyHeld;
 * ► IR_SMALLD_NO_TIMEOUT can be defined before the #include <IRsmallDecoder.h> to disable the timeout feature.
 */

#ifndef IRsmallDecoder_h
#define IRsmallDecoder_h

#if ARDUINO >= 100
  #include <Arduino.h>
#else
  #include <WProgram.h>
  #include <pins_arduino.h>
  #include <WConstants.h>
#endif

#include "IRsmallDProtocolCheck.h"
#include "IRsmallDProtocolStructs.h"
#include "IRsmallDDebug.h"


// ****************************************************************************
// IR_ISR_MODE definition based on protocol:
// Keep in mind that the signal is active low. The decoder pulls the input pin to LOW when detecting an IR pulse.
#if defined(IR_SMALLD_SAMSUNG) || defined(IR_SMALLD_SAMSUNG32)
  #define IR_ISR_MODE  FALLING

#elif defined(IR_SMALLD_SIRC12) || defined(IR_SMALLD_SIRC15) || defined(IR_SMALLD_SIRC20) || \
      defined(IR_SMALLD_SIRC)   || defined(IR_SMALLD_NEC)    || defined(IR_SMALLD_NECx)
  #define IR_ISR_MODE  RISING

#elif defined(IR_SMALLD_RC5)
  #define IR_ISR_MODE  CHANGE

#else
  #error IR_ISR_MODE not defined.
#endif


// ****************************************************************************
// Set the ISR memory attribute (for fast execution) according to the MCU type:
// In ESP32 / ESP8266, flash memory can be slow and inaccessible during certain operations, making it 
// critical to store ISRs in IRAM / ICACHE (Instruction RAM / Instruction CACHE) for fast and reliable execution.
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  #if defined(ARDUINO_ISR_ATTR)
    #define IR_ISR_ATTR ARDUINO_ISR_ATTR
  #elif defined(IRAM_ATTR)
    #define IR_ISR_ATTR IRAM_ATTR
  #else
    #define IR_ISR_ATTR  // empty attribute
    #warning "No ISR attribute for this ESP32!? (interrupts will not be optimized for fast execution)"
  #endif
#elif defined(ESP8266) || defined(ARDUINO_ARCH_ESP8266)
  #if defined(ICACHE_RAM_ATTR)
    #define IR_ISR_ATTR ICACHE_RAM_ATTR
  #else
    #define IR_ISR_ATTR  // empty attribute
    #warning "No ISR attribute for this ESP8266!? (interrupts will not be optimized for fast execution)"
  #endif
#else
  #define IR_ISR_ATTR  // empty attribute
#endif


// ****************************************************************************
// By default, a timeout will be used (if IR_SMALLD_NO_TIMEOUT is not defined).
// Timeout value is in microseconds.
#if defined(IR_SMALLD_NEC) || defined(IR_SMALLD_NECx)
  #define IR_SMALLD_TIMEOUT 126226  // =ceil(105188 x 1.2)  // max time between events = 105.188ms (@IR fall -> I/O input rise)
#elif defined(IR_SMALLD_RC5)
  #define IR_SMALLD_TIMEOUT 106920  // =ceil(89100 x 1.2)   // max time between events = 114ms-24.9ms = 89.100ms (OnChange mode)
#elif defined(IR_SMALLD_SAMSUNG)
  #define IR_SMALLD_TIMEOUT 33525   // =ceil(27937.5 x 1.2) // max time between events = 27937.5 µs (@IR rise -> I/O input fall)
#elif defined(IR_SMALLD_SAMSUNG32)  
  #define IR_SMALLD_TIMEOUT 64125   // =ceil(53437.5 x 1.2) // max time between events = 53437.5 µs (@IR rise -> I/O input fall)
#elif defined(IR_SMALLD_SIRC12) || defined(IR_SMALLD_SIRC15) || defined(IR_SMALLD_SIRC20)
  #define IR_SMALLD_TIMEOUT 2160    // =ceil(1800 x 1.2)    // max time between events = 1800 µs (@IR fall -> I/O input rise)
#elif defined(IR_SMALLD_SIRC)
  #define IR_SMALLD_TIMEOUT 37440   // =ceil(31200 x 1.2)   // max time between events = 31200 µs (@IR fall -> I/O input rise)
#else
  #error Timeout not defined.
#endif


// ****************************************************************************
// Decoder class's forward declaration/definition
/**
 * InfraRed Signals Decoder's Class, used to decode infrared signals from remote controls.
 */
class IRsmallDecoder {
  private:
    static void irISR();
    void resetFSM();                        // used by enable() and checkTimeout() methods
    void checkTimeout();                    // used by dataAvailable() method
    static volatile bool _irDataAvailable;  // will be updated by the ISR
    static volatile irSmallD_t _irData;     // will be updated by the ISR
    static volatile uint8_t _state;         // will be updated and used by the ISR (and timeout)
    static volatile uint32_t _previousTime; // will be updated and used by the ISR (and timeout)
    static bool _irCopyingData;             // used by the ISR but not changed by it, no need for volatile
    uint8_t _irInterruptNum;                // used by enable/disable Decoder methods
    
  public:
    IRsmallDecoder(uint8_t interruptPin);
    void disable();
    void enable(); 
    bool dataAvailable(irSmallD_t &irData);
    bool dataAvailable();                   // method overloading
};


// ****************************************************************************
// Static variables from a class must be re-declared/initialized
// outside the class' forward declaration/definition (usually in the cpp file not the header)
volatile bool IRsmallDecoder::_irDataAvailable = false;  // flag to indicate that new data is available
volatile irSmallD_t IRsmallDecoder::_irData;             // data structure to hold the decoded data
volatile uint8_t IRsmallDecoder::_state = 0;             // FSM satate (0=standby, in any of the FSMs)
volatile uint32_t IRsmallDecoder::_previousTime = UINT32_MAX; // not ini with 0, that would cause an issue in the RC5 decoder
bool IRsmallDecoder::_irCopyingData = false;             // used to avoid volatile _irData corruption by the ISR


// ****************************************************************************
// Decoder's Methods Implementation
/**
 * IRsmallDecoder object constructor
 * 
 * @param interruptPin is the digital pin where the IR receiver is connected. That pin must support external interrupts
 */
IRsmallDecoder::IRsmallDecoder(uint8_t interruptPin) {
  pinMode(interruptPin,INPUT_PULLUP);  //active low
  #if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || \
      defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    _irInterruptNum=0;
  #else
    _irInterruptNum = digitalPinToInterrupt(interruptPin);
  #endif
  attachInterrupt(_irInterruptNum, irISR, IR_ISR_MODE);
}


/**
 * Reset the ISR's FSM, transitioning it to the Standby state.
 */
void IRsmallDecoder::resetFSM() {
  this->irISR();  // two consecutive calls will place any of the FSMs in standby state
  this->irISR();  // forced _state=0 will not work with computed GOTOs based FSMs
}


/**
 * Checks if the FSM is in a non-standby state for too long and resets it if necessary.
 */
void IRsmallDecoder::checkTimeout() {
  if (_state) {                    // If FSM is not in standby state, check for timeout:
    uint32_t prevTimeCopy;
    noInterrupts();                // The following line is not atomic on many microcontrollers, if interrupts
    prevTimeCopy = _previousTime;  // were enabled, the ISR could change _previousTime while we are copying it
    interrupts();                  // It's for a very short time, it won't affect the timing of the ISR
    // Check if the time since the last event is greater than the timeout value:
    if ((uint32_t)(micros() - prevTimeCopy) >= (uint32_t)IR_SMALLD_TIMEOUT) {
      DBG_PRINT_STATE("\nTIMEOUT\n");
      this->resetFSM();  // Put the FSM in Standby state
    }
  }
}

/**
 * Enables the decoder by reattaching the ISR to the hardware interrupt (and resets the FSM).
 */
void IRsmallDecoder::enable() {
  attachInterrupt(_irInterruptNum, irISR, IR_ISR_MODE);  //interrupt flag may already be set
  // if so, ISR will be immediately executed and the FSM jumps out of standby state
  this->resetFSM();  // Put the FSM in Standby state
}


/**
 * Disables the decoder, without interfering with other interrupts
 */
void IRsmallDecoder::disable() {
  detachInterrupt(_irInterruptNum);
}


/**
 * Informs if there's new decoded data and retrieves it if so.
 * 
 * @param irData If there's new data available, it is "moved" to this data structure.
 * @return true if new data was decoded and retrieved; false if not.
 */
bool IRsmallDecoder::dataAvailable(irSmallD_t &irData) {
  // Check Timeout (resets the FSM if necessary):
  #if not defined(IR_SMALLD_NO_TIMEOUT)
    this->checkTimeout();
  #endif
  // Copy data if available:
  if (_irDataAvailable) {
    _irCopyingData = true;     // Let the ISR know that it cannot change the data while it's being copied.
    memcpy(&irData, (void*)&_irData, sizeof(_irData));  // This is not an atomic operation; the ISR could potentially corrupt the data.
    _irDataAvailable = false;  // An ATOMIC_BLOCK would work, but it's not supported on many boards and, as in the disable/enable
    _irCopyingData = false;    // interrupts method, it could unnecessarily keep the ISR from determining the exact intervals  
    // between the signals pulses (memcpy() may take a few microseconds to execute). And besides, it should rarely happen that the
    // ISR is replacing existing data that was not retrieved yet. Unless the user is not polling the dataAvailable() frequently enough.
    return true;
  } else return false;
}


/**
 * Informs if there is new decoded data and DISCARDS it if so.
 * 
 * @return true if new data was decoded; false if not.
 */
bool IRsmallDecoder::dataAvailable() {
  // Check Timeout and reset FSM if necessary:
  #if not defined(IR_SMALLD_NO_TIMEOUT)
    this->checkTimeout();
  #endif
  // Check data availability:
  if (_irDataAvailable) {
    _irDataAvailable = false;
    return true;
  } else return false;
}


// ----------------------------------------------------------------------------
// Computed GOTOs (labels as values) FSM control:
#define FSM_INITIALIZE(initialState) static void* fsm_state = &&initialState
#define FSM_SWITCH() goto *fsm_state; while(false)
#define FSM_NEXT(nextState) fsm_state = &&nextState
#define FSM_DIRECTJUMP(label) goto label


// ----------------------------------------------------------------------------
// Conditional inclusion of protocol specific ISR implementations:
#if defined(IR_SMALLD_NEC) || defined(IR_SMALLD_NECx)
  #include "IRsmallD_NEC.h"
#elif defined(IR_SMALLD_RC5)
  #include "IRsmallD_RC5.h"
#elif defined(IR_SMALLD_SIRC12) || defined(IR_SMALLD_SIRC15) || defined(IR_SMALLD_SIRC20)
  #include "IRsmallD_SIRC_basic.h"
#elif defined(IR_SMALLD_SIRC)
  #include "IRsmallD_SIRC_multi.h"
#elif defined(IR_SMALLD_SAMSUNG)
  #include "IRsmallD_SAMSUNG.h"
#elif defined(IR_SMALLD_SAMSUNG32)
  #include "IRsmallD_SAMSUNG32.h"
#else
  #error ISR implementation not included.
#endif


#endif // end of the #define IRsmallDecoder_h