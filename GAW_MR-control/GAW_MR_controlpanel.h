
/* ------------------------------------------------------------------------ *
 *       Define controlPanel variables
 *         this is the control panel for the model railroad layout
 *         The buttons are handled in a 8 x 8 grid
 * ------------------------------------------------------------------------ */
char keys[ROWS][COLS] = {
  { 1,  2,  3,  4,  5,  6,  7,  8},         // Return values for each
  { 9, 10, 11, 12, 13, 14, 15, 16},         //  ROW/Column crossing
  {17, 18, 19, 20, 21, 22, 23, 24},         //   are pointers into the 
  {25, 26, 27, 28, 29, 30, 31, 32},         //    element array
  {33, 34, 35, 36, 37, 38, 39, 40},
  {41, 42, 43, 44, 45, 46, 47, 48},
  {49, 50, 51, 52, 53, 54, 55, 56},
  {57, 58, 59, 60, 61, 62, 63, 64}
};


/* ------------------------------------------------------------------------- *
 *                                       Pin numbers for each ROW and COLUMN
 * ------------------------------------------------------------------------- */
byte rowPins[ROWS] = {22, 23, 24, 25, 26, 27, 28, 29}; // row pins, key matrix
byte colPins[COLS] = {30, 31, 32, 33, 34, 35, 36, 37}; // column pins, key matrix


/* ------------------------------------------------------------------------- *
 *       Create objects for controlPanel
 * ------------------------------------------------------------------------- */
Keypad controlPanel = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);


/* ------------------------------------------------------------------------- *
 *                                 Global variables needed for Control Panel
 * ------------------------------------------------------------------------- */
int activeLoc = 0;


/* ------------------------------------------------------------------------- *
 *       Create objects with addres for the LCD screen
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C display(0x27,20,4);       // Initialize display


