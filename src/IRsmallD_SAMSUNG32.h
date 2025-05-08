/* IRsmallD_SAMSUNG32 - SAMSUNG 32-bit signal protocol decoder
 *
 * This file is part of the IRsmallDecoder library for Arduino
 * Copyright (c) 2020 Luis Carvalho
 *
 *
 * Protocol specifications:
 * ------------------------
 * Encoding type: Pulse Distance
 * Carrier frequency:    37.9 kHz
 * Leading Mark length:  9000 µs (=4500 µs pulse + 4500 µs space)
 * Bit '0' Mark length:  1125 µs (=562.5 µs pulse + 562.5 µs space) 
 * Bit '1' Mark length:  2250 µs (=562.5 µs pulse + 1687.5 µs space)
 *
 * Repetition period: 108000 µs
 * Signal length:      54562.5  up to  72562.5 µs  (= 2x4500 + 2x8x[1125 to 2250] + 8x(1125 + 2250) + 562.5)
 * Stop space length:  53437.5 down to 35437.5 µs  (= Repetition Period - Signal length)
 * Frames per key press: 1
 * Repetition mode: Exact Copy
 * Number of bits: 32 (but only 16 are used, address is repeated, and the command is followed by its complement)
 * Logical bits' order of transmission:
 *      8 bit Address            same 8 bit Address           8 bit Command             Command Complement
 *  A0 A1 A2 A3 A4 A5 A6 A7    A0 A1 A2 A3 A4 A5 A6 A7    C0 C1 C2 C3 C4 C5 C6 C7    ~C0~C1~C2~C3~C4~C5~C6~C7  
 *
 * (Source: https://www.scribd.com/doc/283100715/S3F80KB-RemoteController-an-REV000-090108-0)
 *
 */
 
// SAMSUNG32 timing in microseconds:
#define LEADING_MARK    9000
#define BIT_0_MARK      1125
#define BIT_1_MARK      2550
#define BIT_TOLERANCE   ((BIT_1_MARK - BIT_0_MARK) / 2)  // = 712
#define STOP_SPACE_MIN 35437       // 35437.5 µs to be more precise
#define STOP_SPACE_MAX 72563       // 72562.5 µs to be more precise


void IR_ISR_ATTR IRsmallDecoder::irISR() {  // Triggered on each falling edge of the IR receiver output signal.
  // The signal goes LOW when IR light is detected (so the falling edge marks the beginning of an IR pulse).

  // SAMSUNG32 timing thresholds:
  const uint16_t c_LMmax = LEADING_MARK * 1.1;          // 10% more = 9900
  const uint16_t c_LMmin = LEADING_MARK * 0.9;          // 10% less = 8100
  const uint16_t c_M1max = BIT_1_MARK + BIT_TOLERANCE;  // 2550+712=3262
  const uint16_t c_M1min = BIT_1_MARK - BIT_TOLERANCE;  // 2550-712=1838
  const uint16_t c_M0min = BIT_0_MARK - BIT_TOLERANCE;  // 1125-712= 413
  const uint32_t c_GapMax = STOP_SPACE_MAX + 6 * BIT_TOLERANCE;  // Bigger tolerance
  const uint16_t c_GapMin = STOP_SPACE_MIN - 6 * BIT_TOLERANCE;  // 6 x 712 = 4272

  // Number of initial repetitions to be ignored:
  const uint8_t  c_RptCount = 2;

  // FSM variables:
  static uint32_t duration;
  static uint8_t  bitCount;
  static uint8_t  irSignal[4];
  static uint8_t  byteIndex = 0;
  static uint8_t  repeatCount = 0;
  static bool possiblyHeld = false;

  DBG_RESTART_TIMER();
  
  duration = micros() - _previousTime;
  _previousTime = micros();
  DBG_PRINTLN_DUR(duration);

  switch (_state) {  // Asynchronous (event-driven) Finite State Machine
    case 0:  // Standby:
      if (duration > c_GapMin) {
        if (duration > c_GapMax) possiblyHeld = false;
        _state = 1;
      } else possiblyHeld = false;
    break;

    case 1:  // StartPulse:
      if (duration >= c_LMmin && duration <= c_LMmax) {  // It's a Leading Mark
        bitCount = 0;
        byteIndex = 0;
        _state = 2;
      } else _state = 0;
    break;

    case 2:  // Receiving:
      if (duration < c_M0min || duration > c_M1max) _state = 0; // Error: not a bit mark
      else {                                                    // It's M0 or M1
        irSignal[byteIndex] >>= 1;                              // Push a 0 from left to right (will be left at 0 if it's M0)
        if (duration >= c_M1min) irSignal[byteIndex] |= 0x80;   // It's M1, change MSB to 1
        bitCount++;
        if (bitCount == 8 || bitCount == 16 || bitCount == 24) byteIndex++;          // Byte full, proceed to the next one (stay in same state)
        else if (bitCount == 32) {                                                   // All bits received,
          _state = 0;                                                                // All paths lead to the standby state...
          if (irSignal[0] == irSignal[1] && irSignal[2] == (uint8_t)~irSignal[3]) {  // Address OK && command OK,
            if (possiblyHeld && (irSignal[2] == _irData.cmd)) {                      // Key Held confirmed (cmd didn't changed)
              if (repeatCount < c_RptCount) repeatCount++;                           // First repeat signals will be ignored
              else if (!_irCopyingData) {                                            // Repetitions ignored;  if not interrupting a copy, update data
                _irData.keyHeld = true;
                _irDataAvailable = true;
              }
            } else if (!_irCopyingData) {  // Key was not held; if allowed, update data; otherwise, discard it
              _irData.addr = irSignal[0];
              _irData.cmd  = irSignal[2];
              _irData.keyHeld = false;
              _irDataAvailable = true;
              possiblyHeld = true;  // Will remain true if the next gap is OK
              repeatCount = 0;
            }
          }
        }
        // Else, remain in this state (continue receiving)
      }
    break;
  }

  DBG_PRINTLN_TIMER();
  DBG_PRINT_STATE(_state);
}


/*
SAMSUNG32 protocol: 
-------------------
  Bits in order of transmission: A0 A1 A2 A3 A4 A5 A6 A7   A0 A1 A2 A3 A4 A5 A6 A7   C0 C1 C2 C3 C4 C5 C6 C7  ~C0~C1~C2~C3~C4~C5~C6~C7
  
  Decoding process using irSignal array and byteIndex:
     uint8_t irSignal[4]
     uint8_t byteIndex = 0
     
     Fill  irSignal[byteIndex]  (8 bit Address)
     byteIndex++
     Fill  irSignal[byteIndex]  (8 bit Address repeated)
     byteIndex++
     Fill  irSignal[byteIndex]  (8 bit Command)
     byteIndex++
     Fill  irSignal[byteIndex]  (Command Complement)
     Check if addr is OK
     Check if cmd is OK
     
     irSignal array completely filled:
     
           irSignal[3]                  irSignal[2]              irSignal[1]              irSignal[0]
     ~C7~C6~C5~C4~C3~C2~C1~C0   C7 C6 C5 C4 C3 C2 C1 C0   A7 A6 A5 A4 A3 A2 A1 A0   A7 A6 A5 A4 A3 A2 A1 A0
 
     irSignal[0] and irSignal[2] are the ones that matter.
 */