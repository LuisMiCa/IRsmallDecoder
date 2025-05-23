/* IRsmallD_RC5 - Philips RC5 protocol decoder
 *
 * This file is part of the IRsmallDecoder library for Arduino
 * Copyright (c) 2020 Luis Carvalho
 *
 *
 * Protocol specifications:
 * ------------------------
 * Modulation type: Manchester code (bi-phase) - LOW to HIGH for ACE (1), HIGH to LOW for ZERO (0)
 * Carrier frequency: 36 KHz
 * Bit period: 1.778 ms (HIGH for 889μs then LOW for 889μs or vice-versa)
 * When the bits are combined, the signal pulses' durations are 889μs or 1778μs, and the same goes for the spaces.
 * Total signal duration: 24.892 ms 
 * Signal repetition interval: 100 ms
 *
 * 14 bit signal (in order of transmission):
 * 2 Start bits; 1 Toggle bit; 5 Address bits (MSB to LSB); 6 Command bits (MSB to LSB).
 *
 * In RC5-extended, the second Start bit is called the Field bit. It indicates whether the command is in the
 * lower field (bit 1: cmd = [0..63]) or upper field (bit 0: cmd = [64..127]).
 * This was added later because 64 commands per device were insufficient.
 * So in RC5-extended, the Field bit becomes the 7th cmd bit (the MSB), but is reversed because it is 1 in normal RC5.
 *
 * The gap between two successive frames is defined as 50 bits wide: 50 x 1778 µs = 88.9 ms.
 * The frame repetition period is 64 × 1-bit width: 64 × 1778 µs = 113.792 ms.
 */


void IR_ISR_ATTR IRsmallDecoder::irISR() {  // Triggered on every change in the IR receiver's output signal level.
  // Due to INPUT_PULLUP mode, the signal goes LOW when IR light is detected (i.e., it is inverted).

  // RC5 timings in micro secs:
  const uint32_t c_rptPmax   = 113792 * 1.2;  // Repetition period upper threshold (20% above standard)
  const uint32_t c_gapMin    = 88900 * 0.8;   // Lower threshold of the gap between 2 signals (20% below standard)
  const uint16_t c_bitPeriod = 1778;
  const uint16_t c_tolerance = 444;                            // Maximum tolerance is 1778/4 = 444.5
  const uint16_t c_longMax   = c_bitPeriod + c_tolerance;      // 1778 + 444 = 2222
  const uint16_t c_shortMax  = c_bitPeriod / 2 + c_tolerance;  // 1778/2+444 = 1333
  const uint16_t c_shortMin  = c_bitPeriod / 2 - c_tolerance;  // 1778/2-444 =  445

  // Number of initial repetitions to be ignored:
  const uint8_t c_rptCount = 2;

  // FSM variables:
  static uint32_t duration;
  static uint8_t  bitCount;
  static uint16_t irSignal;            // Only 14 bits used
  static bool     prevToggle = false;  // Used to convert Toggle to Held
  static uint8_t  repeatCount = 0;
  static uint32_t lastBitTime = 0;     // For the repeat code confirmation

  FSM_INITIALIZE(st_standby);  // Initialize the "hidden" variable fsm_state (only once)

  DBG_RESTART_TIMER();
  duration = micros() - _previousTime;
  _previousTime = micros();
  DBG_PRINTLN_DUR(duration);

  FSM_SWITCH(){  // Asynchronous (event-driven) Finite State Machine, implemented with computed GOTOs
    // ====> States: st_standby, st_roseInSync, st_roseOffSync, st_fellInSync, st_fellOffSync
    st_standby:  // State 0
      if (duration >= c_gapMin) {  //start pulse detected. It's very unlikely that a non-start pulse will be longer than c_gapMin
        bitCount = 0;
        irSignal = 0;
        FSM_NEXT(st_roseInSync);
        DBG_PRINT_STATE(1);
      }
    break;
    
    st_roseInSync:  // State 1
      irSignal <<= 1; irSignal += 1;  // push Bit 1 (from right to left)
      bitCount++;
      if(duration < c_shortMin || duration > c_longMax){      //error
        FSM_NEXT(st_standby);
        DBG_PRINT_STATE(0);
      }
      else if(duration <= c_shortMax) {                       //it's Short
        FSM_NEXT(st_fellOffSync); 
        DBG_PRINT_STATE(4);
      }
      else FSM_DIRECTJUMP(ps_roseChoice);                     //it's Long
    break;
    
    st_roseOffSync:  // State 2
      if (duration < c_shortMin || duration > c_shortMax) {   //error
        FSM_NEXT(st_standby);
        DBG_PRINT_STATE(0);
      }
      else FSM_DIRECTJUMP(ps_roseChoice);                     //it's Short
    break;
      
    st_fellInSync:  // State 3
      irSignal <<= 1;  // push Bit 0 (from right to left)
      bitCount++;
      if (duration < c_shortMin || duration > c_longMax) {    //error
        FSM_NEXT(st_standby); 
        DBG_PRINT_STATE(0);
      }
      else if (duration <= c_shortMax) {                      //it's Short
        FSM_NEXT(st_roseOffSync);
        DBG_PRINT_STATE(2);
      }
      else FSM_DIRECTJUMP(ps_fellChoice);                     //it's Long
    break;
      
    st_fellOffSync:  // State 4
      if (duration < c_shortMin || duration > c_shortMax) {   //error
        FSM_NEXT(st_standby);
        DBG_PRINT_STATE(0);
      }
      else FSM_DIRECTJUMP(ps_fellChoice);                     //it's Short
    break;
      
    // ====> Pseudo-states: ps_roseChoice, ps_fellChoice, ps_decode
    ps_roseChoice:  // State r
      DBG_PRINT_STATE("r");
      if (bitCount==13) {            // All 14 bits received
        irSignal <<= 1;              // Push bit 0 from right to left
        FSM_DIRECTJUMP(ps_decode);   // ps_decode will handle the rest...
      }
      else {
        FSM_NEXT(st_fellInSync);
        DBG_PRINT_STATE(3);
      }
    break;
    
    ps_fellChoice:  // State f
      DBG_PRINT_STATE("f");
      if (bitCount==13) {                // All 14 bits received     
        irSignal <<= 1;  irSignal += 1;  // Push bit 1 from right to left
        FSM_DIRECTJUMP(ps_decode);       // ps_decode will handle the rest...
      }
      else {
        FSM_NEXT(st_roseInSync);
        DBG_PRINT_STATE(1);
      }
    break;
    
    ps_decode:  // State d
      DBG_PRINT_STATE("d");
      if (!_irCopyingData) {  // If not interrupting a copy, decode the signal; otherwise, discard it.
        // If the period is OK and the toggle bit did not change, then the key was held:
        if (_previousTime - lastBitTime < c_rptPmax && (prevToggle == bool(irSignal & 0x0800))) {
          if (repeatCount < c_rptCount) repeatCount++;
          else {  // Initial repetitions have already been ignored
            _irData.keyHeld = true;
            _irDataAvailable = true;
          }
        } else {  // Key was not held, decode the signal:
          _irData.addr = (irSignal & 0x7C0) >> 6;
          _irData.cmd = (irSignal & 0x3F) | ((irSignal & 0x1000) ? 0 : 0x40);  // Extract cmd and add field bit (inverted)
          _irData.keyHeld = false;
          _irDataAvailable = true;
          repeatCount = 0;
        }
        prevToggle = bool(irSignal & 0x0800);
        lastBitTime = _previousTime;  // Last bit transition time (for keyHeld confirmation)
      }
      FSM_NEXT(st_standby);
      DBG_PRINT_STATE(0);
    break;
  }

  _state = fsm_state != &&st_standby;  // Needed just for the timeout check. Will be false (0) if the FSM is in standby state
  DBG_PRINTLN_TIMER();
}

/*
Bit masks:
bit position:  13 12 11 10 09 08 07 06 05 04 03 02 01 00
code bits:      S  F  T A4 A3 A2 A1 A0 C5 C4 C3 C2 C1 C0
toggle mask:    0  0  1  0  0  0  0  0  0  0  0  0  0  0  = 0x800
address mask:   0  0  0  1  1  1  1  1  0  0  0  0  0  0  = 0x7C0
command mask:   0  0  0  0  0  0  0  0  1  1  1  1  1  1  = 0x3F
Field bit(~C6): 0  1  0  0  0  0  0  0  0  0  0  0  0  0  = 0x1000
C6 relocated:   0  0  0  0  0  0  0  1  0  0  0  0  0  0  = 0x40
*/ 