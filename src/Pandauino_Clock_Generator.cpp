#include <Pandauino_Clock_Generator.h>

//============================== DEFINITIONS OF STATIC MEMBERS ================================================================

Clock ClockGeneratorClass::tbClock[3] = {
	Clock(0, true, 100000200, 1, false, 1000000, SI5351_DRIVE_8MA, 0, false, 0, 100, 10000, 100000),
	Clock(1, true, 5100, 0, true, 1000, SI5351_DRIVE_8MA, 0, false, 0, 100, 10000, 100000),
	Clock(2, false, 4001, 0, false, 100, SI5351_DRIVE_8MA, 0, false, 0, 100, 10000, 100000)
};

PLLAClocks ClockGeneratorClass::PLLAClks;    	// PLLModes enum value. The users choses which clocks are phase related / fed by PLLA.
// unsigned long int ClockGeneratorClass::PLLdivider;
// uint64_t ClockGeneratorClass::pllAFreq;
// uint64_t ClockGeneratorClass::pllBFreq;
float ClockGeneratorClass::phaseStep;
byte ClockGeneratorClass::newPhaseIndice;

unsigned int ClockGeneratorClass::timeTestEncoder = 0;
int ClockGeneratorClass::pos = 0;
int ClockGeneratorClass::newPos = 0;
int ClockGeneratorClass::encoderIncDec = 0;

runMode ClockGeneratorClass::editMode = display_clocks_c0;

byte ClockGeneratorClass::selectedClock = 0;

boolean ClockGeneratorClass::flash;
unsigned int ClockGeneratorClass::flashStamp = millis();

//byte ClockGeneratorClass::outputDrive[4];

bool ClockGeneratorClass::editing = false;

float ClockGeneratorClass::calibrationPPM = 0;

int ClockGeneratorClass::nbPeriodSweep = 100;
float ClockGeneratorClass::periodSweep = 1;
volatile boolean ClockGeneratorClass::timeToSweep = false;
int volatile ClockGeneratorClass::countSweep = 0;

swState ClockGeneratorClass::sweepState = INACTIVE;

//=================================== INSTANCES =============================================================================

// Setup a RotaryEncoder
RotaryEncoder encoder(PB13, PB12);

// Setup TFT 1.44 ST7735 controller
Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS,  TFT_DC, TFT_RST);

// Menu button
OneButton button(menuButton, true);

// Instantiates Si5351 clock generator
Si5351 mySi5351;

si5351_clock clk = SI5351_CLK0;   				// Enum value that identifies the selected clock object

HardwareTimer *timer1 = new HardwareTimer(TIM1);



//=================================== Generic function =============================================================================

void printUint64_t(uint64_t num) {

  char rev[128];
  char *p = rev+1;

  while (num > 0) {
    *p++ = '0' + ( num % 10);
    num/= 10;
  }
  p--;
  //Print the number which is now in reverse
  while (p > rev) {
    Serial.print(*p--);
  }
}


//=================================== CLOCK CLASS =============================================================================

// ***********************************************************************************************************
// Clock constructors
Clock:: Clock(byte _id, boolean _ac, uint32_t _fr, byte _un, boolean _in, long _st, si5351_drive _dr, byte _ph, boolean _as, byte _rs, long _ss, long _sa, long _so) {

  id = _id;
  ac = _ac;
  fr = _fr;
  un = _un;
  in = _in;
  st = _st;
  dr = _dr;
  ph = _ph;
  as = _as;
  rs = _rs;
  ss = _ss;
  sa = _sa;
  so = _so;


}

Clock:: Clock() {}

//=============================================================================================================================
//=================================== ClockGenerator CLASS ====================================================================
//=============================================================================================================================

//=================================== Public functions ========================================================================

// ***********************************************************************************************************
// ClockGenerator constructor
ClockGeneratorClass::ClockGeneratorClass() {
		// Would be cleaner to implement a singleton
}

// ***********************************************************************************************************
// Begin
void ClockGeneratorClass::begin() {

  pinMode(statusLedPin, OUTPUT);
  digitalWrite(statusLedPin, HIGH);
  delay(500);
  digitalWrite(statusLedPin, LOW);

  SPI.setClockDivider(SPI_CLOCK_DIV2);

  // These two lines are mandatory to start Serial. The serial monitor should be stopped before a manual reset and started again. Otherwise the COM port might be lost
  //Serial.begin(115200);
  //while (!Serial);
  //delay(10);

  Serial.println("Begin setup");

  // Tests if flash is populated and populates it if empty
  flashInit();
	readCalibration();
	readSweepSettings();
  // Loads all current (slot range = 0) clock parameters
  readAllClocksFromFlash(0);

	sweepState = computeSweepState();

	// VERIFIER  SI NECESSAIRE
	// readPLLAClocks();


/*   // TEST
  TestFlash();
  Serial.print("flashStamp: ");
  Serial.println(flashStamp);
*/

  // BUTTON
  pinMode(menuButton, INPUT_PULLUP);

  button.attachClick(buttonClick); 					// Pas sur que ca fonctionne car appel de méthode privée mais compile
  button.attachLongPressStart(buttonPress); // Pas sur que ca fonctionne car appel de méthode privée mais compile

  // Max freq /// A REVOIR ENLEVER ?
  //if (PLLAClks != PLLA_CK0) SI5351_CLKOUT_MAX_FREQ =  SI5351_CLKOUT_MAX_FREQ;
  //else SI5351_CLKOUT_MAX_FREQ =  SI5351_CLKOUT_MAX_FREQ;

  // Si5351
  mySi5351.init(SI5351_CRYSTAL_LOAD_8PF, xtalFreq, 0);

  // Will compute the best pllFreq and pll divider possible given the 3 clocks parameters
  // and configure Si5351
  configureSi5351();

  // TFT
  tft.initR(INITR_144GREENTAB);   //  A VERIFIER
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);

  // displays first screen with all 3 clocks parameters
  displayAllClocks();
  selectedClock = 0;
  editMode = display_clocks_c0;
  displayCursor(true);

  // DEBUG
  // printRegisters();
  // digitalWrite(statusLedPin, HIGH);
  // delay(500);
  // digitalWrite(statusLedPin, LOW);

  Serial.println("End setup");


	// DEBUG
	// displayStore();
	// displaySweep();
	// displaySweepSettings(1);
	// displayOptions();

}

// ***********************************************************************************************************
// getPhaseStep
float ClockGeneratorClass::getPhaseStep(int clockId) {

	return 9000.0 * ((double)tbClock[clockId].fr / (double)mySi5351.plla_freq);

}


// ***********************************************************************************************************
// setNbSweep
void ClockGeneratorClass::setNbSweep(int nbSweep) {

	if (nbSweep > 9999) nbSweep = 9999;
	if (nbSweep < 1) nbSweep = 1;

	nbPeriodSweep = nbSweep;

}

// ***********************************************************************************************************
// setSweepPeriod
void ClockGeneratorClass::setSweepPeriod(float sweepPeriod) {

	if (sweepPeriod > 9.9) sweepPeriod = 9.9;
	if (sweepPeriod < 0.1) sweepPeriod = 0.1;

	periodSweep = sweepPeriod;

}

// ***********************************************************************************************************
// setSweepPeriod
void ClockGeneratorClass::getSweepParam(int& nbSweep, float& sweepPeriod) {
	nbSweep = nbPeriodSweep;
	sweepPeriod = periodSweep;
}

// ***********************************************************************************************************
// setSweepParamCx
void ClockGeneratorClass::setSweepParamCx(int clockId, long startFreq, long stopFreq, boolean sweepActive) {

 	tbClock[clockId].as = sweepActive;
 	tbClock[clockId].sa = startFreq;
 	tbClock[clockId].so = stopFreq;

}

// ***********************************************************************************************************
// getSweepParamCx
void ClockGeneratorClass::getSweepParamCx(int clockId, long& startFreq, long& stopFreq, boolean& sweepActive) {

 	startFreq = tbClock[clockId].sa;
 	stopFreq = tbClock[clockId].so;
 	sweepActive = tbClock[clockId].as ;

}

// ***********************************************************************************************************
// setCalibration
void ClockGeneratorClass::setCalibration(float cal) {

	if (cal > 9.99) cal = 9.99;
	if (cal < -9.99) cal = -9.99;

	calibrationPPM = cal;

}

// ***********************************************************************************************************
// getCalibration
float ClockGeneratorClass::getCalibration() {

  return calibrationPPM;

}

// ***********************************************************************************************************
// setPhaseTiedClocks(int)
void ClockGeneratorClass::setPhaseTiedClocks(byte phtied) {
 setPhaseTiedClocks(static_cast<PLLAClocks>(phtied));
}

// ***********************************************************************************************************
// setPhaseTiedClocks(PLLAClocks)
void ClockGeneratorClass::setPhaseTiedClocks(PLLAClocks Pllaclk) {

	PLLAClks = Pllaclk;
  savePLLAClocks(0);
	//mySi5351.init(SI5351_CRYSTAL_LOAD_8PF, xtalFreq, 0); // necesary since clocks might have changed PLL
	configureSi5351();

}

// ***********************************************************************************************************
// getPhaseTiedClocks
byte ClockGeneratorClass::getPhaseTiedClocks() {

	return((byte)PLLAClks);

}

// ***********************************************************************************************************
// Run

