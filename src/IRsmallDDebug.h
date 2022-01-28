/* IRsmallDDebug - Debug macros
 *
 * This file is part of the IRsmallDecoder library for Arduino
 * Copyright (c) 2020 Luis Carvalho
 *
 *
 * Debug options:
 *   IRSMALLD_DEBUG_STATE    - Prints FSM states
 *   IRSMALLD_DEBUG_INTERVAL - Prints interval's duration between consecutive interrupts (in µs)
 *   IRSMALLD_DEBUG_ISRTIME  - Prints ISR execution time (in µs)
 * 
 * NOTES:
 * - The usage of debuging functionalities requires a Serial.begin in the setup;
 * - The serial communication speed must be high, to avoid timing errors;
 *   A Baud Rate of 250000 seems to work;
 * - The usage of Serial communications inside an interrupt is not recommended, but 
 *   in this case, it's just a few prints for debuging purposes.
 * - IRSMALLD_DEBUG_ISRTIME mode uses AVR 328p Timer1 hardware specific code;
 *   Results are in microseconds, assuming a 16MHz clock (Clk Count is divided by 16 to get µs);
 */


#ifndef IRsmallD_Debug_h
  #define IRsmallD_Debug_h

  #ifdef IRSMALLD_DEBUG_STATE
    #define DBG_PRINT_STATE(...)  Serial.print(__VA_ARGS__);
  #else
    #define DBG_PRINT_STATE(...) 	//nothing
  #endif

  #ifdef IRSMALLD_DEBUG_INTERVAL
    #ifdef IRSMALLD_DEBUG_ISRTIME
      #define DBG_PRINTLN_DUR(...)  {Serial.print("i"); Serial.print(__VA_ARGS__);}
    #else
      #define DBG_PRINTLN_DUR(...)  {Serial.print("i"); Serial.println(__VA_ARGS__);}
    #endif
  #else
    #define DBG_PRINTLN_DUR(...) 	//nothing
  #endif

  #ifdef IRSMALLD_DEBUG_ISRTIME
    #if defined(IRSMALLD_DEBUG_STATE) || defined(IRSMALLD_DEBUG_INTERVAL)
      #warning Do not use IRSMALLD_DEBUG_ISRTIME with IRSMALLD_DEBUG_STATE or IRSMALLD_DEBUG_INTERVAL if you want accurate measurements of execution time
    #endif 
    #define DBG_RESTART_TIMER()  { TCCR1A = 0; TCCR1B = 1; TCNT1=0; } //set mode to count clock cycles and reset
    #define DBG_PRINTLN_TIMER()  { uint16_t elapsedTime=TCNT1; Serial.print("t"); Serial.println(elapsedTime>>4);}
  #else
    #define DBG_RESTART_TIMER()  //nothing
    #define DBG_PRINTLN_TIMER()  //nothing
  #endif

#endif


//instead of variadic macros, the following syntax could be used:
//PRINT(args...)  Serial.print(args) 

//Call Serial.flush() after every Serial.print() if you need to see the output before the code continues with buffered data

//do{}while(false) can be used to prevent some problems with more complex macros
// #define DBG_SERIALBEGIN(...)  do {Serial.begin(__VA_ARGS__); Serial.println("Serial Port Ready!");} while ( false )