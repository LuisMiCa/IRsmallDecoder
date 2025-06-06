/* IRsmallDProtocolCheck - Protocol selection check
 *
 * This file is part of the IRsmallDecoder library for Arduino
 * Copyright (c) 2020 Luis Carvalho
 *
 *
 * Notes:
 * - It will redefine each protocol selection macro as '1' and check if there is more than one defined.
 * - Although users could manually define it as '1', it's better to keep it simple for them.
 * - The #undef is included to prevent potential compiler warnings.
 */

#ifndef IRsmallD_ProtocolCheck_h
#define IRsmallD_ProtocolCheck_h

  #ifdef      IR_SMALLD_NEC
      #undef  IR_SMALLD_NEC
      #define IR_SMALLD_NEC 1
  #endif

  #ifdef      IR_SMALLD_NECx
      #undef  IR_SMALLD_NECx
      #define IR_SMALLD_NECx 1
  #endif

  #ifdef      IR_SMALLD_RC5
      #undef  IR_SMALLD_RC5
      #define IR_SMALLD_RC5 1
  #endif

  #ifdef      IR_SMALLD_SIRC12
      #undef  IR_SMALLD_SIRC12
      #define IR_SMALLD_SIRC12 1
  #endif

  #ifdef      IR_SMALLD_SIRC15
      #undef  IR_SMALLD_SIRC15
      #define IR_SMALLD_SIRC15 1
  #endif

  #ifdef      IR_SMALLD_SIRC20
      #undef  IR_SMALLD_SIRC20
      #define IR_SMALLD_SIRC20 1
  #endif

  #ifdef      IR_SMALLD_SIRC
      #undef  IR_SMALLD_SIRC
      #define IR_SMALLD_SIRC 1
  #endif

  #ifdef      IR_SMALLD_SAMSUNG
      #undef  IR_SMALLD_SAMSUNG
      #define IR_SMALLD_SAMSUNG 1
  #endif
  
  #ifdef      IR_SMALLD_SAMSUNG32
      #undef  IR_SMALLD_SAMSUNG32
      #define IR_SMALLD_SAMSUNG32 1
  #endif
  
  
  #define IR_SMALLD_CHECKSUM \
      ( IR_SMALLD_NEC       \
      + IR_SMALLD_NECx      \
      + IR_SMALLD_RC5       \
      + IR_SMALLD_SIRC12    \
      + IR_SMALLD_SIRC15    \
      + IR_SMALLD_SIRC20    \
      + IR_SMALLD_SIRC      \
      + IR_SMALLD_SAMSUNG   \
      + IR_SMALLD_SAMSUNG32 )

  #if IR_SMALLD_CHECKSUM == 0
      #error No protocol defined or misspelled. Check the IRsmallDecoder library documentation
  #elif IR_SMALLD_CHECKSUM > 1
      #error Only one protocol can be defined. Check the IRsmallDecoder library documentation
  #endif  

  // If no errors, then there's one and only one protocol macro defined (IR_SMALLD_CHECKSUM == 1)

#endif