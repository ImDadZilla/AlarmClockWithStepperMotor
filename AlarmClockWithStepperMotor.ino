#include <Stepper.h>
#include <Wire.h>
#include "src\Arduino-DS3231-master\DS3231.h"
#include <LiquidCrystal_I2C.h>

DS3231 clock;
RTCDateTime dt;
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display
RTCAlarmTime alarm1;

// Pin definitions
#define BUTTON_PIN1 9  // menu
#define BUTTON_PIN2 10 // up
#define BUTTON_PIN3 11 // down
#define BUTTON_PIN4 12 // ok
#define buzzer 2        //**************Alarm***************//
// ---------------------------------


// ******************************
// BUTTON CLASS
class button {    // this class handles button presses and debouncing
  public:
  const uint8_t pin;
  boolean triggered;

  button(uint8_t p) : pin(p){}

  void press(boolean state) {
    if(state == pressed || (millis() - lastPressed <= debounceTime)){
      return;  // pressed too quick, debounce, do nothing
    }
    lastPressed = millis();
    if(state) {
      // button pressed
    } else {
      // button released
      triggered = true;
    }
    pressed = state;
  }

  void update() {
    press(!digitalRead(pin));
  }

  void clear() {
    triggered = false;
  }

  private:
  const long debounceTime = 30;
  unsigned long lastPressed;
  boolean pressed = 0;
};
// *************************************


//************Variables**************//
int alarm1Hour = 0;
int alarm1Minute = 0;
int menu = 0;
unsigned long menuTime;
const long menuIdleTime = 60000;
String currentScreen;
const int stepsPerRevolution = 2038;  // change this to fit the number of steps per revolution
const int stepsPerHalfRevolution = 1019;
Stepper myStepper(stepsPerRevolution, 4,6,5,7);                 // initialize the stepper library on pins 4-7 (NOTE: The order of 4,6,5,7 is on purpose for the stepper motor to work)
button buttons[] = {
  {BUTTON_PIN1},
  {BUTTON_PIN2},
  {BUTTON_PIN3},
  {BUTTON_PIN4},
};
const uint8_t NumButtons = sizeof(buttons) / sizeof(button);    // constant number of buttons in the array of button


void setup() {
  // FAILSAFE!  This checks if pin1 is grounded and goes into endless loop.
  // just incase something goes wrong we can still upload to the arduino
  pinMode(1,INPUT_PULLUP);
  if(!digitalRead(1)){
    for(;;){}
  }                                             // end failsafe
  
  // set buttons array
  for(int i = 0; i < NumButtons; i++){
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }

  pinMode(buzzer, OUTPUT);                      // Set buzzer as an output
  myStepper.setSpeed(10);                       // set the speed at 60 rpm:                   
  
  Serial.begin(9600);                           // open the Serial port for monitoring
  clock.begin();                                // start the clock
  clock.setDateTime(__DATE__, __TIME__);        // set the clock to current system time

  lcd.init();                                   // initialize the lcd
  lcd.backlight();                              // turn on lcd backlight

  RTCAlarmTime alarm1 = clock.getAlarm1();                                    // get the alarm1 time
  alarm1Hour = alarm1.hour;                                                   // get the alarm1 hour
  alarm1Minute = alarm1.minute;                                               // get the alarm1 minute
  clock.setAlarm1(0, alarm1Hour, alarm1Minute, 0, DS3231_MATCH_H_M_S);        // set the alarm
}

void loop() {
  for(int i = 0; i < NumButtons; i++) {
    buttons[i].update();                        // detect button press
    if(buttons[i].triggered) {                  // button was pressed and released and has triggered.
      menuTime = millis();
      buttons[i].triggered = false;             // set triggered on this button back to false to detect it being pressed again.
      switch (i) {                              // which button was pressed?
        case 0:                                 // "menu" button pressed
          menu = menu + 1;
          if(menu > 2) {
            menu = 0;
          }
          lcd.clear();
          switch (menu) {                        // toggle through menu's
            case 0:
              displayDateTime();
              break;

            case 1:
              displayAlarm1();
              break;

            case 2: 
              displayAlarmStatus();
              break;
          }
          break;                                                                // end of menu button pressed case

        case 1:                                                                 // up button pressed
          switch (menu) {
            case 1:                                                             // up button pressed while on menu 1, alarm display &&
              if(currentScreen == "displayAlarm1Hour") {                        // on displayAlarm1Hour screen
                incrementAlarm1Hour(alarm1Hour);
                lcd.clear();
                displayAlarm1Hour();
              } else if (currentScreen == "displayAlarm1Minute") {              // on displayAlarm1Minute screen
                incrementAlarm1Minute(alarm1Minute);
                lcd.clear();
                displayAlarm1Minute();
              }
              break;
              
            case 2:                                                           // down button while on menu 2, alarm status
              if (currentScreen == "displayAlarmStatus") {
                toggleAlarmStatus();
                lcd.clear();
                displayAlarmStatus();
              }
              break;
          }
          break; // end of up button pressed case.

        case 2: // down button pressed
          switch(menu) {
            case 1:                                                           // down button press while on menu 1, alarm display && 
              if(currentScreen == "displayAlarm1Hour") {                      // on displayAlarm1Hour screen
                  decrementAlarm1Hour(alarm1Hour);
                  lcd.clear();
                  displayAlarm1Hour();
              } else if(currentScreen == "displayAlarm1Minute") {             // on displayAlarm1Minute screen
                  decrementAlarm1Minute(alarm1Minute);
                  lcd.clear();
                  displayAlarm1Minute();
              }
              break; 

            case 2:                                                           // down button while on menu 2, alarm status
              if (currentScreen == "displayAlarmStatus") {
                toggleAlarmStatus();
                lcd.clear();
                displayAlarmStatus();
              }
              break;
          }
          break;                                                              // end of down button pressed case
        
        case 3:                                                               // OK button pressed.
          switch (menu) {
            case 1:                                                           // alarm menu
              if(currentScreen == "displayAlarm1") {
                lcd.clear();
                displayAlarm1Hour();
              } else if (currentScreen == "displayAlarm1Hour") {
                lcd.clear();
                displayAlarm1Minute();
              } else if (currentScreen == "displayAlarm1Minute") {
                lcd.clear();
                displayAlarm1();
              }
              break; 
          }                                                                 // end OK button pressed case
      }                                                                     // end of which button pressed
    }                                                                       // end of if statement on a button triggered.
  }                                                                         // end for loop of all buttons to check for input.
  
  if (menu == 0) {  // if the menu variable is 0, display current date and time
    displayDateTime(); 
  }
  
  if(clock.isAlarm1()){                                                     // if alarm1 H and S match current time, trigger the alarm and do these things
    alarm();
  }

  if (isMenuIdle()) {
    menu = 0;
  }
  
}  // end loop

