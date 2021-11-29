# Alarm Clock With Stepper Motor

*This code requires 4 libraries listed below.  One of them is not a standard library and is included in the src\ folder and must be present when compiled.*
* Stepper.h
* Wire.h
* LiquidCrystal_I2C.h
* "src\Arduino-DS3231-master\DS3231.h"

The Arduino-DS3231-master library can be found here https://github.com/jarzebski/Arduino-DS3231

This code was created for use on a 16x2 lcd display and uses 4 buttons connected to pins 9-12 connected to gnd using INPUT_PULLUP.  It even handles debouncing of button presses.
The original project was for a 3D printed dispenser my son made that included the alarm clock to dispense his daily meds.  The stepper motor turns 180 degrees when the alarm is triggered.
the buttons are used as a Menu button, up and down button for setting Date, Time, Alarm Time, and Toggle Alarm on/off, and an OK/Enter button.
There is also an idle timeout if left on one of the screens for setting alarm or time for 1 minute without a button press to return to the clock.
