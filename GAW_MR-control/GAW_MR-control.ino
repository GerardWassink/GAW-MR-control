/* ------------------------------------------------------------------------- *
 * Name   : GAW-MR-control
 * Author : Gerard Wassink
 * Date   : May 2025
 * Purpose: Control model railway through switch panel via a Loconet connection
 * Contributions by:
 *  @tanner87661
 *            https://www.tindie.com/products/tanner87661/loconet-interface-breakout-board-with-grove-port/
 *  @dylanmyer
 *            suggestion for splitting up code in .h files, 
 *            and making static definitions
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
 *          Small improverments
 *   0.11   New defines instead of numbers
 *   0.12   Improved activation at startup
 *          Shorter pause between activation of switches
 *          Improved readability of error message
 *          Captured some more notify packets
 *          Introduced some static strings
 *          Built in power notification
 *
 *   1.0    First release
 *
 *   1.1    Rolled back the LN_SW_UART_RX_INVERTED definition
 *            in the ln_config,h file
 *   1.2    Send all Loconet commands twice because sometimes the 
 *            DCC commands got lost
 *   1.3    Improved and corrected switch handling
 *   1.4    Replaced switch handling for solenoids with 'normal' ones
 *            seems to be more dependable
 *
 *------------------------------------------------------------------------- */
#define progVersion "1.4"                  // Program version definition
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
#include "GAW_MR_defines.h"                 // various definitions
#include "GAW_MR_layout.h"                  // Define the layout
#include "GAW_MR_multiplexer.h"             // MCP23017 boards definitions
#include "GAW_MR_controlpanel.h"            // Controlpanel definitions

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

//  storeState();                             // to replace it with the definitions in the code
//  exit(0);

  LCD_display(display, 1, 0, F("                    "));
  recallState();                            // By default recall state from EEPROM

  activateState();                          // Activate recalled state

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

  LnPacket = LocoNet.receive();             // Process incoming Loconet msgs
  if (LnPacket) {
    LocoNet.processSwitchSensorMessage(LnPacket);
  }

  char key = controlPanel.getKey();         // Process keypress
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

    case TYPE_SWITCH:                       // SWITCH TYPE
      flipSwitch(index);
      break;

    case TYPE_LOCO:                         // LOCOMOTIVE TYPE
      handleLocomotive(index);
      break;

    case TYPE_FUNCTION:                     // FUNCTION TYPE
      handleFunction(index);
      break;
 
    case TYPE_POWER:                        // POWER TYPE
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
#if DEBUG_LVL > 1
  debug("--- flipSwitch");
#endif

#if DEBUG_LVL > 2
  debug("flipSwitch from " + String( element[index].state ) + " to " );
#endif

  if (element[index].state == STRAIGHT) {
    element[index].state = THROWN;
  } else {
    element[index].state = STRAIGHT;
  }

#if DEBUG_LVL > 2
  debugln( String(element[index].state) );
#endif

  setSwitch(index);
}



/* ------------------------------------------------------------------------- *
 *                                                               setSwitch()
 * ---- Send Loconet command to Command station (Z21) to set the switch ---- *
 * ------------------------------------------------------------------------- */
void setSwitch(int index) {
#if DEBUG_LVL > 1
    debugln("--- setSwitch " + String(element[index].address) + " to " + ( element[index].state == STRAIGHT ? STATE_STRAIGHT : STATE_THROWN ) );
#endif 

                                            // Old way for solenoid switches
//  setLNTurnout(element[index].address, element[index].state);

                                            // Current way for our switches
  sendOPC_SW_REQ(element[index].address - 1, element[index].state, 1);

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
  byte direction = element[activeLoc].state;
  int  speedstep = element[activeLoc].state2;

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

    case FUNC_STORE:                        // Save status
      storeState();
      break;
  
    case FUNC_RECALL:                       // Recall status
      recallState();
      break;

    case FUNC_ACTIVATE:                     // Activate status
      activateState();
      break;

    case FUNC_SHOW:                         // Show elements
      showElements();
      break;



    case FUNC_FORWARD:                      // Loc Forward
      locForward();
      break;

    case FUNC_STOP:                         // Loc Stop
      locStop();
      break;

    case FUNC_REVERSE:                      // Loc Reverse
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
  debugln(state == POWEROFF ? F("OFF") : F("ON") );
  state ? digitalWrite(POWERLED, HIGH) : digitalWrite(POWERLED, LOW);

  LCD_display(display, 3,10, "Power: ");
  LCD_display(display, 3,17, state == POWERON ? "ON " : "OFF");

/* --- Send Loconet command to command station (Z21) to set power state ---- */
  sendOPC_GP(state);

}


/* ------------------------------------------------------------------------- *
 *                                                            showElements()
 * Testing purposes: show array of elements and their states
 * ------------------------------------------------------------------------- */
void showElements() {
  debugln(F("Show elements table:"));
  for (int i=0; i<nElements; i++) {
    debug(String(i+1));

    debug(F(" - Type: "));
    debug(element[i].type);
    switch (element[i].type) {
      case TYPE_SWITCH:
        debug(F(" - Switch: "));
        break;

      case TYPE_LOCO:
        debug(F(" - Locomotive: "));
        break;
        
      case TYPE_FUNCTION:
        debug(F(" - Funtion: "));
        break;
        
      case TYPE_POWER:
        debug(F(" - Power: "));
        break;

      default:
        break;
    }
    
    debug(String(element[i].address));
    debug(F(" - "));

    switch (element[i].type) {
      case TYPE_SWITCH:
        debug("state="+String(element[i].state) + ", ");
        debug(element[i].state  == STRAIGHT ? STATE_STRAIGHT : STATE_THROWN );
        debug(F(" - Module: "));
        debugln(element[i].module);
        break;

      case TYPE_LOCO:
        if (element[i].state == 1) {
          debug("Reverse, ");
        } else if (element[i].state == 0) {
          debug("Stop, ");
        } else if (element[i].state == 2) {
          debug("Forward, ");
        }
        debugln("Speed: "+String(element[i].state2));
        break;

      case TYPE_FUNCTION:
        showFunctions(i);
        break;
      
      case TYPE_POWER:
        debugln(element[i].state == POWEROFF ? "OFF" : "ON" );
        break;

      default:
        break;

    }
  }
}


/* ------------------------------------------------------------------------- *
 *                                                           showFunctions()
 * ------------------------------------------------------------------------- */
