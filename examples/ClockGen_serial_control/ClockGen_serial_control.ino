/*
 * clockGen_serial_control
 * (c)2021 by pandauino.com
 *
 * This script shows an example of how to setup the Pandauino Clock generator 
 * using mySerial commands.
 * 
 * Connect the Clock Generator to your computer with an USB cable
 * It should be recognized as "STMicroelectronics Virtual COM Port (COMxx)"
 * Refer to the user's manual for proper installation / Arduino IDE set-up
 * https://github.com/mrguen/ClockGen/tree/main/doc
 * 
 * Load this sketch to the Clock Generator
 * 
 * 1) If you want to control the Clock Generator from the Serial Monitor or use another device but need to debug on the Serial Monitor
 *  Change the line #define DEBUG 0 to #define DEBUG 1
 *  Open the Serial Monitor @9600 bauds. You can send commands from the Serial Monitor
 * 
 * 2) If you want to control the Clock Generator from another device without connexion to the computer / Serial Monitor, keep the line #define DEBUG 0 
 *  You could use the sketch SendCommands.ino for that purpose
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

static const unsigned int MAX_INPUT = 50;
static char input_line [MAX_INPUT];
static unsigned int input_pos = 0;
static char delimiter[] = ";";

/* Choice of Serial port
 
 Use #define mySerial Serial 
 when you want to interact between the Serial Monitor and the Clock Generator (through the USB COM port)

 Or use #define mySerial SerialOne 
 when you want to interact between another board and the Clock Generator (using Rx and Tx pins)
*/


#define mySerial Serial1    // Use Serial1 to communicate with a standalone device through the Rx and Tx pins
//#define mySerial Serial       // Use Serial (=SerialUSB) to communicate with the Serial Monitor through the USB connector

#define DEBUG 0               // If you activate debug, the ClockGenerator must be connected with the USB cable, 
//#define DEBUG 1             // the right COM port selected and the Serial Monitor launched in order to start the Clock Generator.      
                            

//****************************************************
void info()
{
  mySerial.println("************************************************************************************************************************");  
  mySerial.println(" ");
  mySerial.println("*** ClockGen mySerial interface ***");
  mySerial.println(" ");  
  mySerial.println("cX sf (int): Clock X set frequency (c0 sf 1000000 = Clock 0 frequency set to 1 MHz)");
  mySerial.println("cX gf: Clock X get frequency");
  mySerial.println("cX gt: Clock X get phase step");
  mySerial.println("cX sp (byte): Clock X set phase indice (0 - 127) (c0sp64 = Clock 0 phase indice set to 64)");
  mySerial.println("cX gp: Clock X get phase");
  mySerial.println("cX sd (byte): Clock X set drive (0-3) (c0sd0 = Clock 0 drive set to 2 mA)");
  mySerial.println("cX gd: Clock X get drive");
  mySerial.println("cX ss (int);(int);(boolean): set clock X sweep start and stop frequency, and active/inactive (example: c0 ss 01000;2000;1)");
  mySerial.println("cX gs: get clock X sweep parameters");
  mySerial.println(" ");
  mySerial.println("sp (byte): set phase related clocks 0: all clocks, 1: clk0, 2: clko/clk1, 3: clk0/clk2, 4: clk1/clk2");
  mySerial.println("gp: get phase related clocks");
  mySerial.println("sweepset (int);(float): set sweep number of steps and period");
  mySerial.println("sweepget: get sweep number of steps and period");  
  mySerial.println("sweepstart");
  mySerial.println("sweepstop");
  mySerial.println(" ");
  mySerial.println("scal (float): set calibration in ppm");
  mySerial.println("gcal: get calibration in ppm");
  mySerial.println(" ");
  
  mySerial.println("************************************************************************************************************************");    
  mySerial.println(" ");
  
}

//****************************************************
void removeSpaces(char* dest, const char* src) 
{ 
    int i = 0, j = 0; 
    while (src[i]) 
    { 
        if (src[i] != ' ') 
           dest[j++] = src[i]; 
        i++; 
    } 
    dest[j] = '\0'; 
} 

//****************************************************
void strtolower(char* dest, const char* src) {
    while(*dest++ = tolower(*src++));
}

//****************************************************
void printFreq(int clockId) {
  mySerial.print("Clock ");
  mySerial.print(clockId);
  mySerial.print(" frequency: ");
  mySerial.print(ClockGenerator.getFrequency(clockId));
  mySerial.println(" Hz");  
}

//****************************************************
void printPhase(int clockId) {
  mySerial.print("Clock ");
  mySerial.print(clockId);
  mySerial.print(" phase: ");
  mySerial.print(ClockGenerator.getPhase(clockId), 1);
  mySerial.println(" Hz");  
}

//****************************************************
void printDrive(int clockId) {
  byte drive;
  byte driveValueMA;
  
  drive = ClockGenerator.getDrive(clockId);
  
  switch(drive) {
    case 0: driveValueMA =  2; break;
    case 1: driveValueMA =  4; break;
    case 2: driveValueMA =  6; break;
    case 3: driveValueMA =  8; break;
  }
  
  mySerial.print("Clock ");
  mySerial.print(clockId);
  mySerial.print(" drive: ");
  mySerial.print(driveValueMA);
  mySerial.println(" mA");  
}

