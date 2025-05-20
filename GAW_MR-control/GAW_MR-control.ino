/* ------------------------------------------------------------------------- *
 * Name   : GAW-MR-control
 * Author : Gerard Wassink
 * Date   : May 2025
 * Purpose: Control model railway through switch panel via a Loconet connection
 * Made use of: https://www.tindie.com/products/tanner87661/loconet-interface-breakout-board-with-grove-port/
 * Versions:
 *   0.1  : Initial code base
 *   0.2  : Added 20x4 LCD display
 *   0.3  : Intermediate code cleanup
 *   0.4  : Built in save and recall for status table
 *          Added text to the README
 *   0.5  : Code cleanup & little corrections
 *          expanded matrix to 8 x 8
 *          improved initialization
 *          Multiplexers inbouwen voor LEDS
 *   0.6    Fighting with the multiplexer code
 *          Added and improved comments
 *   0.7    Added test for speed-step control using variable resistor
 *          Added some verbosity
 *   0.8    Second state added to element array for locomotive direction and speed
 *          Inserted 7 switch elements to make the number 32
 *
 *------------------------------------------------------------------------- */
#define progVersion "0.8"                   // Program version definition
/* ------------------------------------------------------------------------- *
 *             GNU LICENSE CONDITIONS
 * ------------------------------------------------------------------------- *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ------------------------------------------------------------------------- *
 *       Copyright (C) May 2025 Gerard Wassink
 * ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- *
 *                                                        DEBUGGING ON / OFF
 * Compiler directives to switch debugging on / off
 * Do not enable debug when not needed, Serial takes space and time
 * DEBUG_LVL:
 *   0 - no debug output
 *   1 - Elementary debug output
 *   2 - verbose debug output
 * ------------------------------------------------------------------------- */
#define DEBUG_LVL 1

#if DEBUG_LVL > 0
#define debugstart(x) Serial.begin(x)
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debugstart(x)
#define debug(x)
#define debugln(x)
#endif

/* ------------------------------------------------------------------------- *
 *                                             Include headers for libraries
 * ------------------------------------------------------------------------- */
#include <EEPROM.h>                         // EEPROM library
#include <Keypad.h>                         // Keypad library
#include <LocoNet.h>                        // LocoNet library
#include <Wire.h>                           // I2C comms library
#include <LiquidCrystal_I2C.h>              // LCD library
#include <Adafruit_MCP23X17.h>              // I/O expander library

/* ------------------------------------------------------------------------- *
 *                                                               Definitions
 * ------------------------------------------------------------------------- */
#define ROWS 8                              // Definitions for
#define COLS 8                              //  key-matrix

#define POWEROFF 0
#define POWERON  1

#define POWERLED 53                         // Panel Power indicator

#define STRAIGHT 0                          // State definitions
#define THROWN 1                            //  for Switch states

#define FORWARD 1                          // State definitions
#define STOP    0                          //   for loc
#define REVERSE -1                         //     direction

#define entrySize sizeof(MR_data)           // Dynamic definitions for
#define nElements sizeof(element) / \
                  entrySize                 //  element size

#define numberOfMx sizeof(mcps) / \
                  sizeof(MCPINFO)           // Number of expander interfaces

#define memSize EEPROM.length()             // Amount of EEPROM memory

/* ------------------------------------------------------------------------- *
 *                                       Global variables needed for Loconet
 * ------------------------------------------------------------------------- */
lnMsg *LnPacket;
int SwitchDirection;

/* ------------------------------------------------------------------------- *
 *                                 Global variables needed for Control Panel
 * ------------------------------------------------------------------------- */
int activeLoc = 0;

/* ------------------------------------------------------------------------- *
 *                                                         Element structure
 * The MR_date structure defines the variables per element.
 * ------------------------------------------------------------------------- */
struct MR_data{                             // single element definition
  int type;
  int module;
  int address;
  int state;
  int state2;
};

/* ------------------------------------------------------------------------- *
 *                                                             Element array
 * The element[] array holds values for elements on the control panel.
 * At thispoint these are Switches, Locomotives, Functions and Power. 
 * ------------------------------------------------------------------------- */
struct MR_data element[] = {

// Types: 0 = Switch, 1 = Locomotive, 90 -= function, 99 = power

// ===== CAVEAT =====
// Switches MUST come first in this array, as calculations for the 
// LED multiplexers are based on the index of the switches in the 
// element array
// ===== CAVEAT =====

/* ------------------------------------------------------------------------- *
 * Type = 0, Switches:
 *   module  = layout module, administrative only for now
 *   address = DCC address of the switch
 *   state   = actual state of the switch
 *   state2  = opposite state, used for 2nd LED
 * ------------------------------------------------------------------------- */

//              Layout module 1
   0, 1, 101, STRAIGHT, THROWN,
   0, 1, 102, STRAIGHT, THROWN,
   0, 1, 103, STRAIGHT, THROWN,
   0, 1, 104, STRAIGHT, THROWN,

//              Layout module 2
   0, 2, 201, STRAIGHT, THROWN,
   0, 2, 202, STRAIGHT, THROWN,
   0, 2, 203, STRAIGHT, THROWN,

//              Layout module 4
   0, 4, 401, STRAIGHT, THROWN,
   0, 4, 402, STRAIGHT, THROWN,
   0, 4, 403, STRAIGHT, THROWN,
   0, 4, 404, STRAIGHT, THROWN,
   0, 4, 405, STRAIGHT, THROWN,
   0, 4, 406, STRAIGHT, THROWN,
   0, 4, 407, STRAIGHT, THROWN,

//              MLayout mdule 5
   0, 5, 501, STRAIGHT, THROWN,
   0, 5, 502, STRAIGHT, THROWN,

//              Layout module 6
   0, 6, 601, STRAIGHT, THROWN,
   0, 6, 602, STRAIGHT, THROWN,
   0, 6, 603, STRAIGHT, THROWN,

//              Layout module 7
   0, 7, 701, STRAIGHT, THROWN,

//              Layout module 8
   0, 8, 801, STRAIGHT, THROWN,
   0, 8, 802, STRAIGHT, THROWN,
   0, 8, 803, STRAIGHT, THROWN,
   0, 8, 804, STRAIGHT, THROWN,
   0, 8, 805, STRAIGHT, THROWN,

/* ------------------------------------------------------------------------- *
 * 7 spare switch positions, for possible future expansion
 * ------------------------------------------------------------------------- */
   0, 0,   0, 0, 0,
   0, 0,   0, 0, 0,
   0, 0,   0, 0, 0,
   0, 0,   0, 0, 0,
   0, 0,   0, 0, 0,
   0, 0,   0, 0, 0,
   0, 0,   0, 0, 0,

/* ------------------------------------------------------------------------- *
 * Type = 1, Locomotives:
 *   module  = arbitrary, not used
 *   address = DCC address of the locomotive
 *   state   = -1 = reverse, 0 = stopped, 1 = forward
 *   state2  = speed step
 * ------------------------------------------------------------------------- */

//              My locomotives
   1, 0, 344, 1, 0,                         // Hondekop
   1, 0, 386, 1, 0,                         // BR 201 386
   1, 0, 611, 1, 0,                         // NS 611
   1, 0, 612, 1, 0,                         // NS 612
   1, 0,2412, 1, 0,                         // NS 2412

/* ------------------------------------------------------------------------- *
 * Type = 90, Funtions:
 *   module  = arbitrary, not used
 *   address = Function number
 *   state   = not used
 *   state2  = not used
 * ------------------------------------------------------------------------- */

//              General Functions
  90, 0,9001, 0, 0,                         // Store state
  90, 0,9002, 0, 0,                         // Recall state
  90, 0,9003, 0, 0,                         // Show elements

//              Loc Functions
  90, 0,9101, 0, 0,                          // Forward
  90, 0,9102, 0, 0,                          // Stop
  90, 0,9103, 0, 0,                          // Reverse
  90, 0,9104, 0, 0,                          // Lights
  90, 0,9105, 0, 0,                          // Sound
  90, 0,9106, 0, 0,                          // Whistle
  90, 0,9107, 0, 0,                          // Horn
  90, 0,9108, 0, 0,                          // Two-tone Horn

/* ------------------------------------------------------------------------- *
 * Type = 99, Power:
 *   module  = arbitrary, not used
 *   address = Function number
 *   state   = power = 1 (on) / 0 (off)
 *   state2  = not used
 * ------------------------------------------------------------------------- */

//              POWER
  99, 0,9999, POWEROFF, 0,                  // Roco Z21

};


/* ------------------------------------------------------------------------ *
 *       Define controlPanel variables
 *         this is the control panel for the model railroad layout
 *         The buttons are handled in a 8 x 8 grid
 * ------------------------------------------------------------------------ */
char keys[ROWS][COLS] = {
  { 1,  2,  3,  4,  5,  6,  7,  8},         // Pointers into the element 
  { 9, 10, 11, 12, 13, 14, 15, 16},         //   array for each button
  {17, 18, 19, 20, 21, 22, 23, 24},
  {25, 26, 27, 28, 29, 30, 31, 32},
  {33, 34, 35, 36, 37, 38, 39, 40},
  {41, 42, 43, 44, 45, 46, 47, 48},
  {49, 50, 51, 52, 53, 54, 55, 56},
  {57, 58, 59, 60, 61, 62, 63, 64}
};

byte rowPins[ROWS] = {22, 23, 24, 25, 26, 27, 28, 29}; // row pins, key matrix
byte colPins[COLS] = {30, 31, 32, 33, 34, 35, 36, 37}; // column pins, key matrix


/* ------------------------------------------------------------------------- *
 *       Create objects for controlPanel
 * ------------------------------------------------------------------------- */
Keypad controlPanel = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);


/* ------------------------------------------------------------------------- *
 *       Create objects with addres for the LCD screen
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C display(0x27,20,4);       // Initialize display


/* ------------------------------------------------------------------------- *
 *              Create objects with addres(ses) for the multiplexer MCP23017
 *
 * For the Switches, MCP23017's are used in pairs for operating the LED's 
 * indicating the switch positions. The first ones (even address) for the
 * THROWN position LED's, the second ones (odd address) for the STRAIGHT
 * position LED's. One pair of MCP23017's serves 16 switches.
 *
 * On my layout there are 25 switches. The first four MCP23017's will be
 * used to operate their LED's, so there will be room for expansion op to
 * 32 switches.
 *
 * For the more simple on/off scenario's, as in the main Power LED, the
 * selection of loc's and functions, individual ports of the MCP23017's 
 * are used.
 *
 * The multiplexer MCP23017's are addressed from 0x20 to max 0x27.
 * Their definitions are stored in the mcps[] array, see below.
 * ------------------------------------------------------------------------- */
struct MCPINFO {
  Adafruit_MCP23X17 mcp;
  uint8_t address;  
};

MCPINFO mcps[] {
  {Adafruit_MCP23X17(), 0x20},              // multiplexer 0
  {Adafruit_MCP23X17(), 0x21},              // multiplexer 1
  {Adafruit_MCP23X17(), 0x22},              // multiplexer 2
  {Adafruit_MCP23X17(), 0x23},              // multiplexer 3
  {Adafruit_MCP23X17(), 0x24},              // multiplexer 4
  {Adafruit_MCP23X17(), 0x25},              // multiplexer 5
  {Adafruit_MCP23X17(), 0x26},              // multiplexer 6
//  {Adafruit_MCP23X17(), 0x27},              // multiplexer 7 (is also the address of the LCD display)
};

/* ------------------------------------------------------------------------- *
 *                                                   Initial routine setup()
 * ------------------------------------------------------------------------- */
void setup() {

  pinMode(POWERLED, OUTPUT);                // Power indicator LED

  debugstart(115200);                       // Start serial

  display.init();                           // Initialize LCD display
  display.backlight();                      // Backlights on by default

  doInitialScreen(1);                       // Show for x seconds

  debugln(F("==============================="));
  debug("GAW-MR-Control v");
  debugln(progVersion);

  debugln(F("==============================="));
  debugln(F("Start initialization"));
  debugln(F("==============================="));
  debug("MemSize   = "); debugln(memSize);
  debug("entrySize = "); debugln(entrySize);
  debug("tableSize = "); debugln(sizeof(element));
  debug("nElements = "); debugln(nElements);

  debugln(F("==============================="));
  debugln(F("Initializing multiplexers:"));

  for (int mx=0; mx<numberOfMx; mx++) {
    debug(F(" #")); debug(mx);
    mcps[mx].mcp.begin_I2C(mcps[mx].address);
    for (int j = 0; j < 16; j++) {
      mcps[mx].mcp.pinMode(j, OUTPUT);
    }
  }
  debugln(); 

  debugln(F("==============================="));
  debugln(F("Initialize LocoNet"));
  LocoNet.init();                           // Initialize Loconet
  debugln(F("==============================="));

//  storeState();                             // to reaplce it with the definitions in the code

  LCD_display(display, 1, 0, F("                    "));
  recallState();                            // By default recall state from EEPROM
  activateState();                          //   and make state as it was!

  LCD_display(display, 0, 0, F("System ready        "));
  
  debugln(F("==============================="));
  debugln(F("Setup done, ready for operations"));
  debugln(F("==============================="));

/* =========================================
 * Test speed control with potentiometer 
 * =========================================
  int value, step, dir;
  while(true) {
    int value = analogRead(A1);
    debug(" "+String(value));
    step = int( (long)value / 18.29);

    debug(", intermediate ="+String(step));

    if (step > 28) {
      step = 28 - (55 - step); 
      dir = 1;
    } else {
      step = 28 - step;
      dir = -1;
    }
    debug (" dir = "+String(dir));
    debugln(" Speedstep "+String(step));
    delay(500);
  }

  exit(0);
 */
}



/* ------------------------------------------------------------------------- *
 *                                                     Repeating code loop()
 * ------------------------------------------------------------------------- */
void loop() {

  char key = controlPanel.getKey();
  if(key) {                                 // Check for a valid key
    debugln("Received key value "+String((int)key));
    handleKeys(key);                        //   and handle key
  }


/* ------------------------------------------------------------------------- *
 *                                         process incoming Loconet messages
 * ------------------------------------------------------------------------- */
  LnPacket = LocoNet.receive();

  if (LnPacket) {
    LocoNet.processSwitchSensorMessage(LnPacket);
  }

/*
  // Code from example loconet sketch

  // button pressed
  if(digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {

    if(SwitchDirection == 0) {
      SwitchDirection = 1;
    } else {
      SwitchDirection = 0;
    }

    LocoNet.requestSwitch(Switch_ADDRESS, 0, SwitchDirection);
    buttonPressed = true;

  } else {  // button released

    if(digitalRead(BUTTON_PIN) == HIGH) {
      buttonPressed = false;
    } else {
      if(digitalRead(BUTTON_PIN) == HIGH) buttonPressed = false;
    }

  }
*/

}



/* ------------------------------------------------------------------------- *
 *                                                              handleKeys()
 *       Routine to handle buttons on the control panel
 * ------------------------------------------------------------------------- */
void handleKeys(char key) {

  int index = key - 1;                      // Convert keycode to table index

  switch(element[index].type) {             // Which type do we have?

    case 0:                                 // SWITCH TYPE
      flipSwitch(index);
      break;

    case 1:                                 // LOCOMOTIVE TYPE
      handleLocomotive(index);
      break;

    case 90:                                // FUNCTION TYPE
      handleFunction(index);
      break;
 
    case 99:                                // POWER TYPE
      handlePower(index);
      break;

    default: 
      break;

  }

}

  

/* ------------------------------------------------------------------------- *
 *                                                              flipSwitch()
 * ------------------------------------------------------------------------- */
void flipSwitch(int index) {
  element[index].state  = !element[index].state;   // Flip state
  element[index].state2 = !element[index].state;   // Flip state2
  setSwitch(index);
}



/* ------------------------------------------------------------------------- *
 *                                                               setSwitch()
 * ------------------------------------------------------------------------- */
void setSwitch(int index) {

  int state  = ( element[index].state  == 0 ? STRAIGHT : THROWN );
  int state2 = ( element[index].state2 == 0 ? STRAIGHT : THROWN );

                                            // Calculate mx address and port 
  int mx = (index / 16) * 2;                //  for the even numbered mux
  int port = (index % 16);                  //  from switch position in elements

  debug("Set Switch "+String(element[index].address)+" to "+ ( state == 0 ? "Straight" : "Thrown  ") );
  debug(" - mx "+String(mx)+","+String(port)+" = "+state);

  mcps[mx].mcp.digitalWrite(port, state);   // Set first LED on or off
  mx++;                                     // One up for odd number mux
  mcps[mx].mcp.digitalWrite(port, state2);  // Set second LED on or off

  debugln(", mx "+String(mx)+","+String(port)+" = "+state2);

  LCD_display(display, 0, 0, F("Switch              "));
  LCD_display(display, 0, 7, String(element[index].address));
  LCD_display(display, 0,12, element[index].state == 0 ? "Straight" : "Thrown  ");


// Future expansion
// SET LOCONET COMMAND TO Z21
//   TO SET SWITCH

}



/* ------------------------------------------------------------------------- *
 *                                                        handleLocomotive()
 * ------------------------------------------------------------------------- */
void handleLocomotive(int index) {
  debug("Loc # ");                                // Just display address
  debug(element[index].address);                //   for future use
  activeLoc = index;
  LCD_display(display, 1, 0, "Loc "+String(element[activeLoc].address)+"            ");

  setLocSpeed(index);                             //   for future use
}

  

/* ------------------------------------------------------------------------- *
 *                                                             setLocSpeed()
 * ------------------------------------------------------------------------- */
void setLocSpeed(int index) {
  int direction = element[activeLoc].state;
  int speedstep = element[activeLoc].state2;

  debug(" set to " + direction == FORWARD ? " forward" : " reverse" );
  debug(", speed: " + String(speedstep) );
  debugln();

// SET LOCONET COMMAND TO Z21
//   TO SET SWITCH
}

  

/* ------------------------------------------------------------------------- *
 *                                                          handleFunction()
 * ------------------------------------------------------------------------- */
void handleFunction(int index) {
  int function = element[index].address;

  switch(function) {

    case 9001:                              // Save status
      storeState();
      break;
  
    case 9002:                              // Recall status
      recallState();
      break;

#if DEBUG_LVL > 0
    case 9003:                              // Show elements
      showElements();
      break;
#endif

    case 9101:                              // Loc Forward
      locForward();
      break;

    case 9102:                              // Loc Forward
      locStop();
      break;

    case 9103:                              // Loc Forward
      locReverse();
      break;

    default:
      break;

  }

}



/* ------------------------------------------------------------------------- *
 *                                                              locForward()
 * ------------------------------------------------------------------------- */
void locForward() {
  if (activeLoc > 0) {
    element[activeLoc].state = FORWARD;
    debugln("Loc #"+String(element[activeLoc].address)+" set to forward");
    LCD_display(display, 1, 10, "forward   ");
  } else {
    LCD_display(display, 1, 0, F("NO ACTIVE LOC!      "));
    debugln(F("NO ACTIVE LOC!"));
  }
}



/* ------------------------------------------------------------------------- *
 *                                                              locForward()
 * ------------------------------------------------------------------------- */
void locStop() {
  if (activeLoc > 0) {
    element[activeLoc].state = STOP;
    debugln("Loc #"+String(element[activeLoc].address)+" set to stop");
    LCD_display(display, 1, 10, "stop      ");
  } else {
    LCD_display(display, 1, 0, F("NO ACTIVE LOC!      "));
    debugln("NO ACTIVE LOC!");
  }
}



/* ------------------------------------------------------------------------- *
 *                                                              locForward()
 * ------------------------------------------------------------------------- */
void locReverse() {
  if (activeLoc > 0) {
    element[activeLoc].state = REVERSE;
    debugln("Loc #"+String(element[activeLoc].address)+" set to reverse");
    LCD_display(display, 1, 10, "reverse   ");
  } else {
    LCD_display(display, 1, 0, F("NO ACTIVE LOC!      "));
    debugln("NO ACTIVE LOC!");
  }
}



/* ------------------------------------------------------------------------- *
 *                                                             handlePower()
 * ------------------------------------------------------------------------- */
void handlePower(int index) {
  element[index].state = !element[index].state;   // Flip state
  setPower(element[index].state);           // Set power on of off
}



/* ------------------------------------------------------------------------- *
 *                                                                setPower()
 * ------------------------------------------------------------------------- */
void setPower(int state) {
  debug("Setting Power ");
  debugln(state == 0 ? F("OFF") : F("ON") );
  state ? digitalWrite(POWERLED, HIGH) : digitalWrite(POWERLED, LOW);

  LCD_display(display, 3,10, "Power: ");
  LCD_display(display, 3,17, state ? "ON " : "OFF");

// Future expansion
// SET LOCONET COMMAND TO Z21
//   TO SET POWER STATE
}


/* ------------------------------------------------------------------------- *
 *                                                            showElements()
 * Testing purposes: show array of elements and their states
 * ------------------------------------------------------------------------- */
#if DEBUG_LVL > 0
void showElements() {
  debugln(F("Show elements table:"));
  for (int i=0; i<nElements; i++) {
    debug(String(i+1));
    debug(F(" - Type: "));
    debug(element[i].type);
    switch (element[i].type) {
      case 0:
        debug(F(" - Switch: "));
        break;

      case 1:
        debug(F(" - Locomotive: "));
        break;
        
      case 90:
        debug(F(" - Funtion: "));
        break;
        
      case 99:
        debug(F(" - Power: "));
        break;

      default:
        break;
    }
    
    debug(String(element[i].address));
    debug(F(" - "));

    switch (element[i].type) {
      case 0:
        debug(element[i].state  == 0 ? F("Straight, ") : F("Thrown, " ) );
        debug(element[i].state2 == 0 ? F("Straight") : F("Thrown" ) );
        debug(F(" - Module: "));
        debugln(element[i].module);
        break;

      case 1:
        if (element[i].state == -1) {
          debug("Reverse, ");
        } else if (element[i].state == 0) {
          debug("Stop, ");
        } else if (element[i].state == 1) {
          debug("Forward, ");
        }

        debugln("Speed: "+String(element[i].state2));

        break;

      case 90:
        showFunctions(i);
        break;
      
      case 99:
        debugln(element[i].state == 0 ? "OFF" : "ON" );
        break;

      default:
        break;

    }
  }
}
#endif


/* ------------------------------------------------------------------------- *
 *                                                           showFunctions()
 * ------------------------------------------------------------------------- */
void showFunctions(int index) {
  switch (element[index].address) {
    case 9001: debugln("Store state"); break;
    case 9002: debugln("Recall state"); break;
    case 9003: debugln("Show Elements"); break;
    case 9101: debugln("Loc Forward"); break;
    case 9102: debugln("Loc Stop"); break;
    case 9103: debugln("Loc Reverse"); break;
    case 9104: debugln("Loc Lights"); break;
    case 9105: debugln("Loc Sound"); break;
    case 9106: debugln("Loc Whistle"); break;
    case 9107: debugln("Loc Horn"); break;
    case 9108: debugln("Loc Two-tone Horn"); break;
    default: break;
  }
}


/* ------------------------------------------------------------------------- *
 *                                                              storeState()
 * ------------------------------------------------------------------------- */
void storeState() {
  debugln("Storing system status");
  for (int i=0; i<nElements; i++) {
    EEPROM.put(i*entrySize, element[i]);
  }
  LCD_display(display, 3, 0, "Stored");
  delay(1000);
  LCD_display(display, 3, 0, F("      "));
}



/* ------------------------------------------------------------------------- *
 *                                                             recallState()
 * ------------------------------------------------------------------------- */
void recallState() {
  debugln("Recalling systemn status");
  for (int i=0; i<nElements; i++) {
    EEPROM.get(i*entrySize, element[i]);
  }
  LCD_display(display, 3, 0, "Recalled");
  delay(1000);
  LCD_display(display, 3, 0, F("        "));
}



/* ------------------------------------------------------------------------- *
 *                                                           activateState()
 * ------------------------------------------------------------------------- */
void activateState() {
  debugln("Activating system status to layout");
  int pwr = 0;                              // Assume power off
  int index = 0;

  for (index = 0; index < nElements; index++) {         // FIRST: restore power state
    if (element[index].type == 99) {            // Power state found
        pwr = element[index].state;             // What was the state?
        setPower(element[index].state);         // Set power on / off
    }
  }

  if (pwr) {                                // Power on? then Switchs
    for (index = 0; index < nElements; index++) {
      if (element[index].type == 0) {       // Is it a turnout?
          setSwitch(index);                 //  then set proper value
      }
    }
  }

}



/* ------------------------------------------------------------------------- *
 *       Show initial screen, then paste template          doInitialScreen()
 * ------------------------------------------------------------------------- */
void doInitialScreen(int s) {
  
  LCD_display(display, 0, 0, F("GAW-MR-control v    "));
  LCD_display(display, 0, 16, progVersion);
  LCD_display(display, 1, 0, F("(c) Gerard Wassink  "));
  LCD_display(display, 2, 0, F("GNU public license  "));

  delay(s * 1000);

  LCD_display(display, 0, 0, F("                    "));
  LCD_display(display, 1, 0, F("                    "));
  LCD_display(display, 2, 0, F("                    "));

}

  

/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
    screen.setCursor(col, row);
    screen.print(text);
}
