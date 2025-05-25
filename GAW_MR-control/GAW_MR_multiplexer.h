
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

#define numberOfMx sizeof(mcps) / \
                  sizeof(MCPINFO)           // Number of expander interfaces

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