void ClockGeneratorClass::run() {

	if (editing) flashCursor();  // shows that we are currently editing

  // Checks machine state of menu button (before handling sweep!)
  button.tick();

	// If the timer handler was triggered, sweep
	testSweep();

  // computes the encoder value
  encoderIncDec = testEncoder();
  if (encoderIncDec == 0) return;

	// Debug
	//Serial.print("editMode: ");
	//Serial.println(editMode);

	// ==========================================================
  // Takes care of all the navigation up/down in menus
	if (!editing) {

		nextDisplayValue(encoderIncDec);
		displayCursor(true);

	// ==========================================================
	// Takes care of edit actions
	} else {

	  switch (editMode) {

    case display_range:

      if (encoderIncDec != 0) {
        // DEBUG
        //Serial.println( tbClock[selectedClock].un );

        if (tbClock[selectedClock].un == 1) {
          tbClock[selectedClock].un = 0;
          tbClock[selectedClock].st = 1000;
          tbClock[selectedClock].fr = frMinKHz;
        }
        else if (tbClock[selectedClock].un == 0) {
          tbClock[selectedClock].un = 1;
          tbClock[selectedClock].st = 1000000;
          tbClock[selectedClock].fr = SI5351_CLKOUT_MIN_FREQ;
        }

        clk = static_cast<si5351_clock>(selectedClock);
        mySi5351.set_freq(tbClock[selectedClock].fr*SI5351_FREQ_MULT, clk);
        tft.fillRect(10, 2*(lineHeight + interLine)+1, 121, lineHeight + interLine,  ST7735_BLACK);
        printStep(tbClock[selectedClock]);
        tft.fillRect(10, 3*(lineHeight + interLine)+1, 121, lineHeight + interLine,  ST7735_BLACK);
        printFreq(tbClock[selectedClock]);

        edit1Clock(tbClock[selectedClock], true );

      }
      flashCursor();
      break;

    case display_freqStep:

      if (encoderIncDec > 0) {

        if (tbClock[selectedClock].un == 0) {

          if (tbClock[selectedClock].st == 1) tbClock[selectedClock].st = 10;
          else if (tbClock[selectedClock].st == 10) tbClock[selectedClock].st = 100;
          else if (tbClock[selectedClock].st == 100) tbClock[selectedClock].st = 1000;
          else if (tbClock[selectedClock].st == 1000) tbClock[selectedClock].st = 10000;
          else if (tbClock[selectedClock].st == 10000) tbClock[selectedClock].st = 1;

        } else if (tbClock[selectedClock].un == 1) {

          if (tbClock[selectedClock].st == 1) tbClock[selectedClock].st = 10;
          else if (tbClock[selectedClock].st == 10) tbClock[selectedClock].st = 100;
          else if (tbClock[selectedClock].st == 100) tbClock[selectedClock].st = 1000;
          else if (tbClock[selectedClock].st == 1000) tbClock[selectedClock].st = 10000;
          else if (tbClock[selectedClock].st == 10000) tbClock[selectedClock].st = 100000;
          else if (tbClock[selectedClock].st == 100000) tbClock[selectedClock].st = 1000000;
          else if (tbClock[selectedClock].st == 1000000) tbClock[selectedClock].st = 1;

        }

        edit1Clock(tbClock[selectedClock], true );

      } else if (encoderIncDec < 0) {

        if (tbClock[selectedClock].un == 0) {

          if (tbClock[selectedClock].st == 1) tbClock[selectedClock].st = 10000;
          else if (tbClock[selectedClock].st == 10) tbClock[selectedClock].st = 1;
          else if (tbClock[selectedClock].st == 100) tbClock[selectedClock].st = 10;
          else if (tbClock[selectedClock].st == 1000) tbClock[selectedClock].st = 100;
          else if (tbClock[selectedClock].st == 10000) tbClock[selectedClock].st = 1000;

	    	} else if (tbClock[selectedClock].un == 1) {

       		if (tbClock[selectedClock].st == 1) tbClock[selectedClock].st = 1000000;
       		else if (tbClock[selectedClock].st == 10) tbClock[selectedClock].st = 1;
          else if (tbClock[selectedClock].st == 100) tbClock[selectedClock].st = 10;
          else if (tbClock[selectedClock].st == 1000) tbClock[selectedClock].st = 100;
          else if (tbClock[selectedClock].st == 10000) tbClock[selectedClock].st = 1000;
          else if (tbClock[selectedClock].st == 100000) tbClock[selectedClock].st = 10000;
          else if (tbClock[selectedClock].st == 1000000) tbClock[selectedClock].st = 10000;
      	}

      	edit1Clock(tbClock[selectedClock], true );

      }
      flashCursor();
      break;

    case display_frequency:

      if (encoderIncDec != 0) {

        long newFreq = roundFrequency(tbClock[selectedClock].fr + encoderIncDec * tbClock[selectedClock].st);

        if (tbClock[selectedClock].un == 0) {
          if (newFreq <frMinKHz) newFreq = frMinKHz;
          if (newFreq >frMaxKHz) newFreq = frMaxKHz;
        }
        else if (tbClock[selectedClock].un == 1) {
          if (newFreq <SI5351_CLKOUT_MIN_FREQ) newFreq = SI5351_CLKOUT_MIN_FREQ;
          if (newFreq >SI5351_CLKOUT_MAX_FREQ) newFreq = SI5351_CLKOUT_MAX_FREQ; // 220 MHz and 100 MHs when 2 or 3 clocks are phase tied
        }

        // Only one clock can be set above 100 MHz in all cases.
        for (int i=0; i<3; i++) {
          if ((i != selectedClock) && (tbClock[i].fr > SI5351_MULTISYNTH_SHARE_MAX) && (newFreq > SI5351_MULTISYNTH_SHARE_MAX)) newFreq = SI5351_MULTISYNTH_SHARE_MAX;
        }

 //       tbClock[selectedClock].fr = newFreq;
				setFrequency(selectedClock, newFreq);
//				setPhase(selectedClock, tbClock[selectedClock].ph +1);
        edit1Clock(tbClock[selectedClock], true );
      }
      flashCursor();
      break;

    case display_drive:

      if (encoderIncDec != 0) {

        int newDrive = (int)tbClock[selectedClock].dr + encoderIncDec;
        if (newDrive > 3) newDrive = 3;
        if (newDrive < 0) newDrive = 0;
//        si5351_drive drv = static_cast<si5351_drive>(newDrive);
				setDrive(selectedClock, newDrive);
        edit1Clock(tbClock[selectedClock], true );

      } // edit drive
      flashCursor();
      break;

    case display_phase:

      if (encoderIncDec != 0) {

				if (encoderIncDec>0) newPhaseIndice = tbClock[selectedClock].ph + 1;
				else newPhaseIndice = tbClock[selectedClock].ph -1;

				setPhase(selectedClock, newPhaseIndice);
        edit1Clock(tbClock[selectedClock], true );
      }

      flashCursor();
      break;

		case display_calibration:

			calibrationPPM += ((float)encoderIncDec)/100.0;
			setCalibration(calibrationPPM);
			//setFrequency(selectedClock, tbClock[selectedClock].fr);
			printCal(true);
			break;

		case display_phaseTied: // display phase tied options

      if (encoderIncDec != 0) {
        if (PLLAClks == PLLA_CK0) { PLLAClks = PLLA_3_Clocks; }
        else if (PLLAClks == PLLA_3_Clocks) { PLLAClks = PLLA_CK0_CK1; }
        else if (PLLAClks == PLLA_CK0_CK1) { PLLAClks = PLLA_CK0_CK2; }
        else if (PLLAClks == PLLA_CK0_CK2) { PLLAClks = PLLA_CK1_CK2; }
        else { PLLAClks = PLLA_CK0; }

				// A REVOIR, ARBITRAIRE
        // If clocks are phase tied, the max frequency is 100 MHz
        //if (PLLAClks != PLLA_CK0) SI5351_CLKOUT_MAX_FREQ =  SI5351_MULTISYNTH_SHARE_MAX;
        //else SI5351_CLKOUT_MAX_FREQ =  SI5351_CLKOUT_MAX_FREQ;

        printPhaseOptions(true);
      }

    flashCursor();
    break;

		case display_nbSweep:
		nbPeriodSweep += encoderIncDec;
		setNbSweep(nbPeriodSweep);
		printNbPeriodSweep(true);
		break;

		case display_periodeSweep:
		periodSweep += (float)encoderIncDec / 10.0;
    setSweepPeriod(periodSweep);
		printPeriodSweep(true);
		break;

		case display_activateSweep:
		if (encoderIncDec != 0) {
			tbClock[selectedClock].as = !tbClock[selectedClock].as;
			printSweepActivate(tbClock[selectedClock], true);
		}
		break;

		case display_rangeSweep:
    if (encoderIncDec != 0) {

    	if (tbClock[selectedClock].rs == 1) {
    		tbClock[selectedClock].rs = 0;
    		tbClock[selectedClock].ss = 1000;
        tbClock[selectedClock].sa = frMinKHz;
        tbClock[selectedClock].so = frMinKHz;
      }
      else if (tbClock[selectedClock].rs == 0) {
        tbClock[selectedClock].rs = 1;
        tbClock[selectedClock].ss = 1000000;
        tbClock[selectedClock].sa = SI5351_CLKOUT_MIN_FREQ;
        tbClock[selectedClock].so = SI5351_CLKOUT_MIN_FREQ;
      }

			printSweepRange(tbClock[selectedClock], true);
			printSweepStartFrequency(tbClock[selectedClock], true);
			printSweepStopFrequency(tbClock[selectedClock], true);

    }

		break;

		case display_stepSweep:
    if (encoderIncDec > 0) {

    	if (tbClock[selectedClock].rs == 0) {

      	if (tbClock[selectedClock].ss == 1) tbClock[selectedClock].ss = 10;
        else if (tbClock[selectedClock].ss == 10) tbClock[selectedClock].ss = 100;
        else if (tbClock[selectedClock].ss == 100) tbClock[selectedClock].ss = 1000;
        else if (tbClock[selectedClock].ss == 1000) tbClock[selectedClock].ss = 10000;
        else if (tbClock[selectedClock].ss == 10000) tbClock[selectedClock].ss = 1;

      } else if (tbClock[selectedClock].rs == 1) {

        if (tbClock[selectedClock].ss == 1) tbClock[selectedClock].ss = 10;
        else if (tbClock[selectedClock].ss == 10) tbClock[selectedClock].ss = 100;
        else if (tbClock[selectedClock].ss == 100) tbClock[selectedClock].ss = 1000;
        else if (tbClock[selectedClock].ss == 1000) tbClock[selectedClock].ss = 10000;
        else if (tbClock[selectedClock].ss == 10000) tbClock[selectedClock].ss = 100000;
        else if (tbClock[selectedClock].ss == 100000) tbClock[selectedClock].ss = 1000000;
        else if (tbClock[selectedClock].ss == 1000000) tbClock[selectedClock].ss = 1;

      }

      printSweepStep(tbClock[selectedClock], true );

    } else if (encoderIncDec < 0) {

    	if (tbClock[selectedClock].rs == 0) {

      	if (tbClock[selectedClock].ss == 1) tbClock[selectedClock].ss = 10000;
        else if (tbClock[selectedClock].ss == 10) tbClock[selectedClock].ss = 1;
        else if (tbClock[selectedClock].ss == 100) tbClock[selectedClock].ss = 10;
        else if (tbClock[selectedClock].ss == 1000) tbClock[selectedClock].ss = 100;
        else if (tbClock[selectedClock].ss == 10000) tbClock[selectedClock].ss = 1000;

	   	} else if (tbClock[selectedClock].rs == 1) {

       	if (tbClock[selectedClock].ss == 1) tbClock[selectedClock].ss = 1000000;
       	else if (tbClock[selectedClock].ss == 10) tbClock[selectedClock].ss = 1;
        else if (tbClock[selectedClock].ss == 100) tbClock[selectedClock].ss = 10;
        else if (tbClock[selectedClock].ss == 1000) tbClock[selectedClock].ss = 100;
        else if (tbClock[selectedClock].ss == 10000) tbClock[selectedClock].ss = 1000;
        else if (tbClock[selectedClock].ss == 100000) tbClock[selectedClock].ss = 10000;
        else if (tbClock[selectedClock].ss == 1000000) tbClock[selectedClock].ss = 10000;
      }

      printSweepStep(tbClock[selectedClock], true );

    }
		break;

		case display_startSweep:
    if (encoderIncDec != 0) {

    	long newFreq = roundFrequency(tbClock[selectedClock].sa + encoderIncDec * tbClock[selectedClock].ss);

      if (tbClock[selectedClock].rs == 0) {
      	if (newFreq <frMinKHz) newFreq = frMinKHz;
      	if (newFreq >frMaxKHz) newFreq = frMaxKHz;
      }
      else if (tbClock[selectedClock].rs == 1) {
      	if (newFreq <SI5351_CLKOUT_MIN_FREQ) newFreq = SI5351_CLKOUT_MIN_FREQ;
      	if (newFreq >SI5351_CLKOUT_MAX_FREQ) newFreq = SI5351_CLKOUT_MAX_FREQ; // 220 MHz and 100 MHs when 2 or 3 clocks are phase tied
      }

      // Only one clock can be set above 100 MHz in all cases.
      for (int i=0; i<3; i++) {
      	if ((i != selectedClock) && (tbClock[i].sa > SI5351_MULTISYNTH_SHARE_MAX) && (newFreq > SI5351_MULTISYNTH_SHARE_MAX)) newFreq = SI5351_MULTISYNTH_SHARE_MAX;
      }

			tbClock[selectedClock].sa = newFreq;
      printSweepStartFrequency(tbClock[selectedClock], true );

 		}
		break;

		case display_stopSweep:
    if (encoderIncDec != 0) {

    	long newFreq = roundFrequency(tbClock[selectedClock].so + encoderIncDec * tbClock[selectedClock].ss);

      if (tbClock[selectedClock].rs == 0) {
      	if (newFreq <frMinKHz) newFreq = frMinKHz;
      	if (newFreq >frMaxKHz) newFreq = frMaxKHz;
      }
      else if (tbClock[selectedClock].rs == 1) {
      	if (newFreq <SI5351_CLKOUT_MIN_FREQ) newFreq = SI5351_CLKOUT_MIN_FREQ;
      	if (newFreq >SI5351_CLKOUT_MAX_FREQ) newFreq = SI5351_CLKOUT_MAX_FREQ; // 220 MHz and 100 MHs when 2 or 3 clocks are phase tied
      }

      // Only one clock can be set above 100 MHz in all cases.
      for (int i=0; i<3; i++) {
      	if ((i != selectedClock) && (tbClock[i].so > SI5351_MULTISYNTH_SHARE_MAX) && (newFreq > SI5351_MULTISYNTH_SHARE_MAX)) newFreq = SI5351_MULTISYNTH_SHARE_MAX;
      }

			tbClock[selectedClock].so = newFreq;
      printSweepStopFrequency(tbClock[selectedClock], true );

 		}
		break;


  	} // switch
	} //else
}


