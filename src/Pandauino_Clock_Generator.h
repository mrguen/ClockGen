/*
 *	Clock Generator library
 *  Version: 1.1.0
 *  By Pandauino.com / Thierry GUENNOU
 *  June 2021
 *
 *  This library is used on the board "Clock Generator v1.1" implementing an Si5351A clock generator chip and an STM32F103CB microcontroller
 *
 * 	DEPENDENCIES:
 *
 *  It needs
 *
 *  ** The Arduino Core STM32 see https://github.com/stm32duino/Arduino_Core_STM32
 *
 *  ** The library https://github.com/mrguen/Si5351Arduino
 *	that is a fork from Jason Milldrum's https://github.com/etherkit/Si5351Arduino
 * 	Spread spectrum is not implemented See. "Manually Generating an Si5351 Register Map.pdf"
 *
 *  ** Matthias Hertel Rotary encoder library http://www.mathertel.de/Arduino/RotaryEncoderLibrary.aspx
 *
 *  ** Matthias Hertel One Button library http://www.mathertel.de/Arduino/OneButtonLibrary.aspx
 *
 *  ** Adafruit GFX and ST7735 libraries
 *	https://github.com/adafruit/Adafruit-GFX-Library
 *  https://github.com/adafruit/Adafruit-ST7735-Library
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
 *  You are free to use, modifiy and distribute this software for non commercial use while keeping the original attribution to Pandauino.com and other authors.
 *
 */


/**************
SCREENS:

All elements are printed using displayElement(...) and displayCursor(...) functions
But the 3 clocks parameters on main page that are printed using displayAllclocks()

Main
	2 lines * 3 clocks
	Sweep
	Options

Clock
	Range
	FreqStep
	Frequency
	Drive
	Phase
	Exit

Options
	Store / retrieve >
	Sweep >
	Cal: + x.xx ppm
	Ph tied: 1-2-3
	Fact reset
	Exit

Store
	Store A
	Store B
	Retrieve A
	Retrieve B
	Exit

Sweep
	Nb: 100
	Period: 0.1 s
	C1 1.00 - 	>
	C2 inactive >
	C3 inactive >
	Exit

Sweep C1-C3
	Activate: yes
 	Range: KHz
 	S: 0.1/1 MHz
 	Start: 1.20000
 	Stop: 2.40000
	Exit

*/

#ifndef Pandauino_ClockGenerator_h
#define Pandauino_ClockGenerator_h

#include <si5351.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/Org_01.h>

#include <RotaryEncoder.h>
#include <OneButton.h>
#include <EEPROM.h>


// ***********************************************************************
// ENUM

enum swState {INACTIVE, HOLD, RUNNING};

enum PLLAClocks {PLLA_3_Clocks, PLLA_CK0, PLLA_CK0_CK1, PLLA_CK0_CK2, PLLA_CK1_CK2};

enum runMode {
	display_clocks_c0,
	display_clocks_c1,
	display_clocks_c2,
	display_clocks_sweep,
	display_clocks_options,

	display_range,
	display_freqStep,
	display_frequency,
	display_drive,
	display_phase,
	display_clock_exit,

	display_store,
	display_sweep,
	display_calibration,
	display_phaseTied,
	display_factory_reset,
	display_options_exit,

	display_storeA,
	display_storeB,
	display_retrieveA,
	display_retrieveB,
	display_store_exit,

	display_nbSweep,
	display_periodeSweep,
	display_C0Sweep,
	display_C1Sweep,
	display_C2Sweep,
	display_sweep_exit,

	display_activateSweep,
	display_rangeSweep,
	display_stepSweep,
	display_startSweep,
	display_stopSweep,
	display_SweepCx_exit
};


// ***********************************************************************
// CONSTANTS

const int TFT_CS = PA2; 													//	TFT signal pins. Used harware SPI i.e. SCK = PA5, MOSI = PA7 plus these pins
const int TFT_DC = PA3;
const int TFT_RST = PA4;
const int activateBlPin = PB9;										// TFT backlight pin

const int boostLap = 150;            							// used in testEncoder(). If the encoder changes value in less than boostLap then count 10 steps instead of one
																									// ! it reacts differently depending on the encoder model

const int flashPeriod = 500;          						// used in flashCursor to set the cursor flash period. In ms.
const int coefEncoder = 10;          							// used in testEncoder() to set the boost coef depending on the context
const byte menuButton = PA0;     									// encoder button on PA0;

const float frMinKHz = SI5351_CLKOUT_MIN_FREQ;   	// minimum frequency of the KHz range
const float frMaxKHz = 999999;     								// maximum frequency of the KHz range
const float frMinMHz = 1000000;    								// minimum frequency of the MHz range

