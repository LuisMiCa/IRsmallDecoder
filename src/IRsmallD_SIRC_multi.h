/* IRsmallD_SIRC_multi - Sony SIRC multi protocol decoder
 *
 * This file is part of the IRsmallDecoder library for Arduino
 * Copyright (c) 2020 Luis Carvalho
 *
 * 
 * Additional Features (not included in the basic versions):
 * ---------------------------------------------------------
 *  - Bit-count auto-detection (12, 15, or 20 bits);
 *  - Triple frame verification;
 *  - KeyHeld check/delay.
 *
 * Protocol specifications:
 * ------------------------
 * Modulation type: pulse width.
 * Carrier frequency: 40kHz.
 *
 * Signal begins with a Start Mark: a 2400µs pulse followed by a 600µs space.
 * Bit 0 mark: a 600µs pulse followed by a 600µs space.
 * Bit 1 mark: a 1200µs pulse followed by a 600µs space.
 *
 * The number of bits can be 12, 15 or 20:
 *   12 bit signal (in order of transmission):
 *     7 bit command      5 bit address
 *     LSB ..... MSB      LSB ..... MSB
 *
 *   15 bit signal (in order of transmission):
 *     7 bit command      8 bit address
 *     LSB ..... MSB      LSB ..... MSB
 *
 *   20 bit signal (in order of transmission):
 *     7 bit command      5 bit address     8 bit extended
 *     LSB ..... MSB      LSB ..... MSB     LSB .....  MSB 
 *
 * Signal repetition:
 *   For each key-pressed (on most Sony remotes) the corresponding signal frame is
 *   sent, at least, 3 times. If the key is held, it keeps sending signals.
 *   The repetition period is 45ms (= 75 x 600µs).
 *
 * The gap between two signal repetitions varies according to the number of bits sent
 * and the bits themselves (bit 0 mark is 1200µs long and Bit 1 mark is 1800µs long).
 */


void IR_ISR_ATTR IRsmallDecoder::irISR() {  // Triggered on each rising edge of the IR receiver output signal.
  // The signal goes LOW when IR light is detected (so the rising edge marks the end of an IR pulse).

  // SIRC timings' thresholds in microseconds:
  // Minimum standard gap length = (75 - (4 + 3 × 20)) × 600  --> assuming 20 '1' bits, which results in the smallest gapMin
  // Maximum standard gap length = (75 - (4 + 2 × 12)) × 600  --> assuming 12 '0' bits, which results in the largest gapMax
  const uint16_t c_GapMax = 33840;  // = (75 -(4 + 2 x 12)) x 600 x 1.2  (20% above standard value)
  const uint16_t c_GapMin =  5280;  // = (75 -(4 + 3 x 20)) x 600 x 0.8  (20% below standard value)
  // Bit 0 Mark length = 600µs space + 600µs pulse  = 1200µs
  // Bit 1 Mark length = 600µs space + 1200µs pulse = 1800µs
  // Maximum tolerance = (1800 - 1200) / 2 = 300
  const uint16_t c_M1max = 2100;  // = 1800 + 300 (it could be more)
  const uint16_t c_M1min = 1500;  // = 1800 - 300
  const uint16_t c_M0min =  900;  // = 1200 - 300 (it could be less)
  
  // Number of initial repetitions to be ignored:
  const uint8_t c_RptCount = 5;

  // FSM variables:
  static uint32_t duration;
  static uint8_t  bitCount;
  static union {       // Received bits are stored in reversed order (11000101... -> ...10100011)
    uint32_t all = 0;  // If all=ABCDEF00 then in memory it's 00EFCDAB (hex format)
    uint8_t  byt[4];   // and byt[0]=00;  byt[1]=EF;  byt[2]=CD;  byt[3]=AB (type punning with a union...)
  } irSignal;
  static uint8_t  frameCount;
  static uint8_t  firstBitCount = 20;
  static uint32_t firstCode;
  static bool     possiblyHeld = false;
  static uint8_t  repeatCount = 0;

  DBG_RESTART_TIMER();

  duration = micros() - _previousTime;
  _previousTime = micros();
  DBG_PRINTLN_DUR(duration)

  switch (_state) {  // Asynchronous (event-driven) Finite State Machine
    case 0:  // Standby
      if (duration >= c_GapMin) {  // Only starts after a GAP without signals
        if (duration > c_GapMax) possiblyHeld = false;
        bitCount = 0;
        irSignal.all = 0;
        frameCount = 1;
        _state = 1;
      } else possiblyHeld = false;
    break;

    case 1:  // Receiving
      if (duration < c_M0min || duration > c_M1max) {                    // Not a Bit Mark duration
        if (frameCount == 3) _state = 0;                                 // Duration error in frame 3
        else {                                                           // Not a Bit Mark duration, possibly a Gap at frame 1 or frame 2
          if (duration < c_GapMin || duration > c_GapMax) _state = 0;    // Duration error
          else {                                                         // It's a Gap at the end of frame 1 or frame 2
            if (frameCount == 1) {                                       // Frame 1 received
              if (bitCount == 12 || bitCount == 15 || bitCount == 20) {  // bitCount confirmed, prep for frame 2
                firstBitCount = bitCount;
                bitCount = 0;
                firstCode = irSignal.all;
                irSignal.all = 0;
                frameCount = 2;
              } else _state = 0;                // BitCount error
            } else {                            // Frame 2 received
              if (irSignal.all == firstCode) {  // Code OK, prep for frame 3
                bitCount = 0;
                irSignal.all = 0;
                frameCount = 3;
              } else _state = 0;                 // Code error at end of frame 2
            }
          }
        }
      } else {                                             // It's a Bit Mark duration
        irSignal.all >>= 1;                                // Push a 0 from left to right (will be left at 0 if it's M0)
        if (duration >= c_M1min) irSignal.byt[3] |= 0x80;  // It's M1, change MSB to 1
        bitCount++;
        if (frameCount == 3) {
          if (bitCount == firstBitCount) {                         // All bits of frame 3 received
            if (!_irCopyingData && (irSignal.all == firstCode)) {  // If not interrupting a copy and the code is OK, decode the data; otherwise, discard it
              if (bitCount == 12) {
                irSignal.all >>= 3;
                irSignal.byt[2] >>= 1;
                _irData.addr = irSignal.byt[3];
                _irData.cmd = irSignal.byt[2];
                _irData.ext = 0;
              } else if (bitCount == 15) {
                irSignal.byt[2] >>= 1;
                _irData.addr = irSignal.byt[3];
                _irData.cmd = irSignal.byt[2];
                _irData.ext = 0;
              } else {  // It's 20 bits
                _irData.ext = irSignal.byt[3];
                irSignal.byt[3] = 0;
                irSignal.all >>= 3;
                irSignal.byt[1] >>= 1;
                _irData.addr = irSignal.byt[2];
                _irData.cmd  = irSignal.byt[1];
              }
              _irData.keyHeld = false;
              _irDataAvailable = true;
              possiblyHeld = true;  // Will remain true if the next gap is OK
            }
            repeatCount = 0;
            _state = 0;  // Done
          }              // Else, remain in this state (continue receiving frame 3)
        } else {         // It's frame 1 or 2. Check if a key was held
          if (frameCount == 1 && possiblyHeld && bitCount == firstBitCount && irSignal.all == firstCode) {  // A key was Held
            if (repeatCount < c_RptCount) repeatCount++;  // First repetitions will be ignored
            else if (!_irCopyingData) {                   // If not interrupting a copy then keyHeld...
              _irData.keyHeld = true;
              _irDataAvailable = true;
            }
            _state = 0;
          }
        }
      }
    break;  // End of case 1 (Receiving)
  }
  DBG_PRINTLN_TIMER();
  DBG_PRINT_STATE(_state);
}


