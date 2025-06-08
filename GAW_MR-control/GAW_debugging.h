
/* ------------------------------------------------------------------------- *
 *                                                        DEBUGGING ON / OFF
 * Compiler directives to switch debugging on / off
 * Do not enable debug when not needed, Serial takes space and time
 * DEBUG_LVL:
 *   0 - no debug output
 *   1 - Elementary debug output
 *   2 - lvl 1 + verbose debug output
 *   3 - lvl 2 + track routine names
 * ------------------------------------------------------------------------- */
#define DEBUG_LVL 3

#if DEBUG_LVL > 0
#define debugstart(x) Serial.begin(x)
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debugstart(x)
#define debug(x)
#define debugln(x)
#endif