//=================================== Debug functions =======================================================================

// ***********************************************************************************************************
// ClockGenerator printRegisters

void ClockGeneratorClass::printRegisters() {

  byte regValue;
  for (int i=0; i<256; i++) {

    regValue = mySi5351.si5351_read(i);
    Serial.print(i);
    Serial.print(";");
    Serial.print((bool)(regValue & B10000000));
    Serial.print(";");
    Serial.print((bool)(regValue & B01000000));
    Serial.print(";");
    Serial.print((bool)(regValue & B00100000));
    Serial.print(";");
    Serial.print((bool)(regValue & B00010000));
    Serial.print(";");
    Serial.print((bool)(regValue & B00001000));
    Serial.print(";");
    Serial.print((bool)(regValue & B00000100));
    Serial.print(";");
    Serial.print((bool)(regValue & B00000010));
    Serial.print(";");
    Serial.println((bool)(regValue & B00000001));

  }
}

// ***********************************************************************************************************
// ClockGenerator printInfoClocks
void ClockGeneratorClass::printInfoClocks() {
  for (int i = 0; i < nbClock; i++) printInfoClock(tbClock[i]);
}

// ***********************************************************************************************************
// ClockGenerator printInfoClock
void ClockGeneratorClass::printInfoClock(Clock &oneClock) {

  Serial.print("id: ");
  Serial.println(oneClock.id);

  Serial.print("ac: ");
  Serial.println(oneClock.ac);

  Serial.print("fr: ");
  Serial.println(oneClock.fr);

  Serial.print("un: ");
  Serial.println(oneClock.un);
 // Serial.print("size: ");
 // Serial.println(sizeof(oneClock.un));

  Serial.print("in: ");
  Serial.println(oneClock.in);

  Serial.print("st: ");
  Serial.println(oneClock.st);

  Serial.print("dr: ");
  Serial.println(oneClock.dr);

  Serial.print("ph: ");
  Serial.println(oneClock.ph);
  Serial.println(" ");

}


// ***********************************************************************************************************
// ClockGenerator TestFlash
 void ClockGeneratorClass::TestFlash() {

  Serial.println("Saves clocks");
  printInfoClocks();

  saveAllClocksInFlash(0);

  Serial.println("Charge clocks");
  readAllClocksFromFlash(0);

  printInfoClocks();


 }

//=================================== Functions to store/read from emulated EEPROM ==============================================

// ***********************************************************************************************************
// ClockGenerator flashInit
// Populates flash with clock parameters if empty
void ClockGeneratorClass::flashInit() {
  unsigned short readTampon;

  EEPROM.get(flashInitAddress,readTampon);
/*
  unsigned short status = EEPROM.get(flashInitAddress,readTampon);
  if (status > 0) {
    Serial.print("Flash read error on flashInit: ");
    Serial.println(status, HEX);
  }
*/
  // Debug
  // Serial.println(readTampon);

  if (readTampon !=  flashInitFlag) {
		saveCalibration();
		saveSweepSettings();
    saveAllClocksInFlash(0);
    EEPROM.put(flashInitAddress, flashInitFlag);
 /*   if (status > 0) {
      Serial.print("Flash write error on flashInit: ");
      Serial.println(status, HEX);
    }
 */
  }
}

// ***********************************************************************************************************
// ClockGenerator saveCalibration
void ClockGeneratorClass::saveCalibration() {
	EEPROM.put(flashCalibrationAddress, calibrationPPM);
}

// ***********************************************************************************************************
// ClockGenerator readCalibration
void ClockGeneratorClass::readCalibration() {
	EEPROM.get(flashCalibrationAddress, calibrationPPM);
}

// ***********************************************************************************************************
// ClockGenerator saveSweepSettings
void ClockGeneratorClass::saveSweepSettings() {

 int eepromAddress = flashSweepAddress;

 EEPROM.put(eepromAddress, nbPeriodSweep);

 eepromAddress += sizeof(nbPeriodSweep);
 EEPROM.put(eepromAddress, periodSweep);

}

// ***********************************************************************************************************
// ClockGenerator readSweepSettings
void ClockGeneratorClass::readSweepSettings() {

 int eepromAddress = flashSweepAddress;

 EEPROM.get(eepromAddress, nbPeriodSweep);

 eepromAddress += sizeof(nbPeriodSweep);
 EEPROM.get(eepromAddress, periodSweep);

}

// ***********************************************************************************************************
// ClockGenerator savePLLAClocks
// Saves the PLLAClocks setting for a set of 3 clocks at the given slot
void ClockGeneratorClass::savePLLAClocks(byte slot) {
  int clockSize = sizeof(Clock);
  int PLLAClocksSize = sizeof(byte);

  int eepromAddress = flashClockBaseAddress + slot * 3 * (clockSize + PLLAClocksSize);

  EEPROM.put(eepromAddress, (byte)PLLAClks);
}

// ***********************************************************************************************************
// ClockGenerator readPLLAClocks
// Reads the PLLAClocks setting for a set of 3 clocks at the given slot
void ClockGeneratorClass::readPLLAClocks(byte slot) {
  int clockSize = sizeof(Clock);
  int PLLAClocksSize = sizeof(byte);

  int eepromAddress = flashClockBaseAddress + slot * 3 * (clockSize + PLLAClocksSize);

  EEPROM.get(eepromAddress, PLLAClks);
}

// ***********************************************************************************************************
// ClockGenerator saveAllClocksInFlash
// Saves all clocks in flash at a slotRange position (starts at 0)
 void ClockGeneratorClass::saveAllClocksInFlash( byte slot) {

  savePLLAClocks(slot);
  for (int i = 0; i < nbClock; i++) saveOneClockInFlash(tbClock[i], slot*nbClock + i);

  // Si5351 parameters are computed depending on the highest clock frequency of all clocks
  // By configureSi5351()
  // Hence phaseStep is computed and phase parameters should be exact, given: phase = tbClock[i].ph * phaseStep
  // If the computation is not repeatable due to the imprecision, the phase might be wrong after saving / loading clock parameters
  // ... To test

 }

// ***********************************************************************************************************
// ClockGenerator saveOneClockInFlash
// Saves the parameters of a clock in flash used as an EEPROM at a slot position: 0, 1 etc
void ClockGeneratorClass::saveOneClockInFlash(Clock &oneClock, byte slot) {

  int clockSize = sizeof(Clock);
  int PLLAClocksSize = sizeof(byte);
  int eepromAddress = flashClockBaseAddress + slot * 3 * (clockSize + PLLAClocksSize) + PLLAClocksSize + oneClock.id * clockSize;
  EEPROM.put(eepromAddress, oneClock);

}

// ***********************************************************************************************************
// ClockGenerator readAllClocksFromFlash
// Loads all clocks from flash at a given slotRange position (starts at 0)
 void ClockGeneratorClass::readAllClocksFromFlash( byte slot) {

   readPLLAClocks(slot);
   for (int i = 0; i < nbClock; i++) readOneClockFromFlash(tbClock[i], slot*nbClock + i);
 }

// ***********************************************************************************************************
// ClockGenerator readOneClockFromFlash
// Loads the parameters of a clock saved in flash used as an EEPROM at a slot position: 0, 1 etc
void ClockGeneratorClass::readOneClockFromFlash(Clock &oneClock, byte slot) {

  int clockSize = sizeof(Clock);
  int PLLAClocksSize = sizeof(byte);
  int eepromAddress = flashClockBaseAddress + slot * 3 * (clockSize + PLLAClocksSize) + PLLAClocksSize + oneClock.id * clockSize;
  EEPROM.get(eepromAddress, oneClock);

}

//=================================== Miscellaneous functions ===============================================================

// ***********************************************************************************************************
// ClockGenerator highestFrequency
/*
float ClockGeneratorClass::highestFrequency(float f1, float f2, float f3) {
  uint64_t highest;

  if (f3> f2) highest = f3;
  else highest = f2;

  if (f1 > highest) highest = f1;

  return highest;
}
*/

// ***********************************************************************************************************
// ClockGenerator highestFrequencyClock
byte ClockGeneratorClass::highestFrequencyClock(byte c1, byte c2, byte c3) {

  byte highestClockId;
  uint64_t highest;

	highest = tbClock[c2].fr; highestClockId = c2;

	if (c3 != 255) {
  	if (tbClock[c3].fr > tbClock[c2].fr) {highest = tbClock[c3].fr; highestClockId = c3;}
	}

  if (tbClock[c1].fr  > highest) {highest = tbClock[c1].fr; highestClockId = c1;}

  return highestClockId;

}

