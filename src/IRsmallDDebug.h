/* IRsmallDDebug - Debug macros
 *
 * This file is part of the IRsmallDecoder library for Arduino
 * Copyright (c) 2020 Luis Carvalho
 *
 *
 * Debug options:
 *   IRSMALLD_DEBUG_STATE    - Prints FSM states.
 *   IRSMALLD_DEBUG_INTERVAL - Prints the durations of the intervals between consecutive interrupts (in µs).
 *   IRSMALLD_DEBUG_ISRTIME  - Prints ISR execution time (in µs).
 *   IRSMALLD_DEBUG_ISRAVG   - Prints the average time taken by the ISR to execute (in µs).
 * 
 * NOTES:
 * - The usage of debugging functionalities requires a Serial.begin() in the setup;
 * - The serial communication speed must be high, to avoid timing errors (a Baud Rate of 115200 seems to work;
 * - The usage of Serial communications inside an interrupt is not recommended, but in this case
 *   it's just a few prints for debugging purposes.
 * - IRSMALLD_DEBUG_ISRTIME and IRSMALLD_DEBUG_ISRAVG modes uses AVR 328p Timer1 hardware-specific code;
 *   These debug modes reconfigure Timer1, which will interfere with any other code using that timer.
 * - Results are in microseconds, assuming a 16MHz clock (Clk Count is divided by 16 to get µs);
 * - IRSMALLD_DEBUG_ISRAVG shows the average of all ISR execution times since the last reset.
 *   To restart the average calculation you'll need to reset the Arduino or restart the serial monitor.
 * - Call Serial.flush() after every Serial.print() if you need to see the output 
 *   before the code continues with buffered data.
 */


#ifndef IRsmallD_Debug_h
  #define IRsmallD_Debug_h

  // Check Debug incompatibilities:
  #if defined(IRSMALLD_DEBUG_INTERVAL) && (defined(IRSMALLD_DEBUG_ISRTIME) || defined(IRSMALLD_DEBUG_ISRAVG))
      #error Do not use IRSMALLD_DEBUG_ISRTIME or IRSMALLD_DEBUG_ISRAVG with IRSMALLD_DEBUG_INTERVAL, the ISR execution time is significantly affected by the print(uint32_t)
  #elif defined(IRSMALLD_DEBUG_STATE) && (defined(IRSMALLD_DEBUG_ISRTIME) || defined(IRSMALLD_DEBUG_ISRAVG))
      #warning Do not use IRSMALLD_DEBUG_ISRTIME or IRSMALLD_DEBUG_ISRAVG with IRSMALLD_DEBUG_STATE if you want accurate measurements of execution time
  #endif 

  // FSM states debug:
  #ifdef IRSMALLD_DEBUG_STATE
    #define DBG_PRINT_STATE(...)  Serial.print(__VA_ARGS__);
  #else
    #define DBG_PRINT_STATE(...)   //nothing
  #endif

  // Signals' intervals' duration:
  #ifdef IRSMALLD_DEBUG_INTERVAL
    #if defined(IRSMALLD_DEBUG_ISRTIME) || defined(IRSMALLD_DEBUG_ISRAVG)
      #define DBG_PRINTLN_DUR(...)  {Serial.print(" i"); Serial.print(__VA_ARGS__);}
    #else
      #define DBG_PRINTLN_DUR(...)  {Serial.print(" i"); Serial.println(__VA_ARGS__);}
    #endif
  #else
    #define DBG_PRINTLN_DUR(...)   //nothing
  #endif

  // ISR Execution Time and Average:
  #if defined(IRSMALLD_DEBUG_ISRTIME) && defined(IRSMALLD_DEBUG_ISRAVG)  // Time and Average
      #define DBG_RESTART_TIMER() TCCR1A = 0; TCCR1B = 1; TCNT1=0;     \
                                  static uint16_t dbg_isrCallsCount=0; \
                                  static uint16_t dbg_isrTimeAccum=0;
      #define DBG_PRINTLN_TIMER() uint16_t dbg_elapsedTime=TCNT1>>4; Serial.print(" t"); Serial.print(dbg_elapsedTime);  \
                                  dbg_isrTimeAccum += dbg_elapsedTime; dbg_isrCallsCount += 1; Serial.print(" a");       \
                                  Serial.println((float)dbg_isrTimeAccum / dbg_isrCallsCount);

  #elif defined(IRSMALLD_DEBUG_ISRTIME)  // just Time, no Average
      #define DBG_RESTART_TIMER() TCCR1A = 0; TCCR1B = 1; TCNT1=0;   
      #define DBG_PRINTLN_TIMER() uint16_t dbg_elapsedTime=TCNT1>>4; Serial.print(" t"); Serial.println(dbg_elapsedTime);

  #elif defined(IRSMALLD_DEBUG_ISRAVG)  // just Average, no Time
      #define DBG_RESTART_TIMER() TCCR1A = 0; TCCR1B = 1; TCNT1=0;     \
                                  static uint16_t dbg_isrCallsCount=0; \
                                  static uint16_t dbg_isrTimeAccum=0;
      #define DBG_PRINTLN_TIMER() dbg_isrTimeAccum += TCNT1>>4; dbg_isrCallsCount += 1; Serial.print(" a");  \
                                  Serial.println((float)dbg_isrTimeAccum / dbg_isrCallsCount);
  #else  // no Time and no Average
    #define DBG_RESTART_TIMER()  //nothing
    #define DBG_PRINTLN_TIMER()  //nothing
  #endif

#endif
