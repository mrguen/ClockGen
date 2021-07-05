/* This version works with the Pandauino.com board "Clock Generator v1.1"
 * MCU = STM32F103CBT6 
 * Programmed using STM32CubePorgrammer (SWD)
 * Or USB / Maple DFU Bootloader v 2.0  (generic_boot20_pb0.bin)
 * 
 * See Pandauino_Clock_Generator.h for more details (on dependencies etc)
 * And the user's manual in https://github.com/mrguen/ClockGen/blob/main/doc
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
  ClockGenerator.begin(false,115200);
  
} // setup()


//**********************************************************************
// LOOP
void loop()
{
  ClockGenerator.run();
  
} // loop ()