//****************************************************
void printPhaseTied() {
  byte phTied;
  String text;
  
  phTied = ClockGenerator.getPhaseTiedClocks();
  
  switch(phTied) {
    case 0: text = "1-2-3"; break;
    case 1: text = "1"; break;
    case 2: text = "1-2"; break;
    case 3: text = "1-3"; break;
    case 4: text = "2-3"; break;
  }
  
  mySerial.print("Phase related clocks: ");
  mySerial.println(text);
}

//****************************************************
void printCxSweep(int clockId) {

  long startFreq;
  long stopFreq;
  bool active;
      
  ClockGenerator.getSweepParamCx(clockId, startFreq, stopFreq, active);

  mySerial.print("Clock ");
  mySerial.print(clockId);
  mySerial.print(", start: ");
  mySerial.print(startFreq);
  mySerial.print(", stop: ");
  mySerial.print(stopFreq);
  mySerial.print(", active: ");
  mySerial.println(active);
    
}

//****************************************************
void printSweep() {
  int nbSweep;
  float periodSweep;
      
  ClockGenerator.getSweepParam(nbSweep, periodSweep);
  mySerial.print("Sweep steps: ");
  mySerial.print(nbSweep);
  mySerial.print(", sweep period: ");
  mySerial.print(periodSweep);
  mySerial.println(" s");
      
}

//****************************************************
void printCalibration() {
  float cal;
  cal = ClockGenerator.getCalibration();
  mySerial.print("Calibration: ");
  mySerial.print(cal,2);      
  mySerial.println(" ppm");
}

//****************************************************
// Analyzes the full command that was passed including parameters 
// and calls the ClockGen functions needed to fulfil the command
// and possibly calls a print function to send back to the Master device (standalone device or Serial Monitor) the current state of the ClockGen
// For example if    
void analyzeCommand(char* data) {
  char firstLetter;
  int clockId;
  char thirdLetter;
  char fourthLetter;

  char dataTreat[50];
  char command[3];
  char buff[50];
  int value;

  removeSpaces(dataTreat, data);
  strtolower(data, dataTreat);
  
  firstLetter = data[0];
  
  //mySerial.println(data);
  //mySerial.println(firstLetter);
  //return;

  // Single Clock settings
  if ((firstLetter == 'c'))
  {
    strncpy(buff, data+1, 1);
    clockId = atoi(buff);
  
    // mySerial.println(clockId);
    
    if (!( (clockId == 0) || (clockId == 1) || (clockId == 2) )) {
    mySerial.println("Unknown clock id");
    return;    
    }

    if (strlen(data) < 4) { //c0sf + frequency on 4 digits minimum
      mySerial.println("Ill-formated command");
      return;
    }

    strncpy(buff, data+2, 2);
        
    // **************** Set frequency
    if (strcmp(buff,"sf") == 0) {

      // mySerial.println("sf ");
      
      strncpy(buff, data+4, strlen(data) -4);
      value = atoi(buff);

      if (value < SI5351_CLKOUT_MIN_FREQ) {
        mySerial.println("Frequency too low");
        return;
      }

      if (value > SI5351_CLKOUT_MAX_FREQ) {
        mySerial.println("Frequency too high");
        return;
      }
      
      ClockGenerator.setFrequency(clockId, value);
      ClockGenerator.displayAllClocks(); // pb sans doute de editMode
      printFreq(clockId);
    
    // **************** Get frequency
    } else if (strcmp(buff,"gf") == 0) {
      printFreq(clockId);

    // **************** Get phase step
    } else if (strcmp(buff,"gt") == 0) {
      mySerial.print("Clock ");
      mySerial.print(clockId);
      mySerial.print(" phase Step: ");
      mySerial.print(ClockGenerator.getPhaseStep(clockId));
      mySerial.println(" Hz"); 
 
    // **************** Set phase indice
    } else if (strcmp(buff,"sp") == 0) {

      strncpy(buff, data+4, strlen(data) -4);
      value = atoi(buff);

      if (value > 127) {
        mySerial.println("Phase indice must be less than 128");
      }
      ClockGenerator.setPhase(clockId, value); 
      ClockGenerator.displayAllClocks();
      printPhase(clockId);
                   
    // **************** Get phase
    } else if (strcmp(buff,"gp") == 0) {
       printPhase(clockId);
    
    // **************** Set drive 
    } else if (strcmp(buff,"sd") == 0) {
      
      strncpy(buff, data+4, strlen(data) -4);
      value = atoi(buff);

      if (! ((value < 4) && (value >= 0)) ) {
        mySerial.println("Drive indice must be 0 (2 mA), 1 (4 mA), 2 (6 mA) or 3 (8 mA)");
        return;
      }
      ClockGenerator.setDrive(clockId, value);
      ClockGenerator.displayAllClocks();
      printDrive(clockId); 
      
    } else if (strcmp(buff,"gd") == 0) {
      printDrive(clockId);  

    // **************** Set sweep params
    } else if (strcmp(buff,"ss") == 0) {
      
      strncpy(buff, data+4, strlen(data)-4); 

      char* ptr = strtok(buff, delimiter);
      int startFreq = atoi(ptr);
      int stopFreq = atoi(strtok(NULL, delimiter));

      bool active = true; // (boolean)activeInt;
      if (strcmp(strtok(NULL, delimiter),"0") == 0) active = false; 
      
      ClockGenerator.setSweepParamCx(clockId, startFreq, stopFreq, active);
      printCxSweep(clockId);
      return;     
      
    // **************** Get sweep params
    } else if (strcmp(buff,"gs") == 0) {
      printCxSweep(clockId);
      return;  
    }  
  
  // Parameters not clock specific   
  } else { 
    strncpy(buff, data, 2);    

    // **************** Get phase tied clocks 
    if (strcmp(buff,"gp") == 0) {      
      printPhaseTied();
      return;
    }
        
    // **************** Set phase tied clocks
    if (strcmp(buff,"sp") == 0) {
      strncpy(buff, data+2, 1);
      value = atoi(buff);      
      ClockGenerator.setPhaseTiedClocks((byte)value); 
      ClockGenerator.displayAllClocks();
      printPhaseTied();   
      return; 
    }

    // **************** Set global sweep params
    strncpy(buff, data, 8); 
    buff[8] = '\0';       // !!! Pourquoi fonctionne dans les autres cas alors que nécessaire a priori d'ajouter \0
    
    if (strcmp(buff,"sweepset") == 0) {
      
      strncpy(buff, data+8, strlen(data)-8);    
//      mySerial.println(buff);
      
      // initialize first part (string, delimiter)
      char* ptr = strtok(buff, delimiter);
      int nbStep = atoi(ptr);
      float period = atof(strtok(NULL, delimiter));  
//      mySerial.println(period, DEC);
      
      ClockGenerator.setNbSweep(nbStep);
      ClockGenerator.setSweepPeriod(period);
      ClockGenerator.displayAllClocks();
      printSweep();
      return;        
    }

    // **************** Get global sweep params
    strncpy(buff, data, 8);    
    if (strcmp(buff,"sweepget") == 0) {
      printSweep();
      return; 
    }

    // **************** Sweep start 
    strncpy(buff, data, 10);    
    if (strcmp(buff,"sweepstart") == 0) {
      if(ClockGenerator.initSweep()) {
        mySerial.println("Sweep should start");      
      } else {
        mySerial.println("There isn't any clock with sweep activated");              
      }
      return;
    }
    
    // **************** Sweep stop 
    strncpy(buff, data, 9);    
    if (strcmp(buff,"sweepstop") == 0) {
      ClockGenerator.stopSweep();
      mySerial.println("Sweep should stop");        
      return;
    }

    strncpy(buff, data, 4);
    buff[4] = '\0';      
    // **************** Set calibration
    if (strcmp(buff,"scal") == 0) {
      strncpy(buff, data+4, strlen(data)-4); 
      float cal = atof(buff);            
      ClockGenerator.setCalibration(cal); 
      printCalibration();   
      return; 
    }

    // **************** Get calibration
    if (strcmp(buff,"gcal") == 0) {
      printCalibration();   
      return; 
    }
    
    mySerial.println("Unknown command");
    if (mySerial==Serial)  info(); 
  }

}


//****************************
void processIncomingByte (const byte inByte)
{

  // Blank screen when expecting a new commande
  if (input_pos == 0) {  
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(2,50);
    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextWrap(true);
  }

  // Prints each byte received on the TFT
  tft.print(char(inByte));
  delay(100);

  if (DEBUG) {
    Serial.print("Byte: ");
    Serial.write(inByte);
    Serial.write("\n");
  }
    
  switch (inByte) {

    case '\n':   // end of text
      input_line [input_pos] = 0;  // terminating null byte

      delay(1000);
      ClockGenerator.displayAllClocks();
      
      // terminator reached! process input_line here ...
      analyzeCommand(input_line);

      // reset buffer for next time
      input_pos = 0;  
      memset(input_line, 0, sizeof(input_line));      
      break;

    case '\r':   // discard carriage return
      break;

    default:
      // keep adding if not full ... allow for terminating null byte
      if (input_pos < (MAX_INPUT - 1))
        input_line [input_pos++] = inByte;
      break;
      
  }  // end of switch

} // end of processIncomingByte  


//*****************************************************************************
void setup()
{
  mySerial.begin(9600);
  while (!mySerial) {}
    
  ClockGenerator.begin(DEBUG, 9600);
  if (mySerial==Serial)  info();  
}

//*****************************************************************************
void loop()
{
  // if mySerial data available, process it
  while (mySerial.available () > 0) { processIncomingByte (mySerial.read ());}
  ClockGenerator.testSweep();     // If sweep is launched, changes the frequency after the programmed period of time 
  ClockGenerator.run();           // allows human interaction
}  