void displayDateTime() {
  currentScreen = "displayDateTime";
  dt = clock.getDateTime();
  lcd.setCursor(2, 0);
  lcd.print(clock.dateFormat("M jS,Y", dt));
  lcd.setCursor(3, 1);
  lcd.print(clock.dateFormat("h:i:s A",dt));
}

void displayAlarm1() {
  currentScreen = "displayAlarm1";
  alarm1 = clock.getAlarm1();
  lcd.setCursor(4, 0);
  lcd.print("Alarm 1");
  lcd.setCursor(4, 1);
  lcd.print(clock.dateFormat("h:i A", alarm1));
}

void displayAlarmStatus() {
  currentScreen = "displayAlarmStatus";
  bool alarm1On = clock.isArmed1();
  lcd.setCursor(0,0);
  if(alarm1On) {
    lcd.print("Alarm is ON");
  } else {
    lcd.print("Alarm is OFF");
  }
}

void toggleAlarmStatus() {
  bool alarm1On = clock.isArmed1();
  if(alarm1On) {
    clock.armAlarm1(false);
  } else {
    clock.armAlarm1(true);
  }
}

void displayAlarm1Hour() {
  currentScreen = "displayAlarm1Hour";
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Set Alarm Hour");
  RTCAlarmTime alarm1;
  alarm1 = clock.getAlarm1();
  lcd.setCursor(4,1);   lcd.print(clock.dateFormat("h:i A",alarm1));
}

void displayAlarm1Minute() {
  currentScreen = "displayAlarm1Minute";
  lcd.clear();
  lcd.setCursor(0,0);  lcd.print("Set Alarm Minutes");
  RTCAlarmTime alarm1;
  alarm1 = clock.getAlarm1();
  lcd.setCursor(4,1);   lcd.print(clock.dateFormat("h:i A",alarm1));
}

void incrementAlarm1Hour(int value) {
  alarm1Hour = incrementHour(value);
  clock.setAlarm1(0,alarm1Hour,alarm1Minute,0,DS3231_MATCH_H_M_S);
}

void decrementAlarm1Hour(int value) {
  alarm1Hour = decrementHour(value);
  clock.setAlarm1(0,alarm1Hour,alarm1Minute,0,DS3231_MATCH_H_M_S);
}

void incrementAlarm1Minute(int value) {
  alarm1Minute = incrementMinute(value);
  clock.setAlarm1(0,alarm1Hour,alarm1Minute,0,DS3231_MATCH_H_M_S);
}

void decrementAlarm1Minute(int value) {
  alarm1Minute = decrementMinute(value);
  clock.setAlarm1(0,alarm1Hour,alarm1Minute,0,DS3231_MATCH_H_M_S);
}

int incrementHour(int value) {
  int returnValue;
  if(value == 23) {
    returnValue = 0;
  } else {
    returnValue = value + 1;
  }
  return returnValue;
}

int decrementHour(int value) {
  int returnValue;
  if(value == 0) {
    returnValue = 23;
  } else {
    returnValue = value - 1;
  }
  return returnValue;
}

int incrementMinute(int value) {
  int returnValue;
  if(value == 59) {
    returnValue = 0;
  } else {
    returnValue = value + 1;
  }
  return returnValue;
}

int decrementMinute(int value) {
  int returnValue;
  if(value == 0) {
    returnValue = 59;
  } else {
    returnValue = value - 1;
  }
  return returnValue;
}

void alarm() {
  tone(buzzer,880);
  delay(300);
  tone(buzzer,698);
  delay(300);
  noTone(buzzer);
  turnMotor();
}

void turnMotor() {
  turnMotorOn();
  myStepper.step(stepsPerHalfRevolution);
  turnMotorOff();
}

void turnMotorOff() {
  digitalWrite(4,LOW);
  digitalWrite(5,LOW);
  digitalWrite(6,LOW);
  digitalWrite(7,LOW);
}

void turnMotorOn() {
  digitalWrite(4,HIGH);
  digitalWrite(5,HIGH);
  digitalWrite(6,HIGH);
  digitalWrite(7,HIGH);
}

bool isMenuIdle() {
  unsigned long nowMillis = millis();
  if((nowMillis - menuTime) > menuIdleTime) {
    return true;
  }
  return false;
}