/* Decoding process using 32 bits data for all cases (bit order already reversed)
 * 
 * 12 bits:                byt[3] (high)                byt[2]                 byt[1]         byt[0] (low)
 * encoded bits:      A4 A3 A2 A1 A0 C6 C5 C4   C3 C2 C1 C0  0  0  0  0   0 0 0 0 0 0 0 0   0 0 0 0 0 0 0 0
 * all >> 3            0  0  0 A4 A3 A2 A1 A0   C6 C5 C4 C3 C2 C1 C0  0   0 0 0 0 0 0 0 0   0 0 0 0 0 0 0 0
 * byt[2] >> 1         0  0  0 A4 A3 A2 A1 A0    0 C6 C5 C4 C3 C2 C1 C0   0 0 0 0 0 0 0 0   0 0 0 0 0 0 0 0
 *
 *
 * 15 bits:                byt[3] (high)                byt[2]                 byt[1]         byt[0] (low)
 * encoded bits:      A7 A6 A5 A4 A3 A2 A1 A0   C6 C5 C4 C3 C2 C1 C0  0   0 0 0 0 0 0 0 0   0 0 0 0 0 0 0 0
 * byt[2] >> 1        A7 A6 A5 A4 A3 A2 A1 A0    0 C6 C5 C4 C3 C2 C1 C0   0 0 0 0 0 0 0 0   0 0 0 0 0 0 0 0
 *
 *
 * 20 bits:                byt[3] (high)                byt[2]                    byt[1]              byt[0] (low)
 * encoded bits:      E7 E6 E5 E4 E3 E2 E1 E0   A4 A3 A2 A1 A0 C6 C5 C4   C3 C2 C1 C0  0  0  0  0   0 0 0 0 0 0 0 0
 * extract ext code    0  0  0  0  0  0  0  0   A4 A3 A2 A1 A0 C6 C5 C4   C3 C2 C1 C0  0  0  0  0   0 0 0 0 0 0 0 0
 * all >> 3            0  0  0  0  0  0  0  0    0  0  0 A4 A3 A2 A1 A0   C6 C5 C4 C3 C2 C1 C0  0   0 0 0 0 0 0 0 0
 * byt[1] >> 1         0  0  0  0  0  0  0  0    0  0  0 A4 A3 A2 A1 A0    0 C6 C5 C4 C3 C2 C1 C0   0 0 0 0 0 0 0 0
*/

