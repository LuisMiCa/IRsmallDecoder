/* IRsmallD_NEC - NEC protocol decoder
 *
 * This file is part of the IRsmallDecoder library for Arduino
 * Copyright (c) 2020 Luis Carvalho
 *
 *
 * Protocol specifications:
 * ------------------------
 * Modulation type: pulse distance.
 * Carrier frequency: 38kHz. 
 * Each pulse burst is 562.5µs in length.
 * The signal starts with a 9ms leading pulse burst (16 times the pulse burst length used for a logical data bit),
 * followed by a 4.5ms space. Then, the logical bits are transmitted as follows:
 *   Logical '0' – a 562.5µs pulse burst followed by a 562.5µs space (total transmit time: 1125µs).
 *   Logical '1' – a 562.5µs pulse burst followed by a 1687.5µs space (total transmit time: 2250µs).
 *
 * Logical bits' transmission order for standard NEC protocol:
 *   8-bit address → inverted address → 8-bit command → inverted command
 *   LSB ... MSB     LSB ... MSB       LSB ... MSB      LSB ... MSB
 *
 * Logical bits' transmission order for Extended NEC protocol:
 *   Low address byte → high address byte → 8-bit command → inverted command
 *   LSB ... MSB        LSB ... MSB         LSB ... MSB      LSB ... MSB 
 *
 * The signal ends with an extra 560µs pulse, required to determine the value of the last bit.
 *
 * Notes:
 *   - Inverted bytes have 1s where the original has 0s, and vice versa.
 *   - The Extended NEC protocol sacrifices address redundancy to extend
 *     the address field from 8 to 16 bits (inverted address becomes the high address byte).
 *
 * Repeat codes:
 *   If a key is held, a repeat code is sent every 108ms.
 *   It consists of a 9ms pulse burst, followed by a 2250µs space, and then a 562.5µs pulse burst.
 */


// NEC/NECx timings in microseconds:
#define NEC_L_MARK 5062.5   /* Leading Mark */
#define NEC_R_MARK 2812.5   /* Repeat Mark */
#define NEC_R_TOL   803.6   /* Repeat Mark tolerance (µs) */
#define NEC_MARK_0  1125    /* Bit 0 Mark */
#define NEC_MARK_1  2250    /* Bit 1 Mark */
#if defined(IR_SMALLD_NEC)  /* Gap1 (interval before first repeat mark) varies with the protocol */ 
  #define NEC_GAP_1 48937.5 /* Gap1 for NEC */
#else                       /* If it's NECx, it does not have a constant address frame length: */
  #define NEC_GAP_1 39937.5 /* Gap1 is smallest when addr=FFFF; Gap1 = 48937.5 - (8 x 1125) = 39937.5 */  
#endif
#define NEC_GAP_2 105187.5  /* Gap2 (between repeat marks) */
// For more information about these timings, go to:
// https://github.com/LuisMiCa/IRsmallDecoder/blob/master/extras/Timings/NEC_timings.svg


