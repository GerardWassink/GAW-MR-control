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
 *   0.9    Built in first Loconet command for Switches
 *   0.10   Code upgrade, isolated stuff into header files
 *             hat-tip at Dylan Myers for this suggestion
 *
 *------------------------------------------------------------------------- */
#define progVersion "0.10"                  // Program version definition
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
 *                                             Include headers for libraries
 * ------------------------------------------------------------------------- */
#include <EEPROM.h>                         // EEPROM library
#include <Keypad.h>                         // Keypad library
#include <LocoNet.h>                        // LocoNet library
#include <Wire.h>                           // I2C comms library
#include <LiquidCrystal_I2C.h>              // LCD library
#include <Adafruit_MCP23X17.h>              // I/O expander library

/* ------------------------------------------------------------------------- *
 *                                                   Include private headers
 * ------------------------------------------------------------------------- */
#include "GAW_debugging.h"                  // Debugging level code
#include "GAW_MR_defines.h"                    // various definitions
#include "GAW_MR_layout.h"                     // Define the layout
#include "GAW_MR_multiplexer.h"                // MCP23017 boards definitions
#include "GAW_MR_controlpanel.h"               // Controlpanel definitions

/* ------------------------------------------------------------------------- *
 *                                       Global variables needed for Loconet
 * ------------------------------------------------------------------------- */
lnMsg *LnPacket;
int SwitchDirection;


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
  LocoNet.init(LN_TX_PIN);                  // Initialize Loconet
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

  LnPacket = LocoNet.receive();             // process incoming Loconet msgs
  if (LnPacket) {
    LocoNet.processSwitchSensorMessage(LnPacket);
  }

  char key = controlPanel.getKey();         // process keypress
  if(key) {                                 // Check for a valid key
    handleKeys(key);                        //   and handle key
  }

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
  element[index].state == !element[index].state;  // Flip state
  setSwitch(index);
}



/* ------------------------------------------------------------------------- *
 *                                                               setSwitch()
 * ------------------------------------------------------------------------- */
void setSwitch(int index) {

// SET LOCONET COMMAND TO Z21
//   TO SET SWITCH
  setLNTurnout(element[index].address, element[index].state);  // Actually set switch

}



/* ------------------------------------------------------------------------- *
 *                                                     notifySwitchRequest()
 * This call-back function is called from LocoNet.processSwitchSensorMessage
 * for all Switch Request messages
 * ------------------------------------------------------------------------- */
void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t state ) {
  int index;
  for (index = 0; index < nElements; index++) {
    if (element[index].address == Address) {
      break;
    }
  }
  if (index >= nElements) {
    debugln("ERROR ERROR ERROR :: Address not found");
    exit(0);
  }
                                            // Calculate mx address and port 
  int mx = (index / 16) * 2;                //  for the even numbered mux
  int port = (index % 16);                  //  from switch position in elements

  debug("Set Switch "+String(element[index].address)+" to "+ ( state == STRAIGHT ? "Thrown  " : "Straight") );
  debug(" - mx "+String(mx)+","+String(port)+" = "+state);

  mcps[mx].mcp.digitalWrite(port, state);   // Set first LED on or off
  mx++;                                     // One up for odd number mux
  mcps[mx].mcp.digitalWrite(port, !state);  // Set second LED on or off

  debug(", mx "+String(mx)+","+String(port)+" = "+!state);
  debug(" - ");
  debugln(Output ? "On" : "Off");

  LCD_display(display, 0, 0, F("Switch              "));
  LCD_display(display, 0, 7, String(Address));
  LCD_display(display, 0,12, state == STRAIGHT ? "Thrown  " : "Straight");
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

// SET LOCONET COMMAND TO Z21
//   TO SET POWER STATE
  sendOPC_GP(state);

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
  LCD_display(display, 0, 0, "Activating state");

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
          LCD_display(display, 0, 17, String(index));
          setSwitch(index);                 //  then set proper value
          delay(1000);
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


// Construct a Loconet packet that requests a turnout to set/change its state
void sendOPC_SW_REQ(int address, byte dir, byte on) {
    lnMsg SendPacket ;
    
    int sw2 = 0x00;
    if (dir == STRAIGHT) { sw2 |= B00100000; }
    if (on) { sw2 |= B00010000; }
    sw2 |= (address >> 7) & 0x0F;
    
    SendPacket.data[ 0 ] = OPC_SW_REQ ;
    SendPacket.data[ 1 ] = address & 0x7F ;
    SendPacket.data[ 2 ] = sw2 ;
    
    LocoNet.send( &SendPacket );
}

// Some turnout decoders (DS54?) can use solenoids, this code emulates the digitrax 
// throttles in toggling the "power" bit to cause a pulse
void setLNTurnout(int address, byte dir) {
    sendOPC_SW_REQ(address - 1, dir, 1);
    sendOPC_SW_REQ(address - 1, dir, 0);
}


// Set power status
void sendOPC_GP(byte on) {
        lnMsg SendPacket;
        if (on) {
            SendPacket.data[ 0 ] = OPC_GPON;  
        } else {
            SendPacket.data[ 0 ] = OPC_GPOFF;  
        }
        LocoNet.send( &SendPacket ) ;
}

