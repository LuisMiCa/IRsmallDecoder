/* IRsmallD_SIRC_basic - Sony SIRC 12/15/20-bit protocol - Basic, individual decoder
 *
 * This file is part of the IRsmallDecoder library for Arduino
 * Copyright (c) 2020 Luis Carvalho
 *
 *
 * Protocol specifications:
 * ------------------------
 * Modulation type: pulse width;
 * Carrier frequency: 40kHz;
 *
 * Signal begins with a Start Mark: a 2400µs pulse followed by a 600µs space;
 * Bit 0 mark: it's a 600µs pulse followed by a 600µs space;
 * Bit 1 mark: it's a 1200µs pulse followed by a 600µs space;
 *
 * The number of bits can be 12, 15, or 20:
 *   12 bit signal (transmitted in this order):
 *     7 bit command      5 bit address
 *     LSB ..... MSB      LSB ..... MSB
 *
 *   15 bit signal (transmitted in this order):
 *     7 bit command      8 bit address
 *     LSB ..... MSB      LSB ..... MSB
 *
 *   20 bit signal (transmitted in this order):
 *     7 bit command      5 bit address     8 bit extended
 *     LSB ..... MSB      LSB ..... MSB     LSB .....  MSB
 *
 * Signals repetition:
 *   For each key press (on most Sony remotes) the corresponding signal frame is
 *   sent, at least, 3 times. If the key is held, the signal continues to repeat.
 *   The repetition period is 45ms (= 75 x 600µs).
 *   
 * The gap between two repetitions varies according to the number of bits sent
 * and the bits themselves. (Bit 0 mark is 1200µs long and Bit 1 mark is 1800µs long).
 *
 *
 * Timings (in microseconds) and multipliers:
 *   BASIC_MARK_LENGTH = 600
 *   START_PULSE_MULT  = 4
 *   MARK1_MULT        = 2
 *   RPEAT_PERIOD_MULT = 75
 *   MAX_FRAME_MULT    = 4 + c_NumberOfBits * 3   ( max size of the signal - all bits are "1")
 *
 */