void IR_ISR_ATTR IRsmallDecoder::irISR() {  // Triggered on each rising edge of the IR receiver output signal.
  // The signal goes LOW when IR light is detected (so the rising edge marks the end of an IR pulse).

  // NEC timings' thresholds in microseconds:
  const uint16_t c_GapMin = NEC_GAP_1 * 0.7;        // 34256 (or 27956 for NECx)
  const uint32_t c_GapMax = NEC_GAP_2 * 1.3;        //136743
  const uint16_t c_RMmin = NEC_R_MARK * 0.7;        //  1968
  const uint16_t c_RMmax = NEC_R_MARK + NEC_R_TOL;  //  3616
  const uint16_t c_LMmin = c_RMmax + 1;             //  3617
  const uint16_t c_LMmax = NEC_L_MARK * 1.3;        //  6581
  const uint16_t c_M1min = NEC_MARK_1 * 0.7;        //  1575
  const uint16_t c_M1max = NEC_MARK_1 * 1.3;        //  2925
  const uint16_t c_M0min = NEC_MARK_0 * 0.7;        //   787

  //number of initial repetition marks to be ignored:
  const uint8_t c_RptCount = 2;

  // FSM variables:
  static uint32_t duration;
  static uint8_t bitCount;
  static union {            // Received bits are stored in reversed order (11000101... -> ...10100011)
    uint32_t all = 0;       // Arduino uses Little Endian so, if all=ABCDEF89 then in memory it's 89EFCDAB (hex format)
    uint8_t byt[4];         // and we get byt[0]=89, byt[1]=EF, byt[2]=CD and byt[3]=AB (type punning with a union...)
  } irSignal;
  static uint8_t repeatCount = 0;
  static bool possiblyHeld = false;

  DBG_RESTART_TIMER();

  duration = micros() - _previousTime;  // Note: micros() has a 4μs resolution (multiples of 4) @ 16MHz or 8μs @ 8MHz
  _previousTime = micros();
  DBG_PRINTLN_DUR(duration);

  switch (_state) {  // Asynchronous (event-driven) Finite State Machine
    case 0:  // Standby:
      if (duration > c_GapMin) {
        if (duration > c_GapMax) possiblyHeld = false;
        _state = 1;
      } 
      else possiblyHeld = false;
    break;

    case 1:  // StartPulse:
      if (duration >= c_LMmin && duration <= c_LMmax) {  // It's a Leading Mark
        bitCount = 0;
        repeatCount = 0;
        _state = 2;
      } else {
        if (possiblyHeld && duration >= c_RMmin && duration <= c_RMmax) {  // It's a Repeat Mark
          if (repeatCount < c_RptCount) repeatCount++;  // First repeat signals will be ignored
          else if (!_irCopyingData) {                   // If not interrupting a copy, then a key was held
            _irData.keyHeld = true;
            _irDataAvailable = true;
          }
        }
        _state = 0;
      }
    break;

    case 2:  // Receiving:
      if (duration < c_M0min || duration > c_M1max) _state = 0;  // Error: not a bit mark
      else {                 // It's M0 or M1
        irSignal.all >>= 1;  // Push a 0 from left to right (will be left at 0 if it's M0)
        if (duration >= c_M1min) irSignal.byt[3] |= 0x80;  // It's M1, change MSB to 1
        bitCount++;
        #if defined(IR_SMALLD_NEC)  // Conditional code inclusion (resolved at compile time)
          if (bitCount == 16) {     // Address and Inverted Address received
            if (irSignal.byt[2] != (uint8_t)~irSignal.byt[3]) _state = 0;  // Address error
            // Else, remain in this state (Address OK, continue with command reception)
          }
          else   // That's right, a loose else...
        #endif
        if (bitCount == 32) {  // All bits received
          if (!_irCopyingData && (irSignal.byt[2] == (uint8_t)~irSignal.byt[3])) {  // If not interrupting a copy and command OK, finish decoding
            #if defined(IR_SMALLD_NEC)  // NEC address has 8 bits
              _irData.addr = irSignal.byt[0];
            #else  // it must be IR_SMALLD_NECx (16 bits)
              _irData.addr = *(uint16_t*)(&irSignal.byt[0]);  // Type punning with a cast...
              // .addr is the 16 bit value pointed by the address of byt[0]  (byt[1] is addr high byte)
              // Dereferencing type-punned pointer will break strict-aliasing rules
              // Usually tolerated by AVR-GCC, but may not work on other compilers
            #endif
            _irData.cmd = irSignal.byt[2];
            _irData.keyHeld = false;
            _irDataAvailable = true;
            possiblyHeld = true;  // Will remain true if the next gap is OK
          }
          _state = 0;
        }
        // Else, remain in this state (continue receiving)
      }
    break;
  }

  DBG_PRINTLN_TIMER();
  DBG_PRINT_STATE(_state);
}