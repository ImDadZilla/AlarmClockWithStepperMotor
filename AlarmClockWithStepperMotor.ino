#include <Stepper.h>
#include <Wire.h>
#include "src\Arduino-DS3231-master\DS3231.h"
#include <LiquidCrystal_I2C.h>

DS3231 clock;
LiquidCrystal_I2C lcd(0x27, 16, 2); 
RTCAlarmTime alarm1;

// Pin definitions
#define BUTTON_PIN1 9  // menu
#define BUTTON_PIN2 10 // up
#define BUTTON_PIN3 11 // down
#define BUTTON_PIN4 12 // ok
#define buzzer 2        //**************Alarm***************//
// ---------------------------------
// compiler constants
#define menuIdleTime (long)60000
#define stepsPerRevolution (int16_t)2038  // change this to fit the number of steps per revolution


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
uint8_t menu = 0;
unsigned long menuTime;
String currentScreen;
Stepper myStepper(stepsPerRevolution, 4,6,5,7);                 // initialize the stepper library on pins 4-7 (NOTE: The order of 4,6,5,7 is on purpose for the stepper motor to work)
button buttons[] = {
  {BUTTON_PIN1},
  {BUTTON_PIN2},
  {BUTTON_PIN3},
  {BUTTON_PIN4},
};
const uint8_t NumButtons = sizeof(buttons) / sizeof(button);    // constant number of buttons in the array of button
bool testMode = false;

void setup() {
  // FAILSAFE!  This checks if pin1 is grounded and goes into endless loop.
  // just incase something goes wrong we can still upload to the arduino
  pinMode(1,INPUT_PULLUP);
  if(!digitalRead(1)){
    for(;;){}
  }                                             // end failsafe
  
  // set buttons array
  for(uint8_t i = 0; i < NumButtons; i++){
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }

  pinMode(buzzer, OUTPUT);                      // Set buzzer as an output
  myStepper.setSpeed(10);                       // set the speed at 60 rpm:                   
  
  Serial.begin(9600);                           // open the Serial port for monitoring
  clock.begin();                               // start the clock

  lcd.init();                                   // initialize the lcd
  lcd.backlight();                              // turn on lcd backlight

  RTCAlarmTime alarm1 = clock.getAlarm1();                                    // get the alarm1 time
  clock.setAlarm1(0, alarm1.hour, alarm1.minute, 0, DS3231_MATCH_H_M_S);        // set the alarm
}

