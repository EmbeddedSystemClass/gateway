# Gateway
This code began as a clone of the dynamometer repo, 4/2019, and was modified to be a two CAN gateway using a DiscoveryF4 board.  Incoming CAN msgs on CAN1 are sent to CAN2, and the PC (using a ASCII/HEX format); incoming CAN2 msgs are sent to CAN1 and the PC; and incoming PC msgs are sent only to CAN1. 

The main purpose of this repo was to reproduce the STM32F4 CAN module problem using the STM32CubMX/FreeRTOS software, where a CAN msg "abort" would occasionally result in a loss of a CAN interrupt under rare timing and bus traffic situations, discovered by the bare-metal gateway code. (See GliderWinchCommons/embed/svn_discoveryf4/sensor/ftdi_2can/trunk and related routines in ../svn_common/trunk, most importantly can_driver.[ch].

Initially there is no filtering of CAN msg IDs in any of the gateway paths.  

Initial CAN speed is set to 500K baud.  

Serial ports: 
 usart6 @ 115200 baud for debugging/monitoring/etc.
 usart2 @ 2000000 baud for ascii/hex CAN msgs to/from PC.
 
 LEDs: trap serious errors and display trap number with Morse code
 
 ADC: Some uncompleted code from dynamometer is included (no extra charge)  Vdd measurement with internal Vref and internal temperature are functional.
 
 iface.[ch] includes CAN msg-by-msg loopback and /NART settings.