const long frMinPhase = 3500000;									// it is useless to program the phase for too low frequencies because the freqStep is too small
const uint32_t  xtalFreq = 26000000; 							// Ref Xtal clock frequency of the SI5351  in Hz unit !!

const unsigned short colorActive =  0xFFFF;				// Color used for active clock
const unsigned short colorInverted =  0x07FF;			// Color used for active inverted clock
const unsigned short colorInactive =  0xF800;			// Color used for inactive clock

const unsigned short colorSweepInactive =  0x6546;		// Color used for the sweep button when not sweeping
const unsigned short colorSweepHold =  ST7735_GREEN;	// Color used for the sweep button when sweeping on hold
const unsigned short colorSweepRunning =  0xF888;			// Color used for the sweep button when sweeping

const int flashInitAddress = 0x10;								// Flash address to store flashInitFlag to check if eeprom is initialized and not corrupted
const int flashInitFlag = 888;										// Flag stored in EEPROM @flashInitAddress
const int flashCalibrationAddress = 0x20;					// Flash base adress to store calibration parameters in emulated EEPROM
const int flashSweepAddress = 0x30;								// Flash base adress to store sweep parameters in emulated EEPROM
const int flashClockBaseAddress = 0x40;						// Flash base adress to store clock objects in emulated EEPROM

const byte nbClock = 3;                     			// 3 clock objects

const int lineHeight = 14; 												// Display line heigth
const int interLine = 4;            							// Display inter-line

const int statusLedPin = PB0;											// Status led pin can be used to give visual information at will

const float maxCal = 50.0;												// Maximum calibration compensation value in ppm

const int minNbSweep = 1;													// Minimum number of sweep steps
const int maxNbSweep = 9999;											// Maximum number of sweep steps

const int minSweepPeriod = 0.1;										// Minimum sweep period
const int maxSweepPeriod = 9.9;										// Maximum sweep period


//**********************************************************************
// Class Clock

class Clock {
  public:
    byte id;            // clock ID
    boolean ac;         // turns on/off the clock
    uint32_t fr;        // clock frequency in Hz
    byte un;            // clock range: 0 for KHz, 1 for MHz
    boolean in;         // clock invert / not
    long st;            // clock adjustment frequency step: 1, 10, 100, 1000, 10000, 100000, 1000000
    si5351_drive dr;    // clock drive
    byte ph;            // clock phase in steps
		// sweep params
		boolean as;					// activate sweep
		byte rs;						// range sweep
		long ss;						// edit sweep frequency step
		long sa;						// sweep start frequency
		long so;						// sweep stop frequency


    Clock(byte, boolean, uint32_t, byte, boolean, long, si5351_drive, byte, boolean, byte, long, long, long);
    Clock(); // not initialized.

}; // End of Clock Class declaration



//**********************************************************************
// Class ClockGenerator
//
// All members are static. It is intended to be used as a singleton event though the singleton code is not implemented.
// Hence they are declared in the header file and defined in the code file

class ClockGeneratorClass {

  public:

		ClockGeneratorClass();
  	static void begin(boolean activateSerial = false, long baudRate = 9600 );
  	static void run();

   	static void setFrequency(int clockId, uint32_t frequency);
		static uint32_t getFrequency(int clockId);
    static void getPLLSettings(int clockId, si5351_pll& pll, uint64_t& pllFr );

   	static void setPhase(int clockId, byte phaseIndice);
   	static float getPhase(int clockId);

   	static void setDrive(int clockId, byte drive);
   	static byte getDrive(int clockId);

   	static void changeClockOutputState(int clockId);
   	static void configureSi5351();
   	static float getPhaseStep(int clockId);

   	static void setNbSweep(int nbSweep);
   	static void setSweepPeriod(float sweepPeriod);
   	static void getSweepParam(int& nbSweep, float& sweepPeriod);

   	static void setSweepParamCx(int clockId, long startFreq, long stopFreq, boolean sweepActive);
   	static void getSweepParamCx(int clockId, long& startFreq, long& stopFreq, boolean& sweepActive);

		static boolean initSweep();
		static void stopSweep();
		static void testSweep();

		static void setCalibration(float cal);
		static float getCalibration();

    static void setPhaseTiedClocks(byte phTied);
    static byte getPhaseTiedClocks();

   	static void displayAllClocks();


  private:

		// Debug functions
    static void printRegisters();
    static void printInfoClocks();
    static void printInfoClock(Clock &oneClock);
 		static void TestFlash();