// ***********************************************************************************************************
// ClockGenerator testEncoder
int ClockGeneratorClass::testEncoder() {

  int incDec = 0;

  encoder.tick();
  newPos = encoder.getPosition();
  // DEBUG
  //Serial.println(newPos);

  if (pos != newPos) {

    // Boost only when editing frequency or phase
    if (((editMode == display_frequency) or (editMode == display_phase) or (editMode == display_calibration) or (editMode == display_nbSweep))  and ((millis() - timeTestEncoder) < boostLap)) {
      incDec = coefEncoder * (newPos - pos);
    }
    else {
      incDec = newPos - pos;
    }

    timeTestEncoder = millis();
    pos = newPos;
  }

  return incDec;
}

// ***********************************************************************************************************
// ClockGenerator nextMenuValue
void ClockGeneratorClass::nextDisplayValue(int incDec) {

  boolean treated = false;

	if (incDec < 0) {

		switch (editMode) {

			case display_clocks_c0: editMode = display_clocks_options; treated = true; break;
			case display_range: editMode = display_clock_exit; treated = true; break;
			case display_store: editMode = display_options_exit; treated = true; break;
			case display_storeA: editMode = display_store_exit; treated = true; break;
			case display_nbSweep: editMode = display_sweep_exit; treated = true; break;
			case display_activateSweep: editMode = display_SweepCx_exit; treated = true; break;

		} // switch

		// all other display cases
		if (!treated) editMode = static_cast<runMode>(static_cast<int>(editMode) -1);

	} // if

	if (incDec > 0) {

		switch (editMode) {

			case display_clocks_options: editMode = display_clocks_c0; treated = true; break;
			case display_clock_exit: editMode = display_range; treated = true; break;
			case display_options_exit: editMode = display_store; treated = true; break;
			case display_store_exit: editMode = display_storeA; treated = true; break;
			case display_sweep_exit: editMode = display_nbSweep; treated = true; break;
			case display_SweepCx_exit: editMode = display_activateSweep; treated = true; break;

		} // switch

		// all other cases
		if (!treated) editMode = static_cast<runMode>(static_cast<int>(editMode) +1);

	} // if

	// Set selectedClock
	switch (editMode) {
		case display_clocks_c0: selectedClock = 0; break;
		case display_clocks_c1: selectedClock = 1; break;
		case display_clocks_c2: selectedClock = 2; break;
	}

}

// ***********************************************************************************************************
// ClockGenerator computeSweepState
swState ClockGeneratorClass::computeSweepState() {

	boolean state = false;

  for (int i = 0; i < nbClock; i++) {
		if (tbClock[i].as) state = true;
	}

	if (state) return HOLD;
	else return INACTIVE;
}

// ***********************************************************************************************************
// ClockGenerator timerHandler
void ClockGeneratorClass::timerHandler() {
	countSweep++;
	timeToSweep = true;
}

// ***********************************************************************************************************
// ClockGenerator setTimer
void ClockGeneratorClass::setTimer(float period) {
	timer1->pause();
  timer1->setOverflow(1000000 * periodSweep, MICROSEC_FORMAT); //

  timer1->attachInterrupt(timerHandler);
  timer1->resume();

/*
	timer1->setCaptureCompare(TIMER_CH1, 1);  // Interrupt 1 count after each update
	timer1->attachInterrupt(timerHandler);
	timer1->refresh();
  timer1->resume();
*/
}

// ***********************************************************************************************************
// ClockGenerator stopTimer
void ClockGeneratorClass::stopTimer() {
	timer1->pause();
}

// ***********************************************************************************************************
// ClockGenerator startSweep
boolean ClockGeneratorClass::initSweep() {

  boolean oneActiveSweep = false;

	// Sets range and start frequency
  for (int i = 0; i<3 ; i++) {
		if 	(tbClock[i].as) {
//			tbClock[i].un = tbClock[i].rs;
			tbClock[i].fr = tbClock[i].sa;
			setFrequency(i, tbClock[i].fr);
			displayClock(i);
			oneActiveSweep = true;
		}
	}

	if (!oneActiveSweep) return false;

	//Serial.print("Period sweep: ");
	//Serial.println(periodSweep);
	//Serial.print("Count sweep: ");
	//Serial.println(countSweep);

  countSweep = 0;
	setTimer(periodSweep);
  sweepState = RUNNING;
  printSweep();
  return true;

}

// ***********************************************************************************************************
// ClockGenerator sweep
void ClockGeneratorClass::sweep() {
	long freqStep;
 // long frequency;

  for (int i = 0; i<3 ; i++) {
	// Each clock that has sweep activated
		if 	(tbClock[i].as) {
			freqStep = (tbClock[i].so - tbClock[i].sa) / nbPeriodSweep;
			tbClock[i].fr += freqStep;
			setFrequency(i, tbClock[i].fr);
//			displayClock(i);
			displayFrequency(i);
		} // if
	}	// for
}

// ***********************************************************************************************************
// ClockGenerator stopSweep
void ClockGeneratorClass::stopSweep() {
	stopTimer();
	sweepState = HOLD;
	printSweep();
	countSweep = 0;
}

// ***********************************************************************************************************
// ClockGenerator testSweep
void ClockGeneratorClass::testSweep() {

	if (timeToSweep==true) {
		if ((countSweep > 1) and (countSweep <= nbPeriodSweep +1)) {
			//Serial.println(millis());
			sweep();
			timeToSweep=false;
		}
		if (countSweep == nbPeriodSweep +1){
			stopSweep();
		}
	}

}

//=================================== Si5351 programming and clock parameters computing =========================================

// ***********************************************************************************************************
// ClockGenerator computePhaseTied
// Computes if the phase of this clock is tied to another or not
boolean ClockGeneratorClass::computePhaseTied(int clockId) {

	boolean phaseTied = false;

  switch (PLLAClks) {

    case PLLA_3_Clocks:
    phaseTied = true;
    break;

    case PLLA_CK0_CK1:
		if ((clockId == 0) || (clockId == 1))  phaseTied = true;
    break;

    case PLLA_CK0_CK2:
		if ((clockId == 0) || (clockId == 2))  phaseTied = true;
    break;

    case PLLA_CK1_CK2:
		if ((clockId == 1) || (clockId == 2))  phaseTied = true;
    break;
  }

  return phaseTied;
}

// ***********************************************************************************************************
// ClockGenerator setPhase
// Sets the phase property of one clock, and program the Si5351
void ClockGeneratorClass::setPhase(int clockId, byte phaseIndice) {

	if (computePhaseTied(clockId)) {

		if (phaseIndice  > 127) phaseIndice = 127;
//    if (phaseIndice  < 1) phaseIndice = 1;

		// Limits the phase to 360°
		if (getPhaseStep(clockId) * (phaseIndice) > 365) {
	    phaseIndice = (byte)(360/phaseStep);
	  }

		// If the frequency is under the minimum defined as const, sets phase to 0
		if (tbClock[clockId].fr < frMinPhase) phaseIndice = 0;

		// Set the phase parameter
    tbClock[clockId].ph = phaseIndice;

    clk = static_cast<si5351_clock>(clockId);

    mySi5351.set_phase(clk, tbClock[clockId].ph);
    mySi5351.pll_reset(SI5351_PLLA);
    delay(100);

  }
}

// ***********************************************************************************************************
// ClockGenerator getPhase
// Get the phase property of one clock
float ClockGeneratorClass::getPhase(int clockId) {

	return getPhaseStep(clockId) * tbClock[clockId].ph;

}

// ***********************************************************************************************************
// ClockGenerator setDrive
// Sets the drive property of one clock and program the Si5351
void ClockGeneratorClass::setDrive(int clockId, byte dr) {

  si5351_drive drive = static_cast<si5351_drive>(dr);

	if (drive < SI5351_DRIVE_2MA) drive = SI5351_DRIVE_2MA;
	if (drive > SI5351_DRIVE_8MA) drive = SI5351_DRIVE_8MA;

  tbClock[clockId].dr = drive;
  clk = static_cast<si5351_clock>(clockId);
  mySi5351.drive_strength(clk, drive);
}

// ***********************************************************************************************************
// ClockGenerator getDrive
// Gets the drive property of one clock
byte ClockGeneratorClass::getDrive(int clockId) {

  return (byte)tbClock[clockId].dr;

}

// ***********************************************************************************************************
// ClockGenerator changeClockOutputState
// Activate, de-activate or invert on clock
void ClockGeneratorClass::changeClockOutputState(int clockId) {

/*
  Serial.print("ac: ");
  Serial.println(oneClock.ac);
  Serial.print("in: ");
  Serial.println(oneClock.in);
*/

  if ((tbClock[clockId].ac == true) && (tbClock[clockId].in == true)) { // Invert clock
    mySi5351.set_clock_invert(static_cast<si5351_clock>(clockId), 1);
  } else if (tbClock[clockId].ac == 0) { // de-activate clock
    mySi5351.output_enable(static_cast<si5351_clock>(clockId), 0);
  } else { // activate clock
    mySi5351.set_clock_invert(static_cast<si5351_clock>(clockId), 0);
    mySi5351.output_enable(static_cast<si5351_clock>(clockId), 1);
  }

}

