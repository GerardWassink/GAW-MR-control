
/* ------------------------------------------------------------------------- *
 *                                                               Definitions
 * ------------------------------------------------------------------------- */
#define ROWS 8                              // Definitions for
#define COLS 8                              //  key-matrix

#define POWEROFF 0                          // Power states
#define POWERON  1

#define STRAIGHT 0                          // Definitions for
#define THROWN   1                          //  Switch states

#define FORWARD  1                          // Definitions
#define STOP     0                          //   for loc
#define REVERSE -1                          //     direction

#define TYPE_SWITCH    0                    // Types
#define TYPE_LOCO      1                    //  for
#define TYPE_FUNCTION 90                    //   element
#define TYPE_POWER    99                    //     array

#define NO_MODULE   0                       // Module names
#define MODULE_NWW  1                       //  by compass bearings
#define MODULE_NW   2
#define MODULE_NE   3
#define MODULE_NEE  4
#define MODULE_SWW  5
#define MODULE_SW   6
#define MODULE_SE   7
#define MODULE_SEE  8

#define FUNC_STORE     9001                 // Functions
#define FUNC_RECALL    9002                 //  for handling 
#define FUNC_ACTIVATE  9003                 //   state of
#define FUNC_SHOW      9004                 //    the layout

#define FUNC_FORWARD   9101                 // Functions
#define FUNC_STOP      9102                 //  for handling
#define FUNC_REVERSE   9103                 //   locomotives
#define FUNC_LIGHTS    9104
#define FUNC_SOUND     9105
#define FUNC_WHISTLE   9106
#define FUNC_HORN      9107
#define FUNC_TWOTONE   9108

#define FUNC_POWER     9999

#define LN_TX_PIN 42                        // Loconet TX pin

#define POWERLED  53                        // Panel Power indicator

#define memSize EEPROM.length()             // Amount of EEPROM memory

