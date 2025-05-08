# GAW_MR_Control
## Description
This program is used to service a control panel to operate turnouts on my layout. In the future I will try to add locomotive control to the same panel. Also my Z21 command center's power can be switched on and off. The state of power and turnouts can be stored to and retrieved from EEPROM between sessions.

## Hardware, connections
The program is used with an Arduino Mega 2560 R3, because it has a large number of pins. 

For the momentary push buttons on the panel I use the 'keypad' library to define a 'keyboard' of rows and columns. That way I can use (for example) 16 pins for 64 buttons in a matrix.

An I2C LCD display (20 x 4) screen is used for visible output.

Communication to and from the command station will take place through the Loconet protocol.