// ***********************************************************************************************************
// ClockGenerator configureSi5351
// This will set Si5351 parameters so it can generate the highest frequency chosen with the highest phase resolution possible.
// And send clock parameters to Si5351
void ClockGeneratorClass::configureSi5351() {

 // Si pas de phase prise en compte on peut fonctionner en mode "integer" et positionner MS0_INT = 1
 // Sinon mode fraction avec MS0_INT = 0
 // A revoir, plus compliqué

 // code example with phase shift
 // D'après l'exemple si5351_phase.ino de la librairie etherkit SI5351
 // et https://groups.io/g/QRPLabs/topic/Si5351_issue/4296292?p=,,,20,0,0,0::recentpostdate%2Fsticky,,,20,2,0,4296292

  uint64_t frequency ;
  uint64_t computingFrequency;
  int PLLdivider;

  float phase;      // in degrees
  byte nbPhaseStep;
  int phaseRegisterValue;
  byte idCA, idCB;
	uint64_t pllFr;
  si5351_pll pll;

  // *********** PLLA ********************************************************

  /* To be certain the pll freq is an even multiplier of the highest frequency we
  1) determine the highest frequency clock
  2) compute the pll freq based on a even multiplier of the frequency
  3) set its frequency, letting the library re-compute the pll_freq if necessary (i.e. for 4 and 6 multipliers)
  4) program the other clocks with the pll_freq
  */

  switch (PLLAClks) {
    case PLLA_CK0: idCA = 0 ; break;
    case PLLA_3_Clocks: idCA = highestFrequencyClock(0, 1, 2); break;
    case PLLA_CK0_CK1: idCA = highestFrequencyClock(0, 1); break;
    case PLLA_CK0_CK2: idCA = highestFrequencyClock(0, 2); break;
    case PLLA_CK1_CK2: idCA = highestFrequencyClock(1, 2); break;
  }

  computingFrequency = tbClock[idCA].fr * SI5351_FREQ_MULT;
  PLLdivider = (SI5351_PLL_VCO_MIN / tbClock[idCA].fr) +1;
	if (PLLdivider % 2) PLLdivider++; // better to use even divider to set phase to 90° precisely
  mySi5351.plla_freq = PLLdivider * computingFrequency;

	//Serial.print("PLLdivider*freq = ");
	//printUint64_t(PLLdivider * computingFrequency);
	//Serial.println(" ");

  mySi5351.set_ms_source(static_cast<si5351_clock>(idCA), SI5351_PLLA);
  mySi5351.set_freq(computingFrequency, static_cast<si5351_clock>(idCA)); // computes the pll frequency and sets the frequency

  // *********** PLLB ********************************************************

  if (PLLAClks  != PLLA_3_Clocks) {   // We need to setup PLLB since one of the clocks is assigned to it

  	// Determines the highest clock, depending on what clocks are assigned to PLLA
  	switch (PLLAClks) {
  	  case PLLA_CK0: idCB = highestFrequencyClock(1, 2); break;
  	  case PLLA_CK0_CK1: idCB = 2; break;
  	  case PLLA_CK0_CK2: idCB = 1; break;
  	  case PLLA_CK1_CK2: idCB = 0; break;
  	}

		// We don't mind about the pllb_freq as compared to the output frequency since phase setting does not apply to PLLB clocks
		// But we prefer to use the lowest VCO available that is a entire multiplier of the reference clock to reduce noise

 		PLLdivider = (SI5351_PLL_VCO_MIN / xtalFreq) +1;
  	mySi5351.pllb_freq = PLLdivider * xtalFreq * SI5351_FREQ_MULT;

  	mySi5351.set_ms_source(static_cast<si5351_clock>(idCB), SI5351_PLLB);
  	mySi5351.set_freq(tbClock[idCB].fr*SI5351_FREQ_MULT, static_cast<si5351_clock>(idCB)); // computes the pll frequency and sets the frequency



  }

	Serial.print("plla_freq: ");
	Serial.print(" ");
	printUint64_t(mySi5351.plla_freq);
	Serial.println(" ");
	Serial.print("pllb_freq: ");
	Serial.print(" ");
	printUint64_t(mySi5351.pllb_freq);
	Serial.println(" ");

  // *********** Set each clock frequency, phase, drive and outputState *********
  for (int i = 0; i<3 ; i++) {

    // set pll and frequency of clocks that were not set
  	if ((i != idCA) && (i != idCB)) {
  		getPLLSettings(i, pll, pllFr);
  	  mySi5351.set_ms_source(static_cast<si5351_clock>(i), pll); // setFrequency does not set the pll of the clock
  	  mySi5351.set_freq_manual(tbClock[i].fr*SI5351_FREQ_MULT, pllFr, static_cast<si5351_clock>(i));
			// setFrequency(i, tbClock[i].fr); // i.e. set_freq_manual, since we have set the pll frequency already
		}

    // Set phase, drive, output state
    mySi5351.set_phase(static_cast<si5351_clock>(i), tbClock[i].ph);
    mySi5351.drive_strength(static_cast<si5351_clock>(i),tbClock[i].dr);
    changeClockOutputState(tbClock[i].id);
  }

  mySi5351.pll_reset(SI5351_PLLA);
  mySi5351.pll_reset(SI5351_PLLB);

}

// ***********************************************************************************************************
// ClockGenerator setFrequency
// Sets the frequency adjusted with the calibration value
void ClockGeneratorClass::setFrequency(int clockId, uint32_t frequency) {
	uint64_t pllFr;
  si5351_pll pll;

	// DEBUG
  //Serial.print("setFreq: ");
  //Serial.println(frequency);

  tbClock[clockId].fr = frequency;

	// IL Y A UN BUG, PAS MIS A JOUR TOUT LE TEMPS...
  if (frequency<1000000) {
		tbClock[clockId].un = 0;
	} else {
		tbClock[clockId].un = 1;
  }

	uint64_t progFreq = frequency*(SI5351_FREQ_MULT + calibrationPPM/10000) ;

  mySi5351.set_freq(progFreq, static_cast<si5351_clock>(clockId)); // computes the pll frequency if needed and sets the frequency

//  getPLLSettings(clockId, pll, pllFr);
//  mySi5351.set_freq_manual(tbClock[clockId].fr*SI5351_FREQ_MULT, pllFr, static_cast<si5351_clock>(clockId));
//  will recompute all so all the clocks are properly set on the same fresh pll_freq
//  configureSi5351();

}


// ***********************************************************************************************************
// ClockGenerator getFrequency
// Returns the current frequency of the clock
uint32_t ClockGeneratorClass::getFrequency(int clockId) {

	return tbClock[clockId].fr;

}

// ***********************************************************************************************************
// ClockGenerator getPLLSettings
void ClockGeneratorClass::getPLLSettings(int clockId, si5351_pll& pll, uint64_t& pllFr) {

	pll = SI5351_PLLB;
	pllFr = mySi5351.pllb_freq;

	switch (PLLAClks) {

	  case PLLA_3_Clocks:
	  pll = SI5351_PLLA;
	  pllFr = mySi5351.plla_freq;
	  break;

	  case PLLA_CK0_CK1:
 	  if ((clockId == 0) || (clockId == 1)) { pll = SI5351_PLLA; pllFr = mySi5351.plla_freq; }
	  break;

	  case PLLA_CK0_CK2:
  	if ((clockId == 0) || (clockId == 2))  { pll = SI5351_PLLA; pllFr = mySi5351.plla_freq; }
	  break;

	  case PLLA_CK1_CK2:
		if ((clockId == 1) || (clockId == 2))  { pll = SI5351_PLLA; pllFr = mySi5351.plla_freq; }
	  break;
	}

}

//=================================== Display functions ========================================================================

