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
 * Each pulse burst is 562.5µs in length
 * Signal starts with a 9ms leading pulse burst (16 times the pulse burst length used for a logical data bit)
 * followed by a 4.5ms space. Then, the logical bits are transmitted as follows:
 *   Logical '0' – a 562.5µs pulse burst followed by a 562.5µs space, with a total transmit time of 1125 μs
 *   Logical '1' – a 562.5µs pulse burst followed by a 1687.5μs space, with a total transmit time of 2250 μs
 *
 * Logical bits' order of transmission for normal NEC protocol:
 * 8bit Address    inverted address    8bit command    inverted command
 * LSB ... MSB       LSB ... MSB       LSB ... MSB       LSB ... MSB
 *
 * Logical bits' order of transmission for Extended NEC protocol:
 * low Address       high address      8bit command      inverted command
 * LSB ... MSB       LSB ... MSB       LSB ... MSB       LSB ... MSB 
 *
 * 
 * The signal ends with an extra 560µs pulse, which is required to determine the value of the last bit.
 * 
 * Notes:
 *   - Inverted bytes have 1s instead of 0s and vice-versa;
 *   - Extended NEC protocol sacrifices the address redundancy to extend 
 *     the address from 8 to 16 bits (inverted address becomes high address).
 *
 * Repeat codes:
 * If a Key is held, a repeat bit is sent every 108ms;
 * It consists of a 9ms pulse burst followed by a 2250 μs space and then a 562.5 μs pulse burst.
 */


// NEC/NECx timings in microseconds:
#define NEC_L_MARK 5062.5  /* Leading Mark */
#define NEC_R_MARK 2812.5  /* Repeat Mark */
#define NEC_R_TOL   803.6  /* Repeat Mark tolerance (µs) */
#define NEC_MARK_0  1125   /* Bit 0 Mark */
#define NEC_MARK_1  2250   /* Bit 1 Mark */
#define NEC_GAP_1  48937.5 /* Gap1 (before first repeat mark) */
#define NEC_GAP_2 105187.5 /* Gap2 (between repeat marks */
// For more information about these timings, go to:
// https://github.com/LuisMiCa/IRsmallDecoder/blob/master/extras/Timings/NEC_timings.svg


void IRsmallDecoder::irISR() { //executed every time the IR signal goes down (but it's actually RISING @ ReceiverOutput)
  const uint16_t c_GapMin = NEC_GAP_1 * 0.7;        // 34256
  const uint32_t c_GapMax = NEC_GAP_2 * 1.3;        //136743
  const uint16_t c_RMmin = NEC_R_MARK * 0.7;        //  1968
  const uint16_t c_RMmax = NEC_R_MARK + NEC_R_TOL;  //  3616
  const uint16_t c_LMmin = c_RMmax + 1;             //  3617
  const uint16_t c_LMmax = NEC_L_MARK * 1.3;        //  6581
  const uint16_t c_M1min = NEC_MARK_1 * 0.7;        //  1575
  const uint16_t c_M1max = NEC_MARK_1 * 1.3;        //  2925
  const uint16_t c_M0min = NEC_MARK_0 * 0.7;        //   787
  const uint16_t c_M0max = NEC_MARK_0 * 1.3;        //  1462

  //number of initial repetition marks to be ignored:
  const uint8_t c_RptCount = 2;

  // FSM variables:
  static uint32_t duration;
  static uint8_t bitCount;
  static uint32_t startTime = -1;  //FFFF...  (by two's complement)
  static union {                   //received bits are stored in reversed order (11000101... -> ...10100011)
    uint32_t all = 0;              //Arduino uses Little Endian so, if all=ABCDEF89 then in memory it's 89EFCDAB (hex format)
    uint8_t byt[4];                //then we get byt[0]=89, byt[1]=EF, byt[2]=CD and byt[3]=AB (type punning with a union...)
  } irSignal;
  static uint8_t repeatCount = 0;
  static uint8_t state = 0;
  static bool possiblyHeld = false;

  DBG_PRINT_STATE(state);
  DBG_RESTART_TIMER();

  duration = micros() - startTime;  //note: micros() has a 4μs resolution (multiples of 4) @ 16MHz or 8μs @ 8MHz
  startTime = micros();
  DBG_PRINTLN_DUR(duration);

  switch (state) {  //asynchronous (event-driven) Finite State Machine
    case 0:  //standby:
      if (duration > c_GapMin) {
        if (duration > c_GapMax) possiblyHeld = false;
        state = 1;
      } 
      else possiblyHeld = false;
    break;

    case 1:  //startPulse:
      if (duration >= c_LMmin && duration <= c_LMmax) {  //its a Leading Mark
        bitCount = 0;
        repeatCount = 0;
        state = 2;
      } else {
        if (possiblyHeld && duration >= c_RMmin && duration <= c_RMmax) {  //its a Repeat Mark
          if (repeatCount < c_RptCount) repeatCount++;  //first repeat signals will be ignored
          else if (!_irCopyingData) {                   //if not interrupting a copy...
            _irData.keyHeld = true;
            _irDataAvailable = true;
          }
        }
        state = 0;
      }
    break;

    case 2: //receiving:
      if (duration < c_M0min || duration > c_M1max) state = 0;  //error, not a bit mark
      else {                // it's M0 or M1
        irSignal.all >>= 1; //push a 0 from left to right (will be left at 0 if it's M0)
        if (duration >= c_M1min) irSignal.byt[3] |= 0x80; //it's M1, change MSB to 1
        bitCount++;
        #if defined(IR_SMALLD_NEC) //Conditional code inclusion (at compile time)
          if (bitCount == 16) {    //Address and Reversed Address received
            if (irSignal.byt[2] != (uint8_t)~irSignal.byt[3]) state = 0;  //address error
            // else state = 2;  //Address OK, continue with command reception //(redundant assignment)
          }
          else   // that's right, a loose else...
    		#endif
  		  if (bitCount == 32) { //all bits received
          if (!_irCopyingData && (irSignal.byt[2] == (uint8_t)~irSignal.byt[3])) { //if not interrupting a copy and command OK, finish decoding
            #if defined(IR_SMALLD_NEC) //NEC address has 8 bits
              _irData.addr = irSignal.byt[0];
            #else //it must be IR_SMALLD_NECx (16 bits)
              _irData.addr = *(uint16_t*)(&irSignal.byt[0]); // type punning with cast...
              //.addr is the 16 bit value pointed by the address of byt[0]  (byt[1] is addr high byte)
              // dereferencing type-punned pointer will break strict-aliasing rules
            #endif
        	  _irData.cmd = irSignal.byt[2];
            _irData.keyHeld = false;
            _irDataAvailable = true;
            possiblyHeld = true;  //will remain true if the next gap is OK
          }
          state = 0;
        }
        // else state = 2; //continue receiving //(redundant assignment)
      }
    break;
  }

  DBG_PRINTLN_TIMER();
}
