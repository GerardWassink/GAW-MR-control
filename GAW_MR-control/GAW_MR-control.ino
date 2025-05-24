/* ------------------------------------------------------------------------- *
 * Name   : GAW-MR-control
 * Author : Gerard Wassink
 * Date   : May 2025
 * Purpose: Control model railway through switch panel via a Loconet connection
 * See GAW_MR.h for more information
 */

#include "GAW_MR.h"

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