// ***********************************************************************************************************
// ClockGenerator testdrawtext
void ClockGeneratorClass::testdrawtext(char *text, uint16_t color) {
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

// ***********************************************************************************************************
// ClockGenerator roundFrequency
// Rounds the frequency on the 6th digit max
uint64_t ClockGeneratorClass::roundFrequency(uint64_t frequency) {
  int divFactor = 1;

  //Serial.print("frequency to round: ");
  //Serial.println(frequency);

 if (frequency >= 100000000) {
    divFactor = 10;
  }

  return (long)(divFactor * floor(frequency/divFactor));
}

// ***********************************************************************************************************
// ClockGenerator formatFrequency
void ClockGeneratorClass::formatFrequency(uint64_t frequency, int unit, char formatedFreq[]) {
  byte width;
  byte precision;

	double freqDouble = frequency;

  width = 9;

  if (unit == 0 ) { //0: KHz
    if (freqDouble <  10000) {
      width = 4;
    } else if  (freqDouble <  100000) {
      width = 5;
    } else {
			width = 6;
		}
    precision = 3;
    freqDouble /= 1000;
  }
  else { // MHz
    if (freqDouble >= 100000000) {
      precision = 6;
    }
    else {
      precision = 6;
    }
    freqDouble /= 1000000;
  }

  dtostrf(freqDouble, width, precision, formatedFreq );
  // debug
  //  Serial.println(formatedFreq);
}

// ***********************************************************************************************************
// ClockGenerator elementPosition
// Provides the position of each element printed on the TFT display
void ClockGeneratorClass::elementPosition(runMode element, byte &line, byte &column, boolean &twoColumns) {

  twoColumns = false;
  column = 1;

  switch (element) {

		case display_clocks_c0:
		line = 1;
		twoColumns = true;
		break;

	 	case display_clocks_c1:
		line = 3;
		twoColumns = true;
		break;

	 	case 	display_clocks_c2:
		line = 5;
		twoColumns = true;
		break;

	 	case 	display_clocks_sweep:
		line = 7;
		twoColumns = true;
		break;

	 	case 	display_clocks_options:
		line = 7;
		column = 2;
		twoColumns = true;
		break;

	 	case 	display_range:
		line = 2;
		break;

	 	case 	display_freqStep:
		line = 3;
		break;

	 	case 	display_frequency:
		line = 4;
		break;

	 	case 	display_drive:
		line = 5;
		break;

	 	case 	display_phase:
		line = 6;
		break;

	 	case 	display_clock_exit:
		line = 7;
		break;

	 	case 	display_store:
		line = 2;
		break;

	 	case 	display_sweep:
		line = 3;
		break;

	 	case 	display_calibration:
		line = 4;
		break;

	 	case 	display_phaseTied:
		line = 5;
		break;

	 	case 	display_factory_reset:
		line = 6;
		break;

	 	case 	display_options_exit:
		line = 7;
		break;

	 	case 	display_storeA:
		line = 2;
		break;

	 	case 	display_storeB:
		line = 3;
		break;

	 	case 	display_retrieveA:
		line = 4;
		break;

	 	case 	display_retrieveB:
		line = 5;
		break;

	 	case 	display_store_exit:
		line = 7;
		break;

	 	case 	display_nbSweep:
		line = 2;
		break;

	 	case 	display_periodeSweep:
		line = 3;
		break;

	 	case 	display_C0Sweep:
		line = 4;
		break;

	 	case 	display_C1Sweep:
		line = 5;
		break;

	 	case 	display_C2Sweep:
		line = 6;
		break;

	 	case 	display_sweep_exit:
		line = 7;
		break;

	 	case 	display_activateSweep:
		line = 2;
		break;

	 	case 	display_rangeSweep:
		line = 3;
		break;

	 	case 	display_stepSweep:
		line = 4;
		break;

	 	case 	display_startSweep:
		line = 5;
		break;

	 	case 	display_stopSweep:
		line = 6;
		break;

	 	case 	display_SweepCx_exit:
		line = 7;
	 	break;

	}

}

// ***********************************************************************************************************
// ClockGenerator displayElement
// displays an element depending on its type defined as a runMode enum value
// returns the current yPos
byte ClockGeneratorClass::displayElement(runMode elementType, char elementText[], uint16_t color) {

  byte line, column;
  boolean twoColumns;
	byte xPos, yPos;

  elementPosition(elementType, line, column, twoColumns);

	xPos = (column -1) * 65;
  yPos = line * (lineHeight + interLine);

  tft.setCursor(xPos, yPos);
  testdrawtext(elementText, color);

	return yPos;
}

// ***********************************************************************************************************
// ClockGenerator displayCursor
// When defined by the parameter "elementType"
void ClockGeneratorClass::displayCursor(runMode elementType, boolean disp) {

	byte line, column;
	boolean twoColumns;

	elementPosition(elementType, line, column, twoColumns);

	column -=1;
  byte baseline = line * (lineHeight + interLine);
  tft.fillRect(0, 0, 7, 128, ST7735_BLACK);

	if (twoColumns) tft.fillRect(56, 7 *(lineHeight + interLine) -15, 7, 16, ST7735_BLACK);

  if (disp == true) {
    tft.fillTriangle(column*56, baseline -12, column*56+6, baseline -6, column*56, baseline, ST7735_WHITE);
  }

}

// ***********************************************************************************************************
// ClockGenerator displayCursor
// When defined by the runMode context
void ClockGeneratorClass::displayCursor(boolean disp) {

	displayCursor(editMode, disp) ;

}


// ***********************************************************************************************************
// ClockGenerator flashTrice
void ClockGeneratorClass::flashTrice(byte line) {

//  displayCursor(line,false);
  displayCursor(false);
  delay(100);
//  displayCursor(line,true);
  displayCursor(true);
  delay(100);
//  displayCursor(line,false);
  displayCursor(false);
  delay(100);
//  displayCursor(line,true);
  displayCursor(true);
  delay(100);
//  displayCursor(line,false);
  displayCursor(false);
  delay(100);
//  displayCursor(line,true);
  displayCursor(true);

}

// ***********************************************************************************************************
// ClockGenerator displayIcon
void ClockGeneratorClass::displayIcon(byte line, boolean active, boolean inv) {

  byte baseline = line * (lineHeight + interLine);

  if ((inv == true)  && (active == true)) {
    tft.drawCircle(14, baseline - 6, 3, colorInverted);
  }
  else if (active == true) {
    tft.fillCircle(14, baseline - 6, 3, colorActive);
  } else {
    tft.fillCircle(14, baseline - 6, 3, ST7735_BLACK);
  }

}

// ***********************************************************************************************************
// ClockGenerator flashCursor
void ClockGeneratorClass::flashCursor() {

  if ((millis() - flashStamp) > flashPeriod) {
    flash = !flash;
    displayCursor(flash);

    flashStamp = millis();
  }

}

// ***********************************************************************************************************
// ClockGenerator displayClock
// Displays one clock data on the main page
void ClockGeneratorClass::displayClock(int clockId) {
  char frChar[11];
  float phaseFloat;
  char phase[5];
  char line1[15];
  char line2[6];
  char line3[6];
  uint16_t color;

  byte baseline = lineHeight + 2 * clockId * (lineHeight + interLine);

  formatFrequency(tbClock[clockId].fr, tbClock[clockId].un, frChar);

  sprintf(line1, "%-10s", frChar);
  sprintf(line2, "%-i mA", driveDisplayValue(tbClock[clockId].dr));

  phaseFloat = tbClock[clockId].ph * getPhaseStep(clockId);
  dtostrf(phaseFloat, 3, 1, phase );
  sprintf(line3, "%-3s", phase);

  tft.setFont(&FreeSans9pt7b);
  tft.fillRect(0, baseline-lineHeight+interLine, 128, 2*lineHeight + interLine , 0);

  color = colorInactive;

  if ((tbClock[clockId].ac == true) && (tbClock[clockId].in == true)) {
      color = colorInverted;

  } else if (tbClock[clockId].ac == true) {
    color = colorActive;
  }

  // Displays formated lines
  tft.setCursor(20,baseline+2);
  testdrawtext(line1, color);

  tft.setCursor(114,baseline +2);
  if (tbClock[clockId].un == 0) testdrawtext("K", color);
  if (tbClock[clockId].un == 1) testdrawtext("M", color);

  tft.setCursor(20,baseline + lineHeight + interLine);
  testdrawtext(line2, color);

	// Prints phase only when phased tied
  if (computePhaseTied(clockId)) {

	  if (phaseFloat < 10.0) {
	  	tft.setCursor(86,baseline + lineHeight + interLine);
	  } else if (phaseFloat < 100.0) {
	  	tft.setCursor(76,baseline + lineHeight + interLine);
		} else {
	  	tft.setCursor(66,baseline + lineHeight + interLine);
		}

	  testdrawtext(line3, color);
	  tft.setCursor(116,baseline + lineHeight + interLine);
	  testdrawtext("d", color);

  }


  tft.drawFastHLine(12,baseline + lineHeight + 2* interLine, 116, color ),
  displayIcon(clockId*2+1,tbClock[clockId].ac,tbClock[clockId].in) ;
}

// ***********************************************************************************************************
// ClockGenerator displayFrequency
// Displays only the frequency of a clock, used when sweeping
void ClockGeneratorClass::displayFrequency(int clockId) {
  char frChar[11];
  float phaseFloat;
  char phase[5];
  char line1[15];
  char line2[6];
  char line3[6];
  uint16_t color;

  byte baseline = lineHeight + 2 * clockId * (lineHeight + interLine);

  formatFrequency(tbClock[clockId].fr, tbClock[clockId].un, frChar);

  sprintf(line1, "%-10s", frChar);
  tft.setFont(&FreeSans9pt7b);
  tft.fillRect(20, baseline-lineHeight+interLine, 93, lineHeight, 0);

  color = colorInactive;

  if ((tbClock[clockId].ac == true) && (tbClock[clockId].in == true)) {
      color = colorInverted;

  } else if (tbClock[clockId].ac == true) {
    color = colorActive;
  }

  // Displays formated lines
  tft.setCursor(20,baseline+2);
  testdrawtext(line1, color);
}

// ***********************************************************************************************************
// ClockGenerator printSweep
void ClockGeneratorClass::printSweep() {

  int color;

	if (sweepState != RUNNING) sweepState = computeSweepState();

	switch(sweepState) {
		case INACTIVE: color = colorSweepInactive; break;
		case HOLD: color = colorSweepHold; break;
		case RUNNING: color = colorSweepRunning; break;
	}

  tft.fillRoundRect(8, 112, 40, 16, 0, color);
  tft.setFont(&Org_01);
  tft.setCursor(14,122);
  testdrawtext("SWEEP", ST7735_BLACK);
}

// ***********************************************************************************************************
// ClockGenerator displayAllClocks
// Displays the main page
void ClockGeneratorClass::displayAllClocks() {

  tft.setFont(&FreeSans9pt7b);
  tft.fillScreen(ST7735_BLACK);

  for (int i = 0; i < nbClock; i++) displayClock(i);

	printSweep();

  tft.fillRoundRect(65, 112, 63, 16, 0, ST7735_YELLOW);
  tft.setFont(&Org_01);
  tft.setCursor(75,122);
  testdrawtext("OPTIONS", ST7735_BLACK);

	displayCursor(true);
}

// ***********************************************************************************************************
// ClockGenerator printRange
void ClockGeneratorClass::printRange(Clock &oneClock) {
  tft.setCursor(10, 2*(lineHeight + interLine));
  if (oneClock.un == 0) testdrawtext("Range: KHz", ST7735_WHITE);
  else if (oneClock.un == 1) testdrawtext("Range: MHz", ST7735_WHITE);
}

// ***********************************************************************************************************
// ClockGenerator printStep
void ClockGeneratorClass::printStep (Clock &oneClock) {
  float freqStep = oneClock.st;
  char freqStepLib[11] ;
  char line[15];

  if (freqStep == 1.0) strcpy(freqStepLib, "1/10 Hz ");
  else if (freqStep == 10.0) strcpy(freqStepLib, "10/100 Hz");
  else if (freqStep == 100.0) strcpy(freqStepLib, "0.1/1 KHz");
  else if (freqStep == 1000.0) strcpy(freqStepLib, "1/10 KHz");
  else if (freqStep == 10000.0) strcpy(freqStepLib, "10/100 KHz");
  else if (freqStep == 100000.0) strcpy(freqStepLib, "0.1/1 MHz");
  else if (freqStep == 1000000.0) strcpy(freqStepLib, "1/10 MHz");

  sprintf(line, "S: %-s", freqStepLib);
  tft.setCursor(10, 3*(lineHeight + interLine));
  testdrawtext(line, ST7735_WHITE);
}

// ***********************************************************************************************************
// ClockGenerator printFreq
void ClockGeneratorClass::ClockGeneratorClass::printFreq (Clock &oneClock) {
  char line[15];
  char frChar[11];

  formatFrequency(oneClock.fr, oneClock.un, frChar);

  if (oneClock.un == 0) sprintf(line, "F: %-6s", frChar);
  if (oneClock.un == 1) sprintf(line, "F: %-10s", frChar);

  tft.setCursor(10, 4*(lineHeight + interLine));
  testdrawtext(line, ST7735_WHITE);

}

// ***********************************************************************************************************
// ClockGenerator driveDisplayValue
byte ClockGeneratorClass::driveDisplayValue(si5351_drive drive) {

  switch(drive) {
		case SI5351_DRIVE_2MA: return 2;
		case SI5351_DRIVE_4MA: return 4;
		case SI5351_DRIVE_6MA: return 6;
		case SI5351_DRIVE_8MA: return 8;
  }
}

// ***********************************************************************************************************
// ClockGenerator printDrive
void ClockGeneratorClass::printDrive (Clock &oneClock) {
  char line[15];
  byte drive;

  switch(oneClock.dr) {

		case SI5351_DRIVE_2MA: drive = 2; break;
		case SI5351_DRIVE_4MA: drive = 4; break;
		case SI5351_DRIVE_6MA: drive = 6; break;
		case SI5351_DRIVE_8MA: drive = 8; break;

	}

  sprintf(line, "D: %-4i mA", drive);
  tft.setCursor(10, 5*(lineHeight + interLine));
  testdrawtext(line, ST7735_WHITE);
}

// ***********************************************************************************************************
// ClockGenerator printPhase
void ClockGeneratorClass::printPhase(Clock &oneClock) {
  char line[15];
  float phaseFloat;
  char phase[6];

// A revoir .ps incohérent avec phaseTied etc

  if (!computePhaseTied(selectedClock)) {
    sprintf(line, "P: IND");
  } else {
		// If the frequency is under the minimum defined as const, sets phase to 0
		if (tbClock[selectedClock].fr < frMinPhase) {
  	  sprintf(line, "P: F LOW");
		} else {
    	phaseFloat = oneClock.ph * getPhaseStep(oneClock.id);
    	dtostrf(phaseFloat, 3, 1, phase );
    	sprintf(line, "P: %-3s d", phase);
		}
  }

  tft.setCursor(10, 6*(lineHeight + interLine));
  testdrawtext(line, ST7735_WHITE);

}

// ***********************************************************************************************************
// ClockGenerator edit1Clock
// Displays the clock paramter setings page
void ClockGeneratorClass::edit1Clock(Clock &oneClock, boolean refresh) {

  char line[15];

  tft.setFont(&FreeSans9pt7b);

  if (!refresh) {

    sprintf(line, "CLOCK %8i", oneClock.id+1);
    tft.fillScreen(ST7735_BLACK);
    tft.fillRect(7, 0, 121, 18, ST7735_YELLOW);
    tft.setCursor(10,lineHeight);
    testdrawtext(line, ST7735_BLACK);

    printRange(oneClock);
    printStep(oneClock);
    printFreq(oneClock);
    printDrive(oneClock);
    printPhase(oneClock);

  } else {

     switch (editMode) {

      case display_range: tft.fillRect(10, 1*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK); printRange(oneClock); break;
      case display_freqStep: tft.fillRect(10, 2*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK); printStep(oneClock); break;
      case display_frequency:
      {
				tft.fillRect(10, 3*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK);
				printFreq(oneClock);
				tft.fillRect(10, 5*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK);
				printPhase(oneClock);
				break;
		  }
      case display_drive: tft.fillRect(10, 4*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK); printDrive(oneClock); break;
      case display_phase: tft.fillRect(10, 5*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK); printPhase(oneClock); break;

    }
  }

	// exit
	printExit();
  displayCursor(true);

}

// ***********************************************************************************************************
// ClockGenerator printPhaseOptions
void ClockGeneratorClass::printPhaseOptions(boolean refresh) {

  if (refresh == true) tft.fillRect(68, 4*(lineHeight + interLine)+1, 60, lineHeight + interLine,  ST7735_BLACK);

  tft.setCursor(10, 5*(lineHeight + interLine));
  testdrawtext("Ph tied:", ST7735_WHITE);

  switch(PLLAClks) {
    case PLLA_CK0: testdrawtext(" None", ST7735_WHITE); break;
    case PLLA_3_Clocks: testdrawtext(" 1-2-3", ST7735_WHITE); break;
    case PLLA_CK0_CK1: testdrawtext(" 1-2", ST7735_WHITE); break;
    case PLLA_CK0_CK2: testdrawtext(" 1-3", ST7735_WHITE); break;
    case PLLA_CK1_CK2: testdrawtext(" 2-3", ST7735_WHITE); break;
  }

}

// ***********************************************************************************************************
// ClockGenerator printCal
void ClockGeneratorClass::ClockGeneratorClass::printCal(boolean refresh) {
  char cal[6];
  char line[15];

  if (refresh == true) tft.fillRect(40, 3*(lineHeight + interLine)+4, 88, lineHeight + interLine,  ST7735_BLACK);

  dtostrf(calibrationPPM, 3, 2, cal );
  sprintf(line, "Cal: %-3s ppm", cal);

  tft.setCursor(10, 4*(lineHeight + interLine));
  testdrawtext(line, ST7735_WHITE);

}


// ***********************************************************************************************************
// ClockGenerator displayOptions
void ClockGeneratorClass::displayOptions() {

  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSans9pt7b);

  tft.fillRoundRect(8, 0, 120, lineHeight + interLine, 0, ST7735_YELLOW);
  tft.setCursor(10,lineHeight);
  testdrawtext("OPTIONS", ST7735_BLACK);
  tft.setCursor(10, 2*(lineHeight + interLine));
  testdrawtext("Store", ST7735_WHITE);
  tft.setCursor(115, 2*(lineHeight + interLine));
  testdrawtext(">", ST7735_WHITE);
  tft.setCursor(10, 3*(lineHeight + interLine));
  testdrawtext("Sweep", ST7735_WHITE);
  tft.setCursor(115, 3*(lineHeight + interLine));
  testdrawtext(">", ST7735_WHITE);

	printCal(false);
  printPhaseOptions(false);

  tft.setCursor(10, 6*(lineHeight + interLine));
  testdrawtext("Fact. reset", ST7735_WHITE);

	printExit();

}

// ***********************************************************************************************************
// ClockGenerator displayStore
void ClockGeneratorClass::displayStore() {

  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSans9pt7b);

  tft.fillRoundRect(8, 0, 120, lineHeight + interLine, 0, ST7735_YELLOW);
  tft.setCursor(10,lineHeight);
  testdrawtext("STORE", ST7735_BLACK);
  tft.setCursor(10, 2*(lineHeight + interLine));
  testdrawtext("STORE A", ST7735_WHITE);
  tft.setCursor(10, 3*(lineHeight + interLine));
  testdrawtext("STORE B", ST7735_WHITE);
  tft.setCursor(10, 4*(lineHeight + interLine));
  testdrawtext("RETRIEVE A", ST7735_WHITE);
  tft.setCursor(10, 5*(lineHeight + interLine));
  testdrawtext("RETRIEVE B", ST7735_WHITE);

	printExit();

}

// ***********************************************************************************************************
// ClockGenerator printNbPeriodSweep
void ClockGeneratorClass::printNbPeriodSweep(boolean refresh) {
  char nb[6];
  char line[15];

  if (refresh == true) tft.fillRect(60, 1*(lineHeight + interLine)+4, 68, lineHeight + interLine,  ST7735_BLACK);

  dtostrf(nbPeriodSweep, 5, 0, nb);
//  sprintf(line, "Steps: %-5s ", nb);

  sprintf(line, "Steps: %-5u ", nbPeriodSweep);

  tft.setCursor(10, 2*(lineHeight + interLine));
  testdrawtext(line, ST7735_WHITE);

}

// ***********************************************************************************************************
// ClockGenerator printPeriodSweep
void ClockGeneratorClass::printPeriodSweep(boolean refresh) {

  char per[6];
  char line[15];

  if (refresh == true) tft.fillRect(70, 2*(lineHeight + interLine)+4, 58, lineHeight + interLine,  ST7735_BLACK);

  dtostrf(periodSweep, 2, 1, per);
  sprintf(line, "Period: %-3s s", per);

  tft.setCursor(10, 3*(lineHeight + interLine));
  testdrawtext(line, ST7735_WHITE);
}

// ***********************************************************************************************************
// ClockGenerator printInfoSweepCx
void ClockGeneratorClass::ClockGeneratorClass::printInfoSweepCx (int clockId) {
  char line[15];
  char frChar[10];

  formatFrequency(tbClock[clockId].sa, tbClock[clockId].rs, frChar);

	if (!tbClock[clockId].as) sprintf(line, "C%i: inactive", clockId+1);
  else {
	  if (tbClock[clockId].rs == 0) sprintf(line, "C%i: %-6s K", clockId+1, frChar);
	  if (tbClock[clockId].rs == 1) sprintf(line, "C%i: %-6s M", clockId+1, frChar);
	}

  byte baseline =  (4 + clockId) * (lineHeight + interLine);

  tft.setCursor(10, baseline);
  testdrawtext(line, ST7735_WHITE);
  tft.setCursor(115, baseline);
  testdrawtext(">", ST7735_WHITE);

}

// ***********************************************************************************************************
// ClockGenerator displaySweep
// Dislays global sweep settings
void ClockGeneratorClass::displaySweep() {

  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSans9pt7b);

  tft.fillRoundRect(8, 0, 120, lineHeight + interLine, 0, ST7735_YELLOW);
  tft.setCursor(10,lineHeight);
  testdrawtext("SWEEP", ST7735_BLACK);

	printNbPeriodSweep(false);
	printPeriodSweep(false);
  printInfoSweepCx(0);
  printInfoSweepCx(1);
  printInfoSweepCx(2);

/*
  tft.setCursor(10, 4*(lineHeight + interLine));

  testdrawtext("C1: 1.000 M", ST7735_WHITE);
  tft.setCursor(115, 4*(lineHeight + interLine));
  testdrawtext(">", ST7735_WHITE);
  tft.setCursor(10, 5*(lineHeight + interLine));
  testdrawtext("C2: 999.0 K", ST7735_WHITE);
  tft.setCursor(115, 5*(lineHeight + interLine));
  testdrawtext(">", ST7735_WHITE);
  tft.setCursor(10, 6*(lineHeight + interLine));
  testdrawtext("C3: inactive", ST7735_WHITE);
  tft.setCursor(115, 6*(lineHeight + interLine));
  testdrawtext(">", ST7735_WHITE);
*/

	printExit();

}

// ***********************************************************************************************************
// ClockGenerator printSweepActivate
// Display each click sweep activate status
void ClockGeneratorClass::printSweepActivate(Clock &oneClock, boolean refresh) {

	if (refresh) tft.fillRect(10, 1*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK);
	tft.setCursor(10, 2*(lineHeight + interLine));
	if (oneClock.as) testdrawtext("Active: yes", ST7735_WHITE);
	else testdrawtext("Active: no", ST7735_WHITE);

 }

// ***********************************************************************************************************
// ClockGenerator printSweepRange
void ClockGeneratorClass::printSweepRange(Clock &oneClock, boolean refresh) {

	if (refresh) tft.fillRect(10, 2*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK);

  tft.setCursor(10, 3*(lineHeight + interLine));
  if (oneClock.rs == 0) testdrawtext("Range: KHz", ST7735_WHITE);
  else if (oneClock.rs == 1) testdrawtext("Range: MHz", ST7735_WHITE);
}

// ***********************************************************************************************************
// ClockGenerator printSweepStep
void ClockGeneratorClass::printSweepStep (Clock &oneClock, boolean refresh) {
  float freqStep = oneClock.ss;
  char freqStepLib[11] ;
  char line[15];

	if (refresh) tft.fillRect(10, 3*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK);

  if (freqStep == 1.0) strcpy(freqStepLib, "1/10 Hz ");
  else if (freqStep == 10.0) strcpy(freqStepLib, "10/100 Hz");
  else if (freqStep == 100.0) strcpy(freqStepLib, "0.1/1 KHz");
  else if (freqStep == 1000.0) strcpy(freqStepLib, "1/10 KHz");
  else if (freqStep == 10000.0) strcpy(freqStepLib, "10/100 KHz");
  else if (freqStep == 100000.0) strcpy(freqStepLib, "0.1/1 MHz");
  else if (freqStep == 1000000.0) strcpy(freqStepLib, "1/10 MHz");

  sprintf(line, "S: %-s", freqStepLib);
  tft.setCursor(10, 4*(lineHeight + interLine));
  testdrawtext(line, ST7735_WHITE);
}

// ***********************************************************************************************************
// ClockGenerator printSweepStartFrequency
void ClockGeneratorClass::ClockGeneratorClass::printSweepStartFrequency (Clock &oneClock, boolean refresh) {
  char line[15];
  char frChar[10];

	if (refresh) tft.fillRect(10, 4*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK);

  formatFrequency(oneClock.sa, oneClock.rs, frChar);

  if (oneClock.un == 0) sprintf(line, "Fr: %-5s", frChar);
  if (oneClock.un == 1) sprintf(line, "Fr: %-9s", frChar);

  tft.setCursor(10, 5*(lineHeight + interLine));
  testdrawtext(line, ST7735_WHITE);

}

// ***********************************************************************************************************
// ClockGenerator printSweepStopFrequency
void ClockGeneratorClass::ClockGeneratorClass::printSweepStopFrequency (Clock &oneClock, boolean refresh) {
  char line[15];
  char frChar[10];

	if (refresh) tft.fillRect(10, 5*(lineHeight + interLine)+1, 118, lineHeight + interLine,  ST7735_BLACK);

  formatFrequency(oneClock.so, oneClock.rs, frChar);

  if (oneClock.un == 0) sprintf(line, "To: %-5s", frChar);
  if (oneClock.un == 1) sprintf(line, "To: %-9s", frChar);

  tft.setCursor(10, 6*(lineHeight + interLine));
  testdrawtext(line, ST7735_WHITE);

}

// ***********************************************************************************************************
// ClockGenerator editSweepSettings
// Displays the clock paramter setings page
void ClockGeneratorClass::editSweepSettings(Clock &oneClock, boolean refresh) {

  char line[15];

  tft.setFont(&FreeSans9pt7b);

  sprintf(line, "SWEEP %8i", oneClock.id+1);
  tft.fillScreen(ST7735_BLACK);
  tft.fillRect(7, 0, 121, 18, ST7735_YELLOW);
  tft.setCursor(10,lineHeight);
  testdrawtext(line, ST7735_BLACK);

  printSweepActivate(oneClock, refresh);
  printSweepRange(oneClock, refresh);
  printSweepStep(oneClock, refresh);
  printSweepStartFrequency(oneClock, refresh);
  printSweepStopFrequency(oneClock, refresh);

	printExit();
  displayCursor(true);

}


// ***********************************************************************************************************
// ClockGenerator displaySweepSettings
// Displays a clock sweep settings
/*

void ClockGeneratorClass::editSweepSettings(int clockId) {

	char cstr[1];
	itoa(clockId, cstr, 10);

  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSans9pt7b);

  tft.fillRoundRect(8, 0, 120, lineHeight + interLine, 0, ST7735_YELLOW);
  tft.setCursor(10,lineHeight);
  testdrawtext("SWEEP C", ST7735_BLACK);
  testdrawtext(cstr, ST7735_BLACK);
  tft.setCursor(10, 2*(lineHeight + interLine));
  testdrawtext("Active: yes", ST7735_WHITE);
  tft.setCursor(10, 3*(lineHeight + interLine));
  testdrawtext("Range: MHz", ST7735_WHITE);
  tft.setCursor(10, 4*(lineHeight + interLine));
  testdrawtext("S: 0.1/1 MHz", ST7735_WHITE);
  tft.setCursor(10, 5*(lineHeight + interLine));
  testdrawtext("From: 180.999", ST7735_WHITE);
  tft.setCursor(10, 6*(lineHeight + interLine));
  testdrawtext("To: 199.999", ST7735_WHITE);

	printExit();

  char frChar[10];
  float phaseFloat;
  char phase[5];
  char line1[15];
  char line2[15];
  uint16_t color;

  byte baseline = lineHeight + 2 * clockId * (lineHeight + interLine);

  formatFrequency(tbClock[clockId], frChar);

  sprintf(line1, "%-8s", frChar);

  phaseFloat = tbClock[clockId].ph * phaseStep;
  dtostrf(phaseFloat, 3, 1, phase );
  sprintf(line2, "%-i mA %-3s d", outputDrive[tbClock[clockId].dr], phase);

  tft.setFont(&FreeSans9pt7b);
  tft.fillRect(0, baseline-lineHeight+interLine, 128, 2*lineHeight + interLine , 0);

  // Displays formated lines
  tft.setCursor(20,baseline);
  color = colorInactive;

  if ((tbClock[clockId].ac == true) && (tbClock[clockId].in == true)) {
      color = colorInverted;

  } else if (tbClock[clockId].ac == true) {
    color = colorActive;
  }

  tft.setCursor(20,baseline);
  testdrawtext(line1, color);

  tft.setCursor(110,baseline);
  if (tbClock[clockId].un == 0) testdrawtext("K", color);
  if (tbClock[clockId].un == 1) testdrawtext("M", color);

  tft.setCursor(20,baseline + lineHeight + interLine);
  testdrawtext(line2, color);
  tft.drawFastHLine(12,baseline + lineHeight + 2* interLine, 116, color ),
  displayIcon(clockId*2+1,tbClock[clockId].ac,tbClock[clockId].in) ;

}
*/

// ***********************************************************************************************************
// ClockGenerator printExit
void ClockGeneratorClass::printExit() {

  tft.fillRoundRect(8, 112, 120, 16, 0, ST7735_YELLOW);
  tft.setFont(&Org_01);
  tft.setCursor(54,122);
  testdrawtext("EXIT", ST7735_BLACK);
	tft.setFont(&FreeSans9pt7b);

}

//========================================== BUTTON EVENT HANDLERS =============================================================

// ***********************************************************************************************************
// ClockGenerator buttonPress
void ClockGeneratorClass::buttonPress() {

	switch (editMode) {

		case display_clocks_c0:
    selectedClock = 0;
  	break;

		case display_clocks_c1:
    selectedClock = 1;
  	break;

		case display_clocks_c2:
    selectedClock = 2;
  	break;

	}

	editMode = display_range;
	clk = static_cast<si5351_clock>(selectedClock);
  edit1Clock(tbClock[selectedClock], false);

}

// ***********************************************************************************************************
// ClockGenerator buttonClick
void ClockGeneratorClass::buttonClick() {

  // Change clock status on main page Circular active - inverted - inactive - active
  // selectedClock must be correctly set
	if ((editMode == display_clocks_c0) or (editMode == display_clocks_c1) or (editMode == display_clocks_c2)) {

  	if ((tbClock[selectedClock].ac) and (!tbClock[selectedClock].in)) {
    	tbClock[selectedClock].in = true;
    }
    else if (tbClock[selectedClock].in) {
    	tbClock[selectedClock].ac = false;
     	tbClock[selectedClock].in = false;
    }
    else if (!tbClock[selectedClock].ac) {
    	tbClock[selectedClock].ac = true;
    }

    changeClockOutputState(tbClock[selectedClock].id);

    displayClock(selectedClock);
   	displayCursor(true);

    saveOneClockInFlash(tbClock[selectedClock], 0);
    return;

	} // c1, c2, c3

	// ===============================
	// When editing
	if (editing) {

		displayCursor(true); // to be certain the cursor is visible since it was flashing and might be in the hidden state

		switch (editMode) {

			case display_clocks_sweep:
			// todo
			break;

			case display_range:
			editing = false;
			break;

			case display_freqStep:
			editing = false;
			break;

			case display_frequency:
			configureSi5351();
			editing = false;
			break;

			case display_drive:
			editing = false;
			break;

			case display_phase:
			editing = false;
			break;

			case display_store:
			editing = false;
    	break;

			case display_sweep:
			editing = false;
    	break;

			case display_calibration:
			saveCalibration();
			editing = false;
			break;

			case display_phaseTied:
			setPhaseTiedClocks(PLLAClks);
			editing = false;
			break;

			case display_nbSweep:
			editing = false;
			break;

			case display_periodeSweep:
			editing = false;
			break;

			case display_activateSweep:
			editing = false;
			break;

			case display_rangeSweep:
			editing = false;
			break;

			case display_stepSweep:
			editing = false;
			break;

			case display_startSweep:
			editing = false;
			break;

			case display_stopSweep:
			editing = false;
			break;

		}

	// ===============================
	// When displaying
	} else {

		switch (editMode) {

			case display_clocks_sweep:

			if (sweepState == HOLD) {
				initSweep();
			} else if (sweepState == RUNNING) {
				stopSweep();
			}
			break;

			case display_clocks_options:
			editMode = display_store;
  		displayOptions();
   		displayCursor(true);
			break;

			case display_range:
			editing = true;
			break;

			case display_freqStep:
			editing = true;
			break;

			case display_frequency:
			editing = true;
			break;

			case display_drive:
			editing = true;
			break;

			case display_phase:
			editing = true;
			break;

			case display_clock_exit:
			// save the clock parameters
	    saveOneClockInFlash(tbClock[selectedClock], 0);
	    if  (selectedClock == 0) editMode = display_clocks_c0;
    	else if (selectedClock == 1) editMode = display_clocks_c1;
    	else editMode = display_clocks_c2;
    	displayAllClocks();
  		displayCursor(true);
    	break;

			case display_store:
			editMode = display_storeA;
			displayStore();
  		displayCursor(true);
    	break;

			case display_sweep:
			editMode = display_nbSweep;
			displaySweep();
  		displayCursor(true);
    	break;

			case display_calibration:
			editing = true;
			break;

			case display_phaseTied:
			editing = true;
			break;

			case display_factory_reset:

			//flashInit(true);
			//begin();
			EEPROM.put(flashInitAddress, flashInitFlag+1);
			NVIC_SystemReset();
			break;

			case display_options_exit:
			selectedClock = 0;
			editMode = display_clocks_c0;
			displayAllClocks();
    	displayCursor(true);
			break;

			case display_storeA:
    	saveAllClocksInFlash(1);
    	flashTrice(3);
			break;

			case display_storeB:
    	saveAllClocksInFlash(2);
    	flashTrice(4);
			break;

			case display_retrieveA:
    	readAllClocksFromFlash(1);
    	configureSi5351();
    	flashTrice(5);
			break;

			case display_retrieveB:
    	readAllClocksFromFlash(2);
    	configureSi5351();
    	flashTrice(6);
			break;

			case display_store_exit:
			saveAllClocksInFlash(0);
			editMode = display_store;
			displayOptions();
    	displayCursor(true);
			break;

			case display_nbSweep:
			editing = true;
			break;

			case display_periodeSweep:
			editing = true;
			break;

			case display_C0Sweep:
    	selectedClock = 0;
			clk = static_cast<si5351_clock>(selectedClock);
			editMode = display_activateSweep;
			editSweepSettings(tbClock[selectedClock], true);
    	displayCursor(true);
			break;


			case display_C1Sweep:
    	selectedClock = 1;
			clk = static_cast<si5351_clock>(selectedClock);
			editMode = display_activateSweep;
			editSweepSettings(tbClock[selectedClock], true);
    	displayCursor(true);
			break;

			case display_C2Sweep:
	    selectedClock = 2;
			clk = static_cast<si5351_clock>(selectedClock);
			editMode = display_activateSweep;
			editSweepSettings(tbClock[selectedClock], true);
    	displayCursor(true);
			break;

			case display_sweep_exit:
			saveSweepSettings();
			editMode = display_sweep;
			displayOptions();
			displayCursor(true);
			break;

			case display_activateSweep:
			editing = true;
			break;

			case display_rangeSweep:
			editing = true;
			break;

			case display_stepSweep:
			editing = true;
			break;

			case display_startSweep:
			editing = true;
			break;

			case display_stopSweep:
			editing = true;
			break;

			case display_SweepCx_exit:
			editMode = display_nbSweep;
			displaySweep();
			displayCursor(true);
			break;

		} // switch
	} //else not editing

}



ClockGeneratorClass ClockGenerator;