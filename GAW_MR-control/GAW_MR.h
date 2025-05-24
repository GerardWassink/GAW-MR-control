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

#define UNUSED -1                          // Unused state

#define TYPE_SWITCH 0                      // Type definitions
#define TYPE_LOCOMOTIVE 1
#define TYPE_FUNCTION 90
#define TYPE_POWER 99

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
 * Types: 0 = Switch, 1 = Locomotive, 90 = function, 99 = power
 * ------------------------------------------------------------------------- */
struct MR_data element[] = {

// ===== CAVEAT =====
// Switches MUST come first in this array, as calculations for the 
// LED multiplexers are based on the index of the switches in the 
// element array
// ===== CAVEAT =====

/* ------------------------------------------------------------------------- *
 *   Type = TYPE_SWITCH, Switches:
 *   module  = layout module, administrative only for now
 *   address = DCC address of the switch
 *   state   = actual state of the switch
 *   state2  = opposite state, used for 2nd LED
 * ------------------------------------------------------------------------- */

//              Layout module 1
   TYPE_SWITCH, 1, 101, STRAIGHT, THROWN,
   TYPE_SWITCH, 1, 102, STRAIGHT, THROWN,
   TYPE_SWITCH, 1, 103, STRAIGHT, THROWN,
   TYPE_SWITCH, 1, 104, STRAIGHT, THROWN,

//              Layout module 2
   TYPE_SWITCH, 2, 201, STRAIGHT, THROWN,
   TYPE_SWITCH, 2, 202, STRAIGHT, THROWN,
   TYPE_SWITCH, 2, 203, STRAIGHT, THROWN,

//              Layout module 4
   TYPE_SWITCH, 4, 401, STRAIGHT, THROWN,
   TYPE_SWITCH, 4, 402, STRAIGHT, THROWN,
   TYPE_SWITCH, 4, 403, STRAIGHT, THROWN,
   TYPE_SWITCH, 4, 404, STRAIGHT, THROWN,
   TYPE_SWITCH, 4, 405, STRAIGHT, THROWN,
   TYPE_SWITCH, 4, 406, STRAIGHT, THROWN,
   TYPE_SWITCH, 4, 407, STRAIGHT, THROWN,

//              Layout module 5
   TYPE_SWITCH, 5, 501, STRAIGHT, THROWN,
   TYPE_SWITCH, 5, 502, STRAIGHT, THROWN,

//              Layout module 6
   TYPE_SWITCH, 6, 601, STRAIGHT, THROWN,
   TYPE_SWITCH, 6, 602, STRAIGHT, THROWN,
   TYPE_SWITCH, 6, 603, STRAIGHT, THROWN,

//              Layout module 7
   TYPE_SWITCH, 7, 701, STRAIGHT, THROWN,

//              Layout module 8
   TYPE_SWITCH, 8, 801, STRAIGHT, THROWN,
   TYPE_SWITCH, 8, 802, STRAIGHT, THROWN,
   TYPE_SWITCH, 8, 803, STRAIGHT, THROWN,
   TYPE_SWITCH, 8, 804, STRAIGHT, THROWN,
   TYPE_SWITCH, 8, 805, STRAIGHT, THROWN,

/* ------------------------------------------------------------------------- *
 * 7 spare switch positions, for possible future expansion
 * ------------------------------------------------------------------------- */
   TYPE_SWITCH, 0,   0, STRAIGHT, THROWN,
   TYPE_SWITCH, 0,   0, STRAIGHT, THROWN,
   TYPE_SWITCH, 0,   0, STRAIGHT, THROWN,
   TYPE_SWITCH, 0,   0, STRAIGHT, THROWN,
   TYPE_SWITCH, 0,   0, STRAIGHT, THROWN,
   TYPE_SWITCH, 0,   0, STRAIGHT, THROWN,
   TYPE_SWITCH, 0,   0, STRAIGHT, THROWN,

/* ------------------------------------------------------------------------- *
 * Type = 1, Locomotives:
 *   module  = arbitrary, not used
 *   address = DCC address of the locomotive
 *   state   = -1 = reverse, 0 = stopped, 1 = forward
 *   state2  = speed step
 * ------------------------------------------------------------------------- */

//              My locomotives
   TYPE_LOCOMOTIVE, 0, 344, FORWARD, 0,                   // Hondekop
   TYPE_LOCOMOTIVE, 0, 386, FORWARD, 0,                   // BR 201 386
   TYPE_LOCOMOTIVE, 0, 611, FORWARD, 0,                   // NS 611
   TYPE_LOCOMOTIVE, 0, 612, FORWARD, 0,                   // NS 612
   TYPE_LOCOMOTIVE, 0,2412, FORWARD, 0,                   // NS 2412

/* ------------------------------------------------------------------------- *
 * Type = 90, Funtions:
 *   module  = arbitrary, not used
 *   address = Function number
 *   state   = not used
 *   state2  = not used
 * ------------------------------------------------------------------------- */

//              General Functions
  TYPE_FUNCTION, 0,9001, UNUSED, UNUSED,                  // Store state
  TYPE_FUNCTION, 0,9002, UNUSED, UNUSED,                  // Recall state
  TYPE_FUNCTION, 0,9003, UNUSED, UNUSED,                  // Show elements

//              Loc Functions
  TYPE_FUNCTION, 0,9101, UNUSED, UNUSED,                  // Forward
  TYPE_FUNCTION, 0,9102, UNUSED, UNUSED,                  // Stop
  TYPE_FUNCTION, 0,9103, UNUSED, UNUSED,                  // Reverse
  TYPE_FUNCTION, 0,9104, UNUSED, UNUSED,                  // Lights
  TYPE_FUNCTION, 0,9105, UNUSED, UNUSED,                  // Sound
  TYPE_FUNCTION, 0,9106, UNUSED, UNUSED,                  // Whistle
  TYPE_FUNCTION, 0,9107, UNUSED, UNUSED,                  // Horn
  TYPE_FUNCTION, 0,9108, UNUSED, UNUSED,                  // Two-tone Horn

/* ------------------------------------------------------------------------- *
 * Type = 99, Power:
 *   module  = arbitrary, not used
 *   address = Function number
 *   state   = power = 1 (on) / 0 (off)
 *   state2  = not used
 * ------------------------------------------------------------------------- */

//              POWER
  TYPE_POWER, 0,9999, POWEROFF, UNUSED,                   // Roco Z21
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
