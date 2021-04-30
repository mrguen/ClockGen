/*********************************************************************


*********************************************************************/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define pauseDelay 2000
#define OLED_RESET 4

#define mySerial Serial     // If you want to user the first serial port. In this case disconnect the USB after programming to avoid interference with the USB
//#define mySerial Serial1  // If you want to user the second serial port (on the 1284 Narrow board for example Rx1 = 10, Tx1 = 11). 

static const unsigned int MAX_INPUT = 50;
static char input_line [MAX_INPUT];
static unsigned int input_pos = 0;

boolean processed = false;

Adafruit_SSD1306 display(OLED_RESET);

//****************************
void processIncomingByte (const byte inByte)
{
  switch (inByte) {

    case '\n':   // end of text
      input_line [input_pos] = 0;  // terminating null byte

      // terminator reached! process input_line here ...
      delay(pauseDelay);
      display.clearDisplay();
      display.setCursor(0,0);
      display.println(input_line);
      display.display();
      delay(pauseDelay);

      // reset buffer for next time
      input_pos = 0;  
      memset(input_line, 0, sizeof(input_line));     
      processed = true; 
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

void setup()   {                
  mySerial.begin(9600);
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C 
  // init done
  
  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);

  display.println("Sending");
  display.println("commands");
  display.println("to the");
  display.println("ClockGen");
  display.display();
  delay(pauseDelay);
  delay(pauseDelay);
  delay(pauseDelay);
  delay(pauseDelay);
  delay(pauseDelay);
    
   // Clearing the mySerial buffer because if the ClockGenerator sketch sends some data that are not needed in this application
  while (mySerial.available () > 0) { mySerial.read (); delay(10);}
  mySerial.flush();
  delay(10);
 
  // Sending: c0 @ 1 MHz =================================================================================================================

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("c0 sf 1000000");
  display.display();

  mySerial.print("c0 sf 1000000\n"); // Need to add the \n that is recognized by the receiving software as end of data

  processed = false;
  while (!processed) {
    while (mySerial.available () > 0) { processIncomingByte (mySerial.read ());}
  }
  delay(pauseDelay);
    

  // Sending: sweepset 100 steps 0.1s  ====================================================================================================
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("sweepset 100;0.1");
  display.display();

  mySerial.print("sweepset 100;0.1\n");

  processed = false;
  while (!processed) {
    while (mySerial.available () > 0) { processIncomingByte (mySerial.read ());}
  }
  delay(pauseDelay);

  // Sending: c0 sweeps from 10 MHz to 20 MHz ===============================================================================================

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("c0 ss 10000000;20000000;1");
  display.display();

  mySerial.print("c0 ss 10000000;20000000;1\n");

  processed = false;
  while (!processed) {
    while (mySerial.available () > 0) { processIncomingByte (mySerial.read ());}
  }
  delay(pauseDelay);

  // Sending: sweepstart  ===================================================================================================================

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("sweepstart");
  display.display();

  mySerial.print("sweepstart\n");

  processed = false;
  while (!processed) {
    while (mySerial.available () > 0) { processIncomingByte (mySerial.read ());}
  }
  delay(pauseDelay);

}


void loop() {
  
}