void loop() {
  for(uint8_t i = 0; i < NumButtons; i++) {
    buttons[i].update();                        // detect button press
    if(buttons[i].triggered) {                  // button was pressed and released and has triggered.
      menuTime = millis();
      buttons[i].triggered = false;             // set triggered on this button back to false to detect it being pressed again.
      switch (i) {                              // which button was pressed?
        case 0:                                 // "menu" button pressed
          menu = menu + 1;
          if(menu > 4) {
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

            case 3:
              changeDateTime();
              break;

            case 4:
              displayTestMode();
              break;
          }
          break;                                                                // end of menu button pressed case

        case 1:                                                                 // up button pressed
          switch (menu) {
            case 1:                                                             // up button pressed while on menu 1, alarm display &&
              if(currentScreen == "displayAlarm1Hour") {                        // on displayAlarm1Hour screen
                RTCAlarmTime al = clock.getAlarm1();
                incrementAlarm1Hour(al);
                lcd.clear();
                displayAlarm1Hour();
              } else if (currentScreen == "displayAlarm1Minute") {              // on displayAlarm1Minute screen
                RTCAlarmTime al = clock.getAlarm1();
                incrementAlarm1Minute(al);
                lcd.clear();
                displayAlarm1Minute();
              }
              break;
              
            case 2:                                                           // up button while on menu 2, alarm status
              if (currentScreen == "displayAlarmStatus") {
                toggleAlarmStatus();
                lcd.clear();
                displayAlarmStatus();
              }
              break;

            case 3:
              if(currentScreen == "displaySetTimeHour") {
                lcd.clear();
                incrementTimeHour();
                displaySetTimeHour();
              } else if (currentScreen == "displaySetTimeMinute") {
                lcd.clear();
                incrementTimeMinute();
                displaySetTimeMinute();
              } else if (currentScreen == "displaySetDateMonth") {
                lcd.clear();
                incrementDateMonth();
                displaySetDateMonth();
              } else if(currentScreen == "displaySetDateDay") {
                lcd.clear();
                incrementDateDay();
                displaySetDateDay();
              } else if (currentScreen == "displaySetDateYear") {
                lcd.clear();
                incrementDateYear();
                displaySetDateYear();
              }
          }
          break; // end of up button pressed case.

        case 2: // down button pressed
          switch(menu) {
            case 1:                                                           // down button press while on menu 1, alarm display && 
              if(currentScreen == "displayAlarm1Hour") {                      // on displayAlarm1Hour screen
                  RTCAlarmTime al = clock.getAlarm1();
                  decrementAlarm1Hour(al);
                  lcd.clear();
                  displayAlarm1Hour();
              } else if(currentScreen == "displayAlarm1Minute") {             // on displayAlarm1Minute screen
                  RTCAlarmTime al = clock.getAlarm1();
                  decrementAlarm1Minute(al);
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

            case 3:                                                           // down button while on menu 3 time set
              if(currentScreen == "displaySetTimeHour") {
                lcd.clear();
                decrementTimeHour();
                displaySetTimeHour();
              } else if (currentScreen == "displaySetTimeMinute") {
                lcd.clear();
                decrementTimeMinute();
                displaySetTimeMinute();
              } else if (currentScreen == "displaySetDateMonth") {
                lcd.clear();
                decrementDateMonth();
                displaySetDateMonth();
              } else if(currentScreen == "displaySetDateDay") {
                lcd.clear();
                decrementDateDay();
                displaySetDateDay();
              } else if (currentScreen == "displaySetDateYear") {
                lcd.clear();
                decrementDateYear();
                displaySetDateYear();
              }
              break;

            default:
              if(testMode) {
                alarm();
              }
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

            case 3:
              if (currentScreen == "changeDateTime") {
                lcd.clear();
                displaySetTimeHour();
              } else if (currentScreen == "displaySetTimeHour") {
                lcd.clear();
                displaySetTimeMinute();
              } else if (currentScreen == "displaySetTimeMinute") {
                lcd.clear();
                displaySetDateMonth();
              } else if (currentScreen == "displaySetDateMonth") {
                lcd.clear();
                displaySetDateDay();
              } else if (currentScreen == "displaySetDateDay") {
                lcd.clear();
                displaySetDateYear();
              } else if (currentScreen == "displaySetDateYear") {
                lcd.clear();
                changeDateTime();
              }
              break;

            case 4:
              if (currentScreen == "displayTestMode") {
                lcd.clear();
                toggleTestMode();
                displayTestMode();
              }
          }                                                                 // end OK button pressed case
      }                                                                     // end of which button pressed
    }                                                                       // end of if statement on a button triggered.
  }                                                                         // end for loop of all buttons to check for input.
  
  if (menu == 0) {  // if the menu variable is 0, display current date and time
    displayDateTime();
    Serial.println(__TIME__);
    Serial.println(__DATE__);
  }
  
  if(clock.isAlarm1()){                                                     // if alarm1 H and S match current time, trigger the alarm and do these things
    alarm();
  }

  if (isMenuIdle()) {                                                       // if left on a menu other then clock for over 1 minute, change to clock
    if(menu != 0) {
      lcd.clear();
      menu = 0;
    }
  }
  
}  // end loop

void displayDateTime() {
  currentScreen = "displayDateTime";
  RTCDateTime dt;
  dt = clock.getDateTime();
  lcd.setCursor(1, 0);
  lcd.print(clock.dateFormat("M jS,Y", dt));
  lcd.setCursor(2, 1);
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
  lcd.setCursor(5,1); lcd.cursor();
}

void displayAlarm1Minute() {
  currentScreen = "displayAlarm1Minute";
  lcd.clear();
  lcd.setCursor(0,0);  lcd.print("Set Alarm Minutes");
  RTCAlarmTime alarm1;
  alarm1 = clock.getAlarm1();
  lcd.setCursor(4,1);   lcd.print(clock.dateFormat("h:i A",alarm1));
  lcd.setCursor(8,1); lcd.cursor();
}

void displaySetTimeHour() {
  currentScreen = "displaySetTimeHour";
  lcd.clear(); lcd.home();
  lcd.print("Set Clock Hour");
  lcd.setCursor(0, 1);  lcd.print(clock.dateFormat("h:i A",clock.getDateTime())); 
  lcd.setCursor(2,1); lcd.cursor();
}

void displaySetTimeMinute() {
  currentScreen = "displaySetTimeMinute";
  lcd.clear(); lcd.home();
  lcd.print("Set Clock Minutes");
  lcd.setCursor(0, 1);  lcd.print(clock.dateFormat("h:i A",clock.getDateTime())); 
  lcd.setCursor(5,1); lcd.cursor();
}

void displaySetDateMonth() {
  currentScreen = "displaySetDateMonth";
  lcd.clear(); lcd.home();
  lcd.print("Set Date Month");
  lcd.setCursor(0, 1);
  lcd.print(clock.dateFormat("M jS,Y", clock.getDateTime()));  
  lcd.setCursor(2,1); lcd.cursor();
}

void displaySetDateDay() {
  currentScreen = "displaySetDateDay";
  lcd.clear(); lcd.home();
  lcd.print("Set Date Day");
  lcd.setCursor(0, 1);
  lcd.print(clock.dateFormat("M jS,Y", clock.getDateTime())); 
  lcd.setCursor(5,1); lcd.cursor(); 
}

void displaySetDateYear() {
  currentScreen = "displaySetDateYear";
  lcd.clear(); lcd.home();
  lcd.print("Set Date Year");
  lcd.setCursor(0, 1);
  lcd.print(clock.dateFormat("M jS,Y", clock.getDateTime()));  
  lcd.setCursor(12,1); lcd.cursor();
}

void displayTestMode() {
  currentScreen = "displayTestMode";
  lcd.clear(); lcd.home();
  lcd.print("Test Mode");
  lcd.setCursor(0, 1);
  String testModeString;
  if(testMode) {
    testModeString = "ON";
  } else {
    testModeString = "OFF";
  }
  lcd.print(testModeString);
}

void toggleTestMode() {
  if(testMode){
    testMode = false;
  } else {
    testMode = true;
  }
}

void changeDateTime() {
  currentScreen = "changeDateTime";
  lcd.clear();
  lcd.home(); lcd.print("Change Date &");
  lcd.setCursor(0,1); lcd.print("Time?");
}

void incrementTimeHour() {
  RTCDateTime now = clock.getDateTime();
  clock.setDateTime(now.year,now.month,now.day,incrementHour(now.hour),now.minute,now.second);
}

void decrementTimeHour(){
  RTCDateTime now = clock.getDateTime();
  clock.setDateTime(now.year,now.month,now.day,decrementHour(now.hour),now.minute,now.second);
}

void incrementTimeMinute() {
  RTCDateTime now = clock.getDateTime();
  clock.setDateTime(now.year,now.month,now.day,now.hour,incrementMinute(now.minute),now.second);
}

void decrementTimeMinute() {
  RTCDateTime now = clock.getDateTime();
  clock.setDateTime(now.year,now.month,now.day,now.hour,decrementMinute(now.minute),now.second);
}

void incrementDateMonth() {
  RTCDateTime now = clock.getDateTime();
  clock.setDateTime(now.year,incrementMonth(now.month),now.day,now.hour,now.minute,now.second);
}

void decrementDateMonth() {
  RTCDateTime now = clock.getDateTime();
  clock.setDateTime(now.year,decrementMonth(now.month),now.day,now.hour,now.minute,now.second);
}

void incrementDateDay() {
  RTCDateTime now = clock.getDateTime();
  clock.setDateTime(now.year,now.month,incrementDay(now.day),now.hour,now.minute,now.second);
}

void decrementDateDay() {
  RTCDateTime now = clock.getDateTime();
  clock.setDateTime(now.year,now.month,decrementDay(now.day),now.hour,now.minute,now.second);
}

void incrementDateYear() {
  RTCDateTime now = clock.getDateTime();
  clock.setDateTime(incrementYear(now.year),now.month,now.day,now.hour,now.minute,now.second);
}

void decrementDateYear() {
  RTCDateTime now = clock.getDateTime();
  clock.setDateTime(decrementYear(now.year),now.month,now.day,now.hour,now.minute,now.second);
}

void incrementAlarm1Hour(RTCAlarmTime al) {
  clock.setAlarm1(0,incrementHour(al.hour),al.minute,0,DS3231_MATCH_H_M_S);
}

void decrementAlarm1Hour(RTCAlarmTime al) {
  clock.setAlarm1(0,decrementHour(al.hour),al.minute,0,DS3231_MATCH_H_M_S);
}

void incrementAlarm1Minute(RTCAlarmTime al) {
  clock.setAlarm1(0,al.hour,incrementMinute(al.minute),0,DS3231_MATCH_H_M_S);
}

void decrementAlarm1Minute(RTCAlarmTime al) {
  clock.setAlarm1(0,al.hour,decrementMinute(al.minute),0,DS3231_MATCH_H_M_S);
}

uint8_t incrementHour(uint8_t value) {
  uint8_t returnValue;
  if(value == 23) {
    returnValue = 0;
  } else {
    returnValue = value + 1;
  }
  return returnValue;
}

uint8_t decrementHour(uint8_t value) {
  uint8_t returnValue;
  if(value == 0) {
    returnValue = 23;
  } else {
    returnValue = value - 1;
  }
  return returnValue;
}

uint8_t incrementMinute(uint8_t value) {
  uint8_t returnValue;
  if(value == 59) {
    returnValue = 0;
  } else {
    returnValue = value + 1;
  }
  return returnValue;
}

uint8_t decrementMinute(uint8_t value) {
  uint8_t returnValue;
  if(value == 0) {
    returnValue = 59;
  } else {
    returnValue = value - 1;
  }
  return returnValue;
}

uint8_t incrementMonth(uint8_t value) {
  uint8_t returnValue;
  if(value == 12) {
    returnValue = 1;
  } else {
    returnValue = value + 1;
  }
  return returnValue;
}

uint8_t decrementMonth(uint8_t value) {
  uint8_t returnValue;
  if(value == 1) {
    returnValue = 12;
  } else {
    returnValue = value - 1;
  }
  return returnValue;
}

uint8_t incrementDay(uint8_t value) {
  uint8_t returnValue;
  if(value == 31) {
    returnValue = 1;
  } else {
    returnValue = value + 1;
  }
  return returnValue;
}

uint8_t decrementDay(uint8_t value) {
  uint8_t returnValue;
  if(value == 1) {
    returnValue = 31;
  } else {
    returnValue = value - 1;
  }
  return returnValue;
}

uint16_t incrementYear(uint16_t value) {
  uint16_t returnValue;
  if(value == 2099) {
    returnValue = 2099;
  } else {
    returnValue = value + 1;
  }
  return returnValue;
}

uint16_t decrementYear(uint16_t value) {
  uint16_t returnValue;
  if(value == 2021) {
    returnValue = 2021;
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
  myStepper.step(stepsPerRevolution / 2);
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