    // Functions to store/read from emulated EEPROM
    static void flashInit();
   	static void saveCalibration();
   	static void readCalibration();
   	static void saveSweepSettings();
   	static void readSweepSettings();
    static void savePLLAClocks(byte slot);
    static void readPLLAClocks(byte slot);
    static void saveAllClocksInFlash(byte slot);
    static void saveOneClockInFlash(Clock &oneClock, byte slot);
    static void readAllClocksFromFlash( byte slot);
   	static void readOneClockFromFlash(Clock &oneClock, byte slot);

		// Miscellaneous functions
   	static uint64_t highestFrequency(float f1, float f2, float f3 = 0);
   	static byte highestFrequencyClock(byte c1, byte c2, byte c3 = 255);

   	static void nextDisplayValue(int incDec);
   	static swState computeSweepState();
		static void timerHandler();
		static void setTimer(float period);
		static void stopTimer();
		static void sweep();

   	// Si5351 programming and clock parameters computing
   	static boolean computePhaseTied(int clockId);
    static void setPhaseTiedClocks(PLLAClocks PLLAClks);

 		// Display functions
 		static void testdrawtext(char *text, uint16_t color);
   	static uint64_t roundFrequency(uint64_t frequency);
   	static void formatFrequency(uint64_t frequency, int unit, char formatedFreq[]);

   	static void elementPosition(runMode element, byte &line, byte &column, boolean &twoColumns);
   	static byte displayElement(runMode elementType, char elementText[], uint16_t color);
   	static void displayCursor(runMode elementType, boolean disp);
   	static void displayCursor(boolean disp);

   	static void flashCursor();
   	static void flashTrice(byte line);
   	static void displayIcon(byte line, boolean active, boolean inv);

		static void printSweep();
   	static void displayClock(int clockId);
   	static void displayFrequency(int clockId);

   	static void printRange(Clock &oneClock);
   	static void printStep(Clock &oneClock);
   	static void printFreq(Clock &oneClock);
		static byte driveDisplayValue(si5351_drive drive);
   	static void printDrive(Clock &oneClock);
   	static void printPhase(Clock &oneClock);
   	static void edit1Clock(Clock &oneClock, boolean refresh);
   	static void printPhaseOptions(boolean refresh);

		static void printCal(boolean refresh);

   	static void displayOptions();

   	static void displayStore();

 		static void printNbPeriodSweep(boolean refresh);
 		static void printPeriodSweep(boolean refresh);
 		static void printInfoSweepCx(int clockId);

 		static void displaySweep();

   	static void printSweepActivate(Clock &oneClock, boolean refresh);
   	static void printSweepRange(Clock &oneClock, boolean refresh);
   	static void printSweepStep(Clock &oneClock, boolean refresh);
   	static void printSweepStartFrequency(Clock &oneClock, boolean refresh);
   	static void printSweepStopFrequency(Clock &oneClock, boolean refresh);

   	static void editSweepSettings(Clock &oneClock, boolean refresh);

		static void printExit();

 		// Button and selector event handlers
   	static int  testEncoder();
   	static void buttonPress();
   	static void buttonClick();

		// Variables

		static boolean activateUSBSerial;
 		static Clock tbClock[];										// A table to store the 3 clock objects

		static PLLAClocks PLLAClks;    						// PLLModes enum value. The users choses which clocks are phase related / fed by PLLA

		static float phaseStep ;                	// Phase step in degrees
		static byte newPhaseIndice;								// Local variable user in run() to compute the new phase

		static unsigned int timeTestEncoder;			// Used in testEncoder() to determine if the encoder is rotated in boost mode
		static int pos;														// Encoder position
		static int newPos;           							// Pos and newPos are used in testEncoder() to determine encoder pulse count
		static int encoderIncDec;  								// Used to flag if the encoder value has increased or decreased at the beginning of the loop

		static runMode editMode;   								// defines the current state of the interface

		static byte selectedClock;     						// Stores the current selected clock number

		static uint64_t lowestFrequency;					// Stores the lowest possible frequency of a clock depending on the PLL frequency

		static boolean flash;											// Used to flash the cursor
		static unsigned int flashStamp;  					// Used to set the flashing speed in flashCursor

		static bool editing;											// Editing a parameter or not

		static float calibrationPPM;							// Calibration factor in ppm

		static int nbPeriodSweep;									// Number of sweep steps
		static float periodSweep;									// Period fo sweep in seconds
		static volatile boolean timeToSweep;			// Flag when the timer has triggered
		static volatile int countSweep;						// Counts the number of sweeps done
		static swState sweepState;							  // Actual sweep state: inactive, hold, running


}; // End of Clock Generator Class declaration


extern ClockGeneratorClass ClockGenerator;

#endif