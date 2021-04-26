/*
 * clockGen_serial_control
 * (c) 2020 by pandauino.com
 *
 * This script shows an example of how to setup the Pandauino Clock generator 
 * using serial commands.
 * 
 * Connect the Clock Generator to your computer with an USB cable
 * It should be recognized as "STMicroelectronics Virtual COM Port (COMxx)"
 * Refer to the user's manual for proper installation / Arduino IDE set-up
 * 
 * Load this sketch to the Clock Generator
 * 
 * Open the Serial Monitor and send commands to set-up each clock parameter or the sweep function etc
 * 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <Pandauino_Clock_Generator.h>

static const unsigned int MAX_INPUT = 50;
static char input_line [MAX_INPUT];
static unsigned int input_pos = 0;
static char delimiter[] = ";";

//****************************************************
void info()
{
  Serial.println("************************************************************************************************************************");  
  Serial.println(" ");
  Serial.println("*** ClockGen serial interface ***");
  Serial.println(" ");  
  Serial.println("cX sf (int): Clock X set frequency (c0 sf 1000000 = Clock 0 frequency set to 1 MHz)");
  Serial.println("cX gf: Clock X get frequency");
  Serial.println("cX gt: Clock X get phase step");
  Serial.println("cX sp (byte): Clock X set phase indice (0 - 127) (c0sp64 = Clock 0 phase indice set to 64)");
  Serial.println("cX gp: Clock X get phase");
  Serial.println("cX sd (byte): Clock X set drive (0-3) (c0sd0 = Clock 0 drive set to 2 mA)");
  Serial.println("cX gd: Clock X get drive");
  Serial.println("cX ss (int);(int);(boolean): set clock X sweep start and stop frequency, and active/inactive (example: c0 ss 01000;2000;1)");
  Serial.println("cX gs: get clock X sweep parameters");
  Serial.println(" ");
  Serial.println("sp (byte): set phase related clocks 0: all clocks, 1: clk0, 2: clko/clk1, 3: clk0/clk2, 4: clk1/clk2");
  Serial.println("gp: get phase related clocks");
  Serial.println("sweepset (int);(float): set sweep number of steps and period");
  Serial.println("sweepget: get sweep number of steps and period");  
  Serial.println("sweepstart");
  Serial.println("sweepstop");
  Serial.println(" ");
  Serial.println("scal (float): set calibration in ppm");
  Serial.println("gcal: get calibration in ppm");
  Serial.println(" ");
  
  Serial.println("************************************************************************************************************************");    
  Serial.println(" ");
  
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
  Serial.print("Clock ");
  Serial.print(clockId);
  Serial.print(" frequency: ");
  Serial.print(ClockGenerator.getFrequency(clockId));
  Serial.println(" Hz");  
}

//****************************************************
void printPhase(int clockId) {
  Serial.print("Clock ");
  Serial.print(clockId);
  Serial.print(" phase: ");
  Serial.print(ClockGenerator.getPhase(clockId), 1);
  Serial.println(" Hz");  
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
  
  Serial.print("Clock ");
  Serial.print(clockId);
  Serial.print(" drive: ");
  Serial.print(driveValueMA);
  Serial.println(" mA");  
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
  
  Serial.print("Phase related clocks: ");
  Serial.println(text);
}

//****************************************************
void printCxSweep(int clockId) {

  long startFreq;
  long stopFreq;
  bool active;
      
  ClockGenerator.getSweepParamCx(clockId, startFreq, stopFreq, active);

  Serial.print("Clock ");
  Serial.print(clockId);
  Serial.print(", start: ");
  Serial.print(startFreq);
  Serial.print(", stop: ");
  Serial.print(stopFreq);
  Serial.print(", active: ");
  Serial.println(active);
    
}

//****************************************************
void printSweep() {
  int nbSweep;
  float periodSweep;
      
  ClockGenerator.getSweepParam(nbSweep, periodSweep);
  Serial.print("Sweep steps: ");
  Serial.print(nbSweep);
  Serial.print(", sweep period: ");
  Serial.print(periodSweep);
  Serial.println(" s");
      
}

//****************************************************
void printCalibration() {
  float cal;
  cal = ClockGenerator.getCalibration();
  Serial.print("Calibration: ");
  Serial.print(cal,2);      
  Serial.println(" ppm");
}

//****************************************************
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
  
  //Serial.println(data);
  //Serial.println(firstLetter);
  //return;

  // Single Clock settings
  if ((firstLetter == 'c'))
  {
    strncpy(buff, data+1, 1);
    clockId = atoi(buff);
  
    // Serial.println(clockId);
    
    if (!( (clockId == 0) || (clockId == 1) || (clockId == 2) )) {
    Serial.println("Unknown clock id");
    return;    
    }

    if (strlen(data) < 4) { //c0sf + frequency on 4 digits minimum
      Serial.println("Ill-formated command");
      return;
    }

    strncpy(buff, data+2, 2);
        
    // **************** Set frequency
    if (strcmp(buff,"sf") == 0) {

      // Serial.println("sf ");
      
      strncpy(buff, data+4, strlen(data) -4);
      value = atoi(buff);

      if (value < SI5351_CLKOUT_MIN_FREQ) {
        Serial.println("Frequency too low");
        return;
      }

      if (value > SI5351_CLKOUT_MAX_FREQ) {
        Serial.println("Frequency too high");
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
      Serial.print("Clock ");
      Serial.print(clockId);
      Serial.print(" phase Step: ");
      Serial.print(ClockGenerator.getPhaseStep(clockId));
      Serial.println(" Hz"); 
 
    // **************** Set phase indice
    } else if (strcmp(buff,"sp") == 0) {

      strncpy(buff, data+4, strlen(data) -4);
      value = atoi(buff);

      if (value > 127) {
        Serial.println("Phase indice must be less than 128");
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
        Serial.println("Drive indice must be 0 (2 mA), 1 (4 mA), 2 (6 mA) or 3 (8 mA)");
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
//      Serial.println(buff);
      
      // initialize first part (string, delimiter)
      char* ptr = strtok(buff, delimiter);
      int nbStep = atoi(ptr);
      float period = atof(strtok(NULL, delimiter));  
//      Serial.println(period, DEC);
      
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
        Serial.println("Sweep should start");      
      } else {
        Serial.println("There isn't any clock with sweep activated");              
      }
      return;
    }
    
    // **************** Sweep stop 
    strncpy(buff, data, 9);    
    if (strcmp(buff,"sweepstop") == 0) {
      ClockGenerator.stopSweep();
      Serial.println("Sweep should stop");        
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
    
    Serial.println("Unknown command");
    info();
  }

}


//****************************
void processIncomingByte (const byte inByte)
{
  switch (inByte) {

    case '\n':   // end of text
      input_line [input_pos] = 0;  // terminating null byte

      // terminator reached! process input_line here ...
      analyzeCommand(input_line);

      // reset buffer for next time
      input_pos = 0;  
      // Ajout TG ca ne fonctionne pas correctement
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
  Serial.begin(115200);
  while (!Serial) {}
  
  ClockGenerator.begin();
  info();  
}

//*****************************************************************************
void loop()
{
  // if serial data available, process it
  while (Serial.available () > 0) { processIncomingByte (Serial.read ());}
  ClockGenerator.testSweep();
}  
