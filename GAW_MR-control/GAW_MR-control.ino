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
 *
 *------------------------------------------------------------------------- */
#define progVersion "0.5"  // Program version definition
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

/* ------------------------------------------------------------------------- *
 *                                                               Definitions
 * ------------------------------------------------------------------------- */
#define ROWS 8
#define COLS 8

#define POWEROFF 0
#define POWERON  1

#define STRAIGHT 0
#define THROWN 1

#define entrySize sizeof(MR_data)
#define nElements sizeof(element) / entrySize

#define memSize EEPROM.length()             // Amount of EEPROM memory

#define POWERLED 53                         // Panel Power indicator

/* ------------------------------------------------------------------------- *
 *                                                          Global variables
 * ------------------------------------------------------------------------- */
lnMsg *LnPacket;
int SwitchDirection;

bool buttonPressed = false;

int activeLoc = 0;

/* ------------------------------------------------------------------------- *
 *                                                         Element structure
 * ------------------------------------------------------------------------- */
struct MR_data{
  int type;
  int module;
  int address;
  int state;
};

struct MR_data element[] = {

// Types: 0 = Switch, 1 = Locomotive, 99 = power

//              Module 1
   0, 1, 101, STRAIGHT,
   0, 1, 102, STRAIGHT,
   0, 1, 103, STRAIGHT,
   0, 1, 104, STRAIGHT,

//              Module 2
   0, 2, 201, STRAIGHT,
   0, 2, 202, STRAIGHT,
   0, 2, 203, STRAIGHT,

//              Module 4
   0, 4, 401, STRAIGHT,
   0, 4, 402, STRAIGHT,
   0, 4, 403, STRAIGHT,
   0, 4, 404, STRAIGHT,
   0, 4, 405, STRAIGHT,
   0, 4, 406, STRAIGHT,
   0, 4, 407, STRAIGHT,

//              Module 5
   0, 5, 501, STRAIGHT,
   0, 5, 502, STRAIGHT,

//              Module 6
   0, 6, 601, STRAIGHT,
   0, 6, 602, STRAIGHT,
   0, 6, 603, STRAIGHT,

//              Module 7
   0, 7, 701, STRAIGHT,

//              Module 8
   0, 8, 801, STRAIGHT,
   0, 8, 802, STRAIGHT,
   0, 8, 803, STRAIGHT,
   0, 8, 804, STRAIGHT,
   0, 8, 805, STRAIGHT,

//              Locomotives
   1, 0, 344, 0,                            // Hondekop
   1, 0, 386, 0,                            // BR 201 386
   1, 0, 611, 0,                            // NS 611
   1, 0, 612, 0,                            // NS 612
   1, 0,2412, 0,                            // NS 2412

//              Functions
  90, 0,9001, 0,                            // Store state
  90, 0,9002, 0,                            // Recall state
  90, 0,9003, 0,                            // Test display states


//              POWER
  99, 0,9999, POWEROFF,                     // Roco Z21
};


/* ------------------------------------------------------------------------ *
 *       Define controlPanel variables
 *         this is the control panel for the model railroad layout
 *         The buttons are handled in a 6 x 6 grid
 * ------------------------------------------------------------------------ */
char keys[ROWS][COLS] = {
  { 1,  2,  3,  4,  5,  6,  7,  8},                 // Pointers into the element 
  { 9, 10, 11, 12, 13, 14, 15, 16},                 //   array for each button
  {17, 18, 19, 20, 21, 22, 23, 24},
  {25, 26, 27, 28, 29, 30, 31, 32},
  {33, 34, 35, 36, 37, 38, 39, 40},
  {41, 42, 43, 44, 45, 46, 47, 48},
  {49, 50, 51, 52, 53, 54, 55, 56},
  {57, 58, 59, 60, 61, 62, 63, 64}
};

byte rowPins[ROWS] = {22, 23, 24, 25, 26, 27, 28, 29}; // row pins of the controlPanel
byte colPins[COLS] = {30, 31, 32, 33, 34, 35, 36, 37}; // column pins of the controlPanel


/* ------------------------------------------------------------------------- *
 *       Create objects for controlPanel
 * ------------------------------------------------------------------------- */
Keypad controlPanel = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);


/* ------------------------------------------------------------------------- *
 *       Create objects with addres(ses) for the LCD screen
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C display(0x26,20,4);          // Initialize display



/* ------------------------------------------------------------------------- *
 *                                                   Initial routine setup()
 * ------------------------------------------------------------------------- */
void setup() {

  delay(1000);                              // Delay execution before start

  pinMode(POWERLED, OUTPUT);                // Power indicator LED

  display.init();                           // Initialize display
  display.backlight();                      // Backlights on by default

  doInitialScreen(3);                       // Show when no debugging

  debugstart(115200);                       // Start serial

  debugln(F("==============================="));
  debug("GAW-MR-Control v");
  debugln(progVersion);
  debugln(F("==============================="));

  debugln(F("Start initialization"));
  debug("MemSize   = "); debugln(memSize);
  debug("entrySize = "); debugln(entrySize);
  debug("tableSize = "); debugln(sizeof(element));
  debug("nElements = "); debugln(nElements);

  debugln(F("Initialize LocoNet"));
  LocoNet.init();                           // Initialize Loconet

  debugln(F("Restore state from memory"));
  recallState();                            // Recall state from EEPROM

  restoreState();                             // Make state as it was!

#if DEBUG_LVL > 1
  showElements();
#endif

  debugln(F("Setup done, ready for operations"));
  debugln(F("==============================="));

}



/* ------------------------------------------------------------------------- *
 *                                                     Repeating code loop()
 * ------------------------------------------------------------------------- */
void loop() {

  char key = controlPanel.getKey();
  if(key) {                                 // Check for a valid key
    debugln("received "+String((int)key));
    handleButtons(key);                    //   and handle key
  }


/* ------------------------------------------------------------------------- *
 *                                         process incoming Loconet messages
 * ------------------------------------------------------------------------- */
  LnPacket = LocoNet.receive();

  if (LnPacket) {
    LocoNet.processSwitchSensorMessage(LnPacket);
  }

/*
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
 *       Routine to handle buttons on the control panel      handleButtons()
 * ------------------------------------------------------------------------- */
void handleButtons(char key) {

  int index = key - 1;                      // Keycode to table index

  switch(element[index].type) {

    case 0:                                 // Switch TYPE
      handleSwitch(index);
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
 *                                                        handleLocomotive()
 * ------------------------------------------------------------------------- */
void handleLocomotive(int index) {
  debug("Loc # ");                                // Just display address
  debugln(element[index].address);                //   for future use
  activeLoc = element[index].address;

  setLocSpeed(index);
}

  

/* ------------------------------------------------------------------------- *
 *                                                           handleSwitch()
 * ------------------------------------------------------------------------- */
void setLocSpeed(int index) {
  LCD_display(display, 1, 0, "Active Loc "+String(activeLoc)+"   ");

// SET LOCONET COMMAND TO Z21
//   TO SET SWITCH
}

  

/* ------------------------------------------------------------------------- *
 *                                                           handleSwitch()
 * ------------------------------------------------------------------------- */
void handleSwitch(int index) {
  element[index].state = !element[index].state;   // Flip state

  debug("Switch # "); 
  debug(element[index].address); debug(" - ");
  debugln(element[index].state ? "straight" : "thrown"); 
  
  setSwitch(index);
}



/* ------------------------------------------------------------------------- *
 *                                                              setSwitch()
 * ------------------------------------------------------------------------- */
void setSwitch(int index) {
  delay(500);
  debug("Set Switch "+String(element[index].address)+" to ");
  debugln(element[index].state == 0 ? F("Straight") : F("Thrown") );

  LCD_display(display, 0, 0, F("Switch             "));
  LCD_display(display, 0, 7, String(element[index].address));
  LCD_display(display, 0,12, element[index].state == 0 ? F("Straight") : F("Thrown") );

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

    case 9003:                              // Show elements
      showElements();
      break;

    default:
      break;

  }

}



/* ------------------------------------------------------------------------- *
 *                                                             recallState()
 * ------------------------------------------------------------------------- */
void recallState() {
  for (int i=0; i<nElements; i++) {
    EEPROM.get(i*entrySize, element[i]);
  }
  LCD_display(display, 3, 0, "State recalled      ");
}



/* ------------------------------------------------------------------------- *
 *                                                              storeState()
 * ------------------------------------------------------------------------- */
void storeState() {
  for (int i=0; i<nElements; i++) {
    EEPROM.put(i*entrySize, element[i]);
  }
  LCD_display(display, 3, 0, "State stored        ");
}



/* ------------------------------------------------------------------------- *
 *                                                             handlePower()
 * ------------------------------------------------------------------------- */
void handlePower(int index) {
  element[index].state = !element[index].state;   // Flip state

  setPower(element[index].state);

  LCD_display(display, 3,17, element[index].state ? "ON " : "OFF");
}



/* ------------------------------------------------------------------------- *
 *                                                                setPower()
 * ------------------------------------------------------------------------- */
void setPower(int state) {
  debug("Setting Power ");
  debugln(state == 0 ? F("OFF") : F("ON") );
  state ? digitalWrite(POWERLED, HIGH) : digitalWrite(POWERLED, LOW);

// SET LOCONET COMMAND TO Z21
//   TO SET POWER STATE
}

  

/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
    screen.setCursor(col, row);
    screen.print(text);
}



/* ------------------------------------------------------------------------- *
 *                                                            showElements()
 * ------------------------------------------------------------------------- */
void showElements() {
  debugln(F("Show elements table:"));
  for (int i=0; i<nElements; i++) {
    debug(String(i+1));
    debug(F(" - Type: "));
    debug(element[i].type);
    debug(F(" - Module: "));
    debug(element[i].module);
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
    
    debug(element[i].address);
    debug(F(" - "));

    switch (element[i].type) {
      case 0:
        debug(element[i].state == 0 ? F("Straight") : F("Thrown") ); debugln();
        break;

      case 1:
        debug("Speed: "+String(element[i].state)); debugln();
        break;

      case 90:
        if (element[i].address == 9001) debugln("Store");
        if (element[i].address == 9002) debugln("Recall");
        if (element[i].address == 9003) debugln("Show Elements");
        break;
      
      case 99:
        debugln(element[i].state == 0 ? "OFF" : "ON" );
        break;

      default:
        break;

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
 *                                                              resetState()
 * ------------------------------------------------------------------------- */
void restoreState() {
  int pwr = 0;                              // Assume power off

  for (int i=0; i<nElements; i++) {         // FIRST: restore power state
    if (element[i].type == 99) {
        pwr = element[i].state;
        setPower(element[i].state);
    }
  }

  if (pwr) {                                // Power on? then Switchs
    for (int i=0; i<nElements; i++) {
      if (element[i].type == 0) {
          setSwitch(i);
      }
    }
  }

}
