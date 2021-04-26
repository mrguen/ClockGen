/* This version works with the board "Pandauino.com Clock Generator v1.0"
 * MCU = STM32F103CBT9 
 * Programmed using STM32CubePorgrammer (SWD)
 * Or Maple DFU Bootloader v 2.0  (generic_boot20_pb0.bin)
 * 
 * DEPENDENCIES:
 *
 * 1) "Arduino Core STM32" https://github.com/stm32duino/Arduino_Core_STM32
 *  // !!!!!!!!!!!!!!!! REVOIR LIEN / LIBRAIRIE Pandauino_Si5351A
 * 2) The library Si5351 by Etherkit https://github.com/etherkit/Si5351Arduino 
 * 3) Rotary encoder library http://www.mathertel.de/Arduino/RotaryEncoderLibrary.aspx  
 * 4) One Button library http://www.mathertel.de/Arduino/OneButtonLibrary.aspx
 * 5-6) Adafruit GFX and ST7735 libraries 
 *  https://github.com/adafruit/Adafruit-GFX-Library 
 *  https://github.com/adafruit/Adafruit-ST7735-Library
 * 
 * CONFIGURATION:
 * 
 * After installing the Arduino Core STM32, choose these parameters in the TOOLS menu
 * 
 * Board: Generic STM32F1 series
 * Part number: Generic F103CB
 * USART support: enabled generic serial
 * USB support: CDC generic Serial supersede USART
 * USB speed: low/full speed
 * Optimize: whatever you want
 * C runtime library: NewLib Nano
 * Upload method: choose the appropriate either STM32CubeProgrammer SWD or DFU. You need to have the proper STM32 drivers installed
 * 
 * 
 *  This code is based on Arduino code and is provided with the same warning that:
 *  THIS SOFTWARE IS PROVIDED TO YOU "AS IS" AND WE MAKE NO EXPRESS OR IMPLIED WARRANTIES WHATSOEVER WITH RESPECT TO ITS FUNCTIONALITY, OPERABILITY, OR USE,
 *  INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR INFRINGEMENT.
 *  WE EXPRESSLY DISCLAIM ANY LIABILITY WHATSOEVER FOR ANY DIRECT, INDIRECT, CONSEQUENTIAL, INCIDENTAL OR SPECIAL DAMAGES, INCLUDING, WITHOUT LIMITATION, LOST REVENUES,
 *  LOST PROFITS, LOSSES RESULTING FROM BUSINESS INTERRUPTION OR LOSS OF DATA, REGARDLESS OF THE FORM OF ACTION OR LEGAL THEORY UNDER WHICH THE LIABILITY MAY BE ASSERTED,
 *  EVEN IF ADVISED OF THE POSSIBILITY OR LIKELIHOOD OF SUCH DAMAGES.
 *
 *  Licence CC BY-NC-SA 4.0 see https://creativecommons.org/licenses/by-nc-sa/4.0/
 *  You are free to use, modifiy and distribute this software for non commercial use while keeping the original attribution to Pandauino.com
 */
 
#include <Pandauino_Clock_Generator.h>
// The library creates an instance called "ClockGenerator"

//**********************************************************************
// SETUP
void setup()
{
  ClockGenerator.begin();
} // setup()


//**********************************************************************
// LOOP
void loop()
{
  ClockGenerator.run();
} // loop ()
