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
 *
 *---------------------------------------------                                                          ---------------------------- */
#define progVersion "0.3"  // Program version definition
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
#define ROWS 6
#define COLS 6

#define POWEROFF 0
#define POWERON  1

#define STRAIGHT 0
#define THROWN 1

#define nElements sizeof(element) / sizeof(MR_data)

#define memSize EEPROM.length()             // Amount of EEPROM memory

#define POWERLED 53                         // Panel Power indicator


/* ====================== FOR TESTING LOCONET ===================*/
#define BUTTON_PIN  4
#define TURNOUT_ADDRESS 501
/* ====================== FOR TESTING LOCONET ===================*/


/* ------------------------------------------------------------------------- *
 *                                                          Global variables
 * ------------------------------------------------------------------------- */
lnMsg *LnPacket;
int turnoutDirection;

bool buttonPressed = false;

int activeLoc = 0;                          // (future use)

/* ------------------------------------------------------------------------- *
 *                                                         Element structure
 * ------------------------------------------------------------------------- */
struct MR_data{
  int type;
  int module;
  int address;
  int state;
};

int entrySize = sizeof(MR_data);

struct MR_data element[] = {

// Types: 0 = turnout, 1 = Locomotive, 99 = power

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
   1, 0, 344, 0,
   1, 0, 386, 0,
   1, 0, 611, 0,
   1, 0, 612, 0,
   1, 0, 611, 0,
   1, 0,2412, 0,

//              POWER
  99, 0,   0, POWEROFF

};


/* ------------------------------------------------------------------------ *
 *       Define controlPanel variables
 *         this is the control panel for the model railroad layout
 *         The buttons are handled in a 6 x 6 grid
 * ------------------------------------------------------------------------- */
char keys[ROWS][COLS] = {
  { 1, 2, 3, 4, 5, 6},
  { 7, 8, 9,10,11,12},
  {13,14,15,16,17,18},
  {19,20,21,22,23,24},
  {25,26,27,28,29,30},
  {31,32,33,34,35,36},
};

byte rowPins[ROWS] = {22,23,24,25,26,27}; // row pins of the controlPanel
byte colPins[COLS] = {28,29,30,31,32,33}; // column pins of the controlPanel

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

  pinMode(4, INPUT_PULLUP);                 // Loconet TEST-SWITCH
  pinMode(POWERLED, OUTPUT);                // Power indicator LED

  display.init();                           // Initialize display
  display.backlight();                      // Backlights on by default
  doInitialScreen(3);                       // Show when no debugging

  debugstart(115200);                       // Start serial

  debugln(F("==============================="));
  debug("GAW-Turnouts v");
  debugln(progVersion);
  debugln(F("==============================="));

  debugln(F("Start initialization"));
  debug("MemSize   = "); debugln(memSize);
  debug("entrySize = "); debugln(entrySize);
  debug("tableSize = "); debugln(sizeof(element));
  debug("nElements = "); debugln(nElements);

#if DEBUG_LVL > 1
  debugln(F("Show elements:"));
  for (int i=0; i<25; i++) {
    debug(F("Type: "));
    debug(element[i].type);
    debug(F(" - Module: "));
    debug(element[i].module);
    debug(F(" - Element: "));
    debug(element[i].address);
    debug(" - State: ");
    element[i].state == 0? debug("Straight") : debug("Thrown");
    debugln();
  }
#endif

  debugln(F("Initialize LocoNet"));
  LocoNet.init();                           // Initialize Loconet

  LCD_display(display, 1, 0, F("                    "));
  LCD_display(display, 2, 0, F("                    "));

  debugln(F("Setup done, ready for operations"));
  debugln(F("==============================="));

}



/* ------------------------------------------------------------------------- *
 *                                                     Repeating code loop()
 * ------------------------------------------------------------------------- */
void loop() {

  char key = controlPanel.getKey();
  if(key) {                                 // Check for a valid key.
    handleButtons(key);                    //   and handle key
  }


/* ------------------------------------------------------------------------- *
 *                                         process incoming Loconet messages
 * ------------------------------------------------------------------------- */
  LnPacket = LocoNet.receive();

  if (LnPacket) {
    LocoNet.processSwitchSensorMessage(LnPacket);
  }

  // button pressed
  if(digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {

    if(turnoutDirection == 0) {
      turnoutDirection = 1;
    } else {
      turnoutDirection = 0;
    }

    LocoNet.requestSwitch(TURNOUT_ADDRESS, 0, turnoutDirection);
    buttonPressed = true;

  } else {  // button released

    if(digitalRead(BUTTON_PIN) == HIGH) {
      buttonPressed = false;
    } else {
      if(digitalRead(BUTTON_PIN) == HIGH) buttonPressed = false;
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
  
}



/* ------------------------------------------------------------------------- *
 *       Routine to handle buttons on the control panel      handleButtons()
 * ------------------------------------------------------------------------- */
void handleButtons(char key) {

  int index = key - 1;                       // Keycode to table index

  switch(element[index].type) {

    case 0:                                           // TURNOUT
      element[index].state = !element[index].state;   // Flip state
      debug("Turnout # "); 
      debug(element[index].address); debug(" - ");
      debugln(element[index].state ? "straight" : "thrown"); 
      displayTurnoutState(index);
      break;

    case 1:                                           // LOCOMOTIVE
      debug("Loc # ");                                // Just display address
      debugln(element[index].address);                //   future use
      activeLoc = element[index].address;
      LCD_display(display, 2, 0, "Loc "+String(activeLoc)+"   ");
      break;

    case 99:                                          // POWER 
      element[index].state = !element[index].state;   // Flip state
      debug("POWER # "); 
      debug(element[index].address); debug(" - ");
      debugln(element[index].state ? "ON" : "OFF");
      element[index].state ? digitalWrite(POWERLED, HIGH) : digitalWrite(POWERLED, LOW);
      LCD_display(display, 3,17, element[index].state ? "ON " : "OFF");
      break;

    default: 
      break;

  }

}

  

/* ------------------------------------------------------------------------- *
 *                                                     displayTurnoutState()
 * ------------------------------------------------------------------------- */
void displayTurnoutState(int index) {
    LCD_display(display, 1, 0, F("Turnout             "));
    LCD_display(display, 1, 8, String(element[index].address));
    LCD_display(display, 1,12, element[index].state ? "straight" : "thrown  ");
}

  

/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
    screen.setCursor(col, row);
    screen.print(text);
}


