/* IRsmallD_SAMSUNG - SAMSUNG old standard protocol decoder
 *
 * This file is part of the IRsmallDecoder library for Arduino
 * Copyright (c) 2020 Luis Carvalho
 *
 *
 * Protocol specifications:
 * ------------------------
 * Encoding type: Pulse Distance
 * Carrier frequency:    37.9 kHz
 * Leading Mark length:  9000 µs (=4500µs pulse + 4500µs space)
 * Bit '0' Mark length:  1125 µs (=562.5µs pulse + 562.5µs space) 
 * Bit '1' Mark length:  2250 µs (=562.5µs pulse + 1687.5µs space)
 *
 * Repetition Period:   60000 µs
 * Signal length : from 32062.5  up to  54562.5 µs  (= 2x4500 + 20x[1125 to 2250]+ 562.5) 
 * Stop Space Length:   27937.5 down to  5437.5 µs  (= Repetition Period - Signal length)
 * Frames per key press: 2 (the message is always sent at least twice). The decoder will ignore it!
 *                       Repetitions are not used for error checking. 
 *                       Note that not all remotes share this characteristic.         
 * Repetition Mode:      Exact Copy (not a NEC type repetition frame)
 * Bit order:            LSB first
 * Number of bits:       20 (12 for manufacturer code + 8 for command)
 * Logical bits' order of transmission:
 *             12 bit Address                  8 bit Command       
 *  A0 A1 A2 A3 A4 A5 A6 A7 A8 A9 Aa Ab    C0 C1 C2 C3 C4 C5 C6 C7   
 * 
 * Sources: 
 *   https://www.mikrocontroller.net/attachment/55409/samsungRCProtokoll.pdf (page 5-30)
 *   https://www.handsontec.com/pdf_files/IR_Code_Analy.pdf
 *
 */

// SAMSUNG Standard timing in microseconds:
#define LEADING_MARK    9000
#define BIT_0_MARK      1125
#define BIT_1_MARK      2550
#define BIT_TOLERANCE   ((BIT_1_MARK - BIT_0_MARK)/2)  // = 712
#define STOP_SPACE_MIN  5437       //  5437.5 µs to be more precise
#define STOP_SPACE_MAX 27938       // 27937.5 µs to be more precise


void IR_ISR_ATTR IRsmallDecoder::irISR() {  // Triggered on each falling edge of the IR receiver output signal.
  // The signal goes LOW when IR light is detected (so the falling edge marks the beginning of an IR pulse).

  // SAMSUNG timing thresholds:
  const uint16_t c_LMmax = LEADING_MARK * 1.1;           // 10% more = 9900
  const uint16_t c_LMmin = LEADING_MARK * 0.9;           // 10% less = 8100
  const uint16_t c_M1max = BIT_1_MARK + BIT_TOLERANCE;   // 2550+712=3262
  const uint16_t c_M1min = BIT_1_MARK - BIT_TOLERANCE;   // 2550-712=1838
  const uint16_t c_M0min = BIT_0_MARK - BIT_TOLERANCE;   // 1125-712= 413  
  const uint32_t c_GapMax = STOP_SPACE_MAX + 6 * BIT_TOLERANCE;  // bigger tolerance
  const uint16_t c_GapMin = STOP_SPACE_MIN - 6 * BIT_TOLERANCE;  // 6 x 712 = 4272

  // Number of initial repetitions to ignore:
  const uint8_t c_RptCount = 3;   

  // FSM variables:
  static uint32_t duration;
  static uint8_t  bitCount;
  static uint8_t  signal_Cmd;      // Starts as an auxiliary Byte for address decoding
  static uint16_t signal_Addr16;
  static uint8_t  repeatCount = 0;
  static bool     possiblyHeld = false;

  DBG_RESTART_TIMER();
  
  duration = micros() - _previousTime;
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
      if (duration >= c_LMmin && duration <= c_LMmax) { // It's a Leading Mark
        bitCount = 0;
        _state = 2;
      }
      else _state = 0;
    break;
 
    case 2:  // Receiving:
      if (duration < c_M0min || duration > c_M1max) _state = 0; // Error: not a bit mark
      else {                                                    // It's M0 or M1
        signal_Cmd >>= 1;                                       // Push a 0 from left to right (remains 0 if it's M0)
        if (duration >= c_M1min) signal_Cmd |= 0x80;            // It's M1, change MSB to 1
        bitCount++;
        if (bitCount == 8) signal_Addr16 = signal_Cmd;      // Set address low byte (and stay in same state)
        else if (bitCount == 12) {
          signal_Cmd >>= 4;                                 // Push 4 '0' bits to the right
          signal_Addr16 |= signal_Cmd << 8;                 // Set address high byte (and stay in same state)
        } else if (bitCount == 20) {                        // All bits received,
          if (possiblyHeld && signal_Cmd == _irData.cmd) {  // Key Held confirmed (addr shouldn't have changed)
            if (repeatCount < c_RptCount) repeatCount++;    // First repeat signals will be ignored
            else if (!_irCopyingData) {                     // Repetitions ignored; if not interrupting a copy, update data
              _irData.keyHeld = true;
              _irDataAvailable = true;
            }
          } else if (!_irCopyingData) {  // Key was not held; if allowed, update the data; otherwise discard it
            _irData.addr = signal_Addr16;
            _irData.cmd = signal_Cmd;
            _irData.keyHeld = false;
            _irDataAvailable = true;
            possiblyHeld = true;  // Will remain true if the next gap is OK
            repeatCount = 0;
          }
          _state = 0;  // Done
        }
        // Else, remain in this state (continue receiving)
      }
    break;
  }

  DBG_PRINTLN_TIMER();
  DBG_PRINT_STATE(_state);
}


/*
Bits in order of transmission: A0 A1 A2 A3 A4 A5 A6 A7 A8 A9 Aa Ab C0 C1 C2 C3 C4 C5 C6 C7
 
Decoding process using command byte as an auxiliary byte to decode the address:
  Store A0 to A7 in signal_Cmd:        A7 A6 A5 A4 A3 A2 A1 A0
  Set address low byte to signal_Cmd:  signal_Addr16 = signal_Cmd
  Store A8 to Ab in signal_Cmd:        Ab Aa A9 A8 A7 A6 A5 A4
  Push 4 '0' bits to the right:         0  0  0  0 Ab Aa A9 A8
  Set address high byte to signal_Cmd: signal_Addr16 |= signal_Cmd << 8
  Store C0 to C7 in signal_Cmd:        C7 C6 C5 C4 C3 C2 C1 C0
 */