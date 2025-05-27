
/* ------------------------------------------------------------------------- *
 * 
 * Definition for the layout and it's components and control panel functions
 * 
 * ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- *
 *                                               Size and number of elements
 * ------------------------------------------------------------------------- */
#define entrySize sizeof(MR_data)               // Dynamic definitions for
#define nElements sizeof(element) / entrySize   //  element size


/* ------------------------------------------------------------------------- *
 *                                                         Element structure
 * The MR_date structure defines the variables per element.
 * ------------------------------------------------------------------------- */
struct MR_data{                             // single element definition
  int           type;
  int           module;
  uint16_t      address;
  byte          state;
  int           state2;
};


/* ------------------------------------------------------------------------- *
 *                                                             Element array
 * The element[] array holds values for elements on the control panel.
 * At thispoint these are Switches, Locomotives, Functions and Power. 
 * ------------------------------------------------------------------------- */
struct MR_data element[] = {


// ===== CAVEAT ===== CAVEAT ===== CAVEAT ===== CAVEAT =====
// Switches MUST come first in this array, as calculations for the 
// LED multiplexers are based on the index of the switches in the 
// element array
// ===== CAVEAT ===== CAVEAT ===== CAVEAT ===== CAVEAT =====


/* ------------------------------------------------------------------------- *
 * Type = 0, Switches:
 *   module  = layout module, administrative only for now
 *   address = DCC address of the switch
 *   state   = actual state of the switch
 *   state2  = opposite state, used for 2nd LED
 * ------------------------------------------------------------------------- */

//              Layout module 1
   TYPE_SWITCH, MODULE_NWW, 101, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NWW, 102, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NWW, 103, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NWW, 104, STRAIGHT, 0,

//              Layout module 2
   TYPE_SWITCH, MODULE_NW,  201, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NW,  202, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NW,  203, STRAIGHT, 0,

//              Layout module 4
   TYPE_SWITCH, MODULE_NEE, 401, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NEE, 402, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NEE, 403, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NEE, 404, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NEE, 405, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NEE, 406, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_NEE, 407, STRAIGHT, 0,

//              MLayout mdule 5
   TYPE_SWITCH, MODULE_SWW, 501, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_SWW, 502, STRAIGHT, 0,

//              Layout module 6
   TYPE_SWITCH, MODULE_SW,  601, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_SW,  602, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_SW,  603, STRAIGHT, 0,

//              Layout module 7
   TYPE_SWITCH, MODULE_SE,  701, STRAIGHT, 0,

//              Layout module 8
   TYPE_SWITCH, MODULE_SEE, 801, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_SEE, 802, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_SEE, 803, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_SEE, 804, STRAIGHT, 0,
   TYPE_SWITCH, MODULE_SEE, 805, STRAIGHT, 0,

/* ------------------------------------------------------------------------- *
 * 7 spare switch positions, for possible future expansion
 * ------------------------------------------------------------------------- */
   TYPE_SWITCH, NO_MODULE,   0, 0, 0,
   TYPE_SWITCH, NO_MODULE,   0, 0, 0,
   TYPE_SWITCH, NO_MODULE,   0, 0, 0,
   TYPE_SWITCH, NO_MODULE,   0, 0, 0,
   TYPE_SWITCH, NO_MODULE,   0, 0, 0,
   TYPE_SWITCH, NO_MODULE,   0, 0, 0,
   TYPE_SWITCH, NO_MODULE,   0, 0, 0,

/* ------------------------------------------------------------------------- *
 * Type = 1, Locomotives:
 *   module  = arbitrary, not used
 *   address = DCC address of the locomotive
 *   state   = -1 = reverse, 0 = stopped, 1 = forward
 *   state2  = speed step
 * ------------------------------------------------------------------------- */

//              My locomotives
   TYPE_LOCO, NO_MODULE, 344, 1, 0,                 // Hondekop
   TYPE_LOCO, NO_MODULE, 386, 1, 0,                 // BR 201 386
   TYPE_LOCO, NO_MODULE, 611, 1, 0,                 // NS 611
   TYPE_LOCO, NO_MODULE, 612, 1, 0,                 // NS 612
   TYPE_LOCO, NO_MODULE,2412, 1, 0,                 // NS 2412

/* ------------------------------------------------------------------------- *
 * Type = 90, Funtions:
 *   module  = arbitrary, not used
 *   address = Function number
 *   state   = not used
 *   state2  = not used
 * ------------------------------------------------------------------------- */

//              General Functions
  TYPE_FUNCTION, NO_MODULE, FUNC_STORE,    0, 0,    // Store state
  TYPE_FUNCTION, NO_MODULE, FUNC_RECALL,   0, 0,    // Recall state
  TYPE_FUNCTION, NO_MODULE, FUNC_ACTIVATE, 0, 0,    // Activate state
  TYPE_FUNCTION, NO_MODULE, FUNC_SHOW,     0, 0,    // Show elements

//              Loc Functions
  TYPE_FUNCTION, NO_MODULE, FUNC_FORWARD, 0, 0,
  TYPE_FUNCTION, NO_MODULE, FUNC_STOP,    0, 0,
  TYPE_FUNCTION, NO_MODULE, FUNC_REVERSE, 0, 0,
  TYPE_FUNCTION, NO_MODULE, FUNC_LIGHTS,  0, 0,
  TYPE_FUNCTION, NO_MODULE, FUNC_SOUND,   0, 0,
  TYPE_FUNCTION, NO_MODULE, FUNC_WHISTLE, 0, 0,
  TYPE_FUNCTION, NO_MODULE, FUNC_HORN,    0, 0,
  TYPE_FUNCTION, NO_MODULE, FUNC_TWOTONE, 0, 0,

/* ------------------------------------------------------------------------- *
 * Type = 99, Power:
 *   module  = arbitrary, not used
 *   address = Function number
 *   state   = power = 1 (on) / 0 (off)
 *   state2  = not used
 * ------------------------------------------------------------------------- */

//              POWER
  TYPE_POWER,    NO_MODULE, FUNC_POWER, POWERON, 0,

};                                          // END OF element[] ARRAY