void showFunctions(int index) {
  switch (element[index].address) {

    case FUNC_STORE:    debugln("Store state"); break;
    case FUNC_RECALL:   debugln("Recall state"); break;
    case FUNC_ACTIVATE: debugln("Activate state"); break;
    case FUNC_SHOW:     debugln("Show Elements"); break;

    case FUNC_FORWARD:  debugln("Loc Forward"); break;
    case FUNC_STOP:     debugln("Loc Stop"); break;
    case FUNC_REVERSE:  debugln("Loc Reverse"); break;
    case FUNC_LIGHTS:   debugln("Loc Lights"); break;
    case FUNC_SOUND:    debugln("Loc Sound"); break;
    case FUNC_WHISTLE:  debugln("Loc Whistle"); break;
    case FUNC_HORN:     debugln("Loc Horn"); break;
    case FUNC_TWOTONE:  debugln("Loc Two-tone Horn"); break;
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
  debugln("System status stored");
  LCD_display(display, 3, 0, "Stored");
  delay(1000);
  LCD_display(display, 3, 0, F("      "));
}



/* ------------------------------------------------------------------------- *
 *                                                             recallState()
 * ------------------------------------------------------------------------- */
void recallState() {
  debugln("Recalling system status");
  for (int i=0; i<nElements; i++) {
    EEPROM.get(i*entrySize, element[i]);
  }
  LCD_display(display, 3, 0, "Recalled");
  delay(1000);
  LCD_display(display, 3, 0, F("        "));

#if DEBUG_LVL > 1
  showElements();
#endif

}



/* ------------------------------------------------------------------------- *
 *                                                           activateState()
 * ------------------------------------------------------------------------- */
void activateState() {
  debugln("Synchronising system status to layout");
  LCD_display(display, 1, 0, "Sync state          ");

  int pwr = 0;                              // Assume power off
  int index = 0;

  for (index = 0; index < nElements; index++) {  // FIRST: restore power state
    if (element[index].type == 99) {        // Power state found
        pwr = element[index].state;         // What was the state?
        setPower(element[index].state);     // Set power on / off
    }
  }

  if (pwr) {                                // Power on? then Switches
    for (index = 0; index < nElements; index++) {
      unsigned long prevMillis = millis();
                                            // Is it a switch?
                                            //  & address > zero?
      if (element[index].type == TYPE_SWITCH && element[index].address > 0 ) {
        LCD_display(display, 1, 15, String(element[index].address) );
        
        setSwitch(index);                   //  then set proper value
        
        LnPacket = LocoNet.receive();             // process incoming Loconet msgs
        if (LnPacket) {
          LocoNet.processSwitchSensorMessage(LnPacket);
        }

        do {} while (millis() - prevMillis < 50 );

      }
    }
  }

  delay(1000);
  LCD_display(display, 1, 0, "                    " );

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


// Some turnout decoders (DS54?) can use solenoids, this code emulates the digitrax 
// throttles in toggling the "power" bit to cause a pulse
void setLNTurnout(int address, byte dir) {
#if DEBUG_LVL > 2
  debugln("--- setLNTurnout, direction = "+String(dir) );
#endif

/* ------------------------------------------------------------------------- *
 * Sending the commands twice, just to be sure, because sometimes the 
 * LocoNet commands went through, but the DCC commands got lost...
 * ------------------------------------------------------------------------- */
    sendOPC_SW_REQ(address - 1, dir, 1);
    sendOPC_SW_REQ(address - 1, dir, 0);
    delay(20);

    sendOPC_SW_REQ(address - 1, dir, 1);
    sendOPC_SW_REQ(address - 1, dir, 0);
    delay(20);

}


// Construct a Loconet packet that requests a turnout to set/change its state
void sendOPC_SW_REQ(int address, byte dir, byte on) {
#if DEBUG_LVL > 2
  debugln("--- sendOPC_SW_REQ, "+String(address)+String(dir)+", "+String(on) );
#endif
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



/* ------------------------------------------------------------------------- *
 *                                                             notifyPower()
 * This call-back function is called from the routine
 * LocoNet.processSwitchSensorMessage for the power Request messages
 * ------------------------------------------------------------------------- */
void notifyPower(uint8_t State) {
  Serial.print("Layout Power State: ");
  Serial.println(State ? "On" : "Off");

  State == POWERON ? digitalWrite(POWERLED, HIGH) : digitalWrite(POWERLED, LOW);

  LCD_display(display, 3,10, "Power: ");
  LCD_display(display, 3,17, State == POWERON ? "ON " : "OFF");

}



/* ------------------------------------------------------------------------- *
 *                                                     notifySwitchRequest()
 * These call-back functions are called from the routine
 * LocoNet.processSwitchSensorMessage for all Switch Request messages
 * ------------------------------------------------------------------------- */
void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t State ) {
#if DEBUG_LVL > 2
  debugln("--- notifySwitchRequest, "+String(Address)+", "+String(Output)+", "+String(State));
#endif
  handleSwitchRequest( Address, Output, State );
}

void notifySwitchReport( uint16_t Address, uint8_t Output, uint8_t Direction ) {
#if DEBUG_LVL > 2
  debugln("--- notifySwitchReport, "+String(Address)+", "+String(Output)+", "+String(Direction));
#endif
  handleSwitchRequest( Address, Output, Direction );
}

void notifySwitchState( uint16_t Address, uint8_t Output, uint8_t Direction ) {
#if DEBUG_LVL > 2
  debugln("--- notifySwitchState, "+String(Address)+", "+String(Output)+", "+String(Direction));
#endif
  handleSwitchRequest( Address, Output, Direction );
}



/* ------------------------------------------------------------------------- *
 *                                                     HandleSwitchRequest()
 * This call-back function is called from LocoNet.processSwitchSensorMessage
 * for all Switch Request messages
 * ------------------------------------------------------------------------- */
void handleSwitchRequest( uint16_t Address, uint8_t Output, uint8_t state ) {
#if DEBUG_LVL > 2
  debugln("--- handleSwitchRequest, "+String(Address)+", "+String(Output)+", "+String(state));
#endif

  int index;
  for (index = 0; index < nElements; index++) { // Look up Switch address
    if (element[index].address == Address) {
      break;
    }
  }

  if (element[index].type == TYPE_SWITCH && index < nElements) {
                                            // Calculate mx address and port 
    int mx = (index / 16) * 2;              //  for the even numbered mux
    int port = (index % 16);                //  from switch position in elements

    int val = (state == 0 ? 0 : 1 );          // To set mux ports
    mcps[mx].mcp.digitalWrite(port, val );    // Set first LED on or off
    mcps[mx+1].mcp.digitalWrite(port, !val ); // Set second LED on or off

#if DEBUG_LVL > 1
    debug("Set Switch "+String(element[index].address)+" to "+ String(state) );
    debug(" - mx "+String(mx)+","+String(port)+" = "+String(val) );
    debug(", mx "+String(mx+1)+","+String(port)+" = "+ String(!val) );
    debug(" - ");
    debugln(Output ? "On" : "Off");
#endif

    LCD_display(display, 0, 0, F("Switch              "));
    LCD_display(display, 0, 7, String(Address));
    LCD_display(display, 0,12, state == STRAIGHT ? STATE_STRAIGHT : STATE_THROWN );

  } else {

    debug("Address " + String(element[index].address) + " :: ");
    debugln("ERROR ERROR ERROR :: Address not found");

  }
}