void IR_ISR_ATTR IRsmallDecoder::irISR() {  // Triggered on each rising edge of the IR receiver output signal.
  // The signal goes LOW when IR light is detected (so the rising edge marks the end of an IR pulse).

  #if defined(IR_SMALLD_SIRC12)  // Set bit count based on the selected protocol (resolved at compile time)
    const uint8_t c_NumberOfBits = 12;
  #elif defined(IR_SMALLD_SIRC15)
    const uint8_t c_NumberOfBits = 15;
  #else  // It must be IR_SMALLD_SIRC20
    const uint8_t c_NumberOfBits = 20;
  #endif

  // SIRC timings' thresholds in micro secs:
  // bit 0 Mark length = 600µs space + 600µs pulse  = 1200µs
  // bit 1 Mark length = 600µs space + 1200µs pulse = 1800µs
  // Maximum tolerance = (1800 - 1200) / 2 = 300
  const uint16_t c_M1max = 2100;  // = 1800 + 300 (it could be more)
  const uint16_t c_M1min = 1500;  // = 1800 - 300
  const uint16_t c_M0min =  900;  // = 1200 - 300 (it could be less)
  // Minimum standard Gap length = (75 - (4 + 3 x c_NumberOfBits)) x 600
  const uint16_t c_GapMin  = (75 -(4 + 3 * c_NumberOfBits)) * 600 * 0.8;  // 20% below standard value

  // FSM variables:
  static uint32_t duration;
  static uint8_t bitCount;
  static union {  // Received bits are stored in reversed order (e.g., 11000101... -> ...10100011)
    #if defined(IR_SMALLD_SIRC12) || defined(IR_SMALLD_SIRC15)
      uint16_t all = 0;  // Arduino uses Little Endian so, if all=ABCD then in memory it's CDAB (hex format),
      uint8_t byt[2];    // then we get byt[0]=CD and byt[1]=AB (type punning with a union...)
    #else                // It must be IR_SMALLD_SIRC20
      uint32_t all = 0;  // If all=ABCDEF00 then in memory it's 00EFCDAB (hex format)
      uint8_t byt[4];    // byt[0]=00;  byt[1]=EF;  byt[2]=CD;  byt[3]=AB
    #endif
  } irSignal;

  DBG_RESTART_TIMER();

  duration = micros() - _previousTime;
  _previousTime = micros();
  DBG_PRINTLN_DUR(duration)

  switch (_state) {  // Asynchronous (event-driven) Finite State Machine
    case 0: // Standby
      if (duration > c_GapMin) {  // Only starts after a GAP without signals
        bitCount = 0;
        _state = 1;  // Leading pulse detected
      }
    break;

    case 1: // Receiving
      if (duration < c_M0min || duration > c_M1max) _state = 0;  // Not a Mark duration
      else {                                                     // It's M0 or M1
        irSignal.all >>= 1;  // Push a 0 from left to right (will be left at 0 if it's M0)
        bitCount++;
        if (duration >= c_M1min) {  // It's a bit 1 mark, change Most Significant bit to 1
          #if defined(IR_SMALLD_SIRC12) || defined(IR_SMALLD_SIRC15)
            irSignal.byt[1] |= 0x80;
          #else   // IR_SMALLD_SIRC20 uses 4 bytes
            irSignal.byt[3] |= 0x80;
          #endif
        }
        if (bitCount == c_NumberOfBits) {  // All bits received
          if (!_irCopyingData) {           // If not interrupting a copy, decode the signal; otherwise, discard it
            #if defined(IR_SMALLD_SIRC12)
              irSignal.all >>= 3;          // Adjust address in the high byte (only needed for SIRC12)
            #endif
            #if defined(IR_SMALLD_SIRC12) || defined(IR_SMALLD_SIRC15)
              irSignal.byt[0] >>= 1;           // Adjust command in the low byte
              _irData.addr = irSignal.byt[1];  // Little-Endian puts high byte in the array's second position
              _irData.cmd  = irSignal.byt[0];
            #else                              // Else, it's IR_SMALLD_SIRC20...
              _irData.ext = irSignal.byt[3];
              irSignal.byt[3] = 0;
              irSignal.all >>= 3;
              irSignal.byt[1] >>= 1;
              _irData.addr = irSignal.byt[2];
              _irData.cmd  = irSignal.byt[1];
            #endif  
            _irDataAvailable = true;
          }
          _state = 0; // Done
        } // Else, remain in this state (continue receiving)
      }
    break;
  }
  DBG_PRINTLN_TIMER();
  DBG_PRINT_STATE(_state);
}


/* 12-bit signal decode process (bit order is already reversed)
 *                     byt[1] (high byte)        byt[0] (low byte)
 * encoded bits:    A4 A3 A2 A1 A0 C6 C5 C4   C3 C2 C1 C0  0  0  0  0
 * all >> 3          0  0  0 A4 A3 A2 A1 A0   C6 C5 C4 C3 C2 C1 C0  0
 * byt[0] >> 1       0  0  0 A4 A3 A2 A1 A0    0 C6 C5 C4 C3 C2 C1 C0
 * 
 *
 * 15-bit signal decode process (bit order already reversed)
 *                     byt[1] (high byte)        byt[0] (low byte)
 * encoded bits:    A7 A6 A5 A4 A3 A2 A1 A0   C6 C5 C4 C3 C2 C1 C0  0
 * byt[0] >> 1      A7 A6 A5 A4 A3 A2 A1 A0    0 C6 C5 C4 C3 C2 C1 C0
 * 
 *
 * 20-bit signal decode process (bit order already reversed)
 *                            byt[3] (high)                 byt[2]                   byt[1]             byt[0] (low)
 * encoded bits:         E7 E6 E5 E4 E3 E2 E1 E0   A4 A3 A2 A1 A0 C6 C5 C4   C3 C2 C1 C0  0  0  0  0   0 0 0 0 0 0 0 0
 * extract ext code       0  0  0  0  0  0  0  0   A4 A3 A2 A1 A0 C6 C5 C4   C3 C2 C1 C0  0  0  0  0   0 0 0 0 0 0 0 0
 * all >> 3               0  0  0  0  0  0  0  0    0  0  0 A4 A3 A2 A1 A0   C6 C5 C4 C3 C2 C1 C0  0   0 0 0 0 0 0 0 0
 * byt[1] >> 1            0  0  0  0  0  0  0  0    0  0  0 A4 A3 A2 A1 A0    0 C6 C5 C4 C3 C2 C1 C0   0 0 0 0 0 0 0 0
*/
