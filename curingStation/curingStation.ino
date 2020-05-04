/*

 UV Resin Curing Station Controller with Arduino
 v1.0

 This is the firmware of a DIY curing station for 3d prints 
 printed with an SLA/DLP/LCD printer using UV sensitive resin.
 It features:
 -LCD interface to set the timer (LCD Keypad Shield)
 -Controls UV 405nm LEDs to cure the model.
 -Controls the enclosure fan.
 -Uses a turn table with a gear motor to rotate the model.
 More info to build the enclosure can be found on my Github.
 
 Created April 2020 by Tolga Ozuygur

 http://github.com/tolgaozuygur

 */


#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

const long int timePresets[9] = {1800000,2700000,3600000,5400000,7200000,9000000,10800000,12600000,14400000};

const int BUZZER_PIN = 3;
const int UV_PIN = 12;
const int MOTOR_PIN = 11;

const int BUTTON_WAITING_RELEASE = -1;
const int BUTTON_UP = 0;
const int BUTTON_1 = 1;
const int BUTTON_2 = 2;
const int BUTTON_3 = 3;

const int SCREEN_MAIN = 0;
const int SCREEN_CURING = 1;
const int SCREEN_COMPLETED = 2;

const int MOTOR_MS_BETWEEN_TURNS = 10000;
const int MOTOR_PWM = 86; //increase if motor can't turn (0-255);

int currentTimePreset = 0;
int buttonPressed = BUTTON_UP;
int currentScreen = SCREEN_MAIN;
unsigned long curingStartMillis = 0;
unsigned long curingPreviousSecond = 0;
unsigned long motorPreviousTurnMillis = 0;

byte sub_per1[8] = {
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000
};

byte sub_per2[8] = {
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000
};

byte sub_per3[8] = {
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100
};

byte sub_per4[8] = {
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110
};

byte sub_per5[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte up_arrow[8] = {
  B00000,
  B00000,
  B00000,
  B00100,
  B01110,
  B11111,
  B00000,
  B00000
};

byte down_arrow[8] = {
  B00000,
  B00000,
  B00000,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000
};

byte time_icon[8] = {
  B11111,
  B10001,
  B01010,
  B00100,
  B01010,
  B10001,
  B11111,
  B00000
};


void setup() {
    Serial.begin(9600);
    pinMode(UV_PIN, OUTPUT);
    pinMode(MOTOR_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    lcd.createChar(0, sub_per1);
    lcd.createChar(1, sub_per2);
    lcd.createChar(2, sub_per3);
    lcd.createChar(3, sub_per4);
    lcd.createChar(4, sub_per5);
    lcd.createChar(5, up_arrow);
    lcd.createChar(6, down_arrow);
    lcd.createChar(7, time_icon);
    //
    lcd.setCursor(0,0);
    lcd.print("Cure-O-Matic");
    lcd.setCursor(0,1);
    lcd.print("V 1.0");
    tone(BUZZER_PIN, 400, 300);
    delay(300);
    tone(BUZZER_PIN, 500, 100);
    delay(3500);
    lcd.clear(); 
    //show main screen first
    updateMainScreen();
}

int tempPercentage = 0;

void loop() {
    handleButtons();
    //handle screens
    if(currentScreen == SCREEN_MAIN){
      if(buttonPressed == BUTTON_1){
        if(currentTimePreset > 0){
          currentTimePreset -= 1;
          updateMainScreen();
        }
      }
      if(buttonPressed == BUTTON_2){
        if(currentTimePreset < (sizeof(timePresets)/sizeof(timePresets[0]))-1){
          currentTimePreset += 1;
          updateMainScreen();
        }
      } 
      if(buttonPressed == BUTTON_3){
        //start curing
        tone(BUZZER_PIN, 400, 1500);
        lcd.clear(); 
        digitalWrite(UV_PIN, HIGH);
        curingStartMillis = millis();
        currentScreen = SCREEN_CURING;
      }       
    }
    if(currentScreen == SCREEN_CURING){
      unsigned long elapsedMillis = millis() - curingStartMillis;
      unsigned long remainingMillis = timePresets[currentTimePreset] - elapsedMillis;
      if(remainingMillis > 1000){
        if(curingPreviousSecond != (unsigned long)(remainingMillis/1000)){
          //second tick, update screen
          int _percentage = int(float(float(elapsedMillis) / float(timePresets[currentTimePreset]))*100);
          Serial.println(_percentage);
          updateCuringScreen(_percentage, millisToText(remainingMillis, true));
          curingPreviousSecond = (unsigned long)(remainingMillis/1000);
        }      
        if(motorPreviousTurnMillis < millis()){
          rotateTurnTable();
          motorPreviousTurnMillis = millis() + MOTOR_MS_BETWEEN_TURNS;
        }
      }else{
        //curing completed
        tone(BUZZER_PIN, 400, 2500);
        digitalWrite(UV_PIN, LOW);
        curingPreviousSecond = 0;
        updateCuringScreen(100, " ");
        lcd.setCursor(7, 0);  
        lcd.print("Completed");
        currentScreen = SCREEN_COMPLETED;
      }
    }
    if(currentScreen == SCREEN_COMPLETED){
      if(buttonPressed == BUTTON_3){
        currentScreen = SCREEN_MAIN;
        updateMainScreen();
      } 
    }
    delay(20);
}

void rotateTurnTable(){
  for (int i = MOTOR_PWM; i > 0; i--) {
    analogWrite(MOTOR_PIN, i);
    delay(1);
  }
  analogWrite(MOTOR_PIN, 0);  
}

String millisToText(unsigned long _millis, bool _showSeconds){
    int _hours = int(_millis / 3600000);
    _millis -= _hours*3600000;
    int _minutes = int(_millis / 60000);
    _millis -= _minutes*60000;  
    String millisText = "";
    if(_hours > 0){
      millisText += String(_hours) + "h";
    }
    if(_minutes > 0 || _hours > 0){
      millisText += String(_minutes) + "m";
    }
    if(_showSeconds){
      int _seconds = int(_millis / 1000);
       millisText += String(_seconds) + "s";
    }
    return millisText;
}

void handleButtons(){
    int buttonRead = analogRead (0);
    if(buttonPressed == BUTTON_UP){
      if (buttonRead < 60) {
        //3
        buttonPressed = BUTTON_3;
        tone(BUZZER_PIN, 900, 60);
      }
      else if (buttonRead < 600){
        //2
        buttonPressed = BUTTON_2;
        tone(BUZZER_PIN, 900, 60);
      }
      else if (buttonRead < 800){
        //1
        buttonPressed = BUTTON_1;
        tone(BUZZER_PIN, 900, 60);
      }
    }else{
      if (buttonRead < 800){
        //button is still pressed, wait till it get released to avoid multiple triggers
        buttonPressed = BUTTON_WAITING_RELEASE;
      }else{
        //button released
        buttonPressed = BUTTON_UP;
      }
    }
}

void updateMainScreen(){  
    lcd.clear();  
    lcd.setCursor(0, 0);  
    lcd.write(byte(7));
    lcd.print(": "); 
    lcd.print(millisToText(timePresets[currentTimePreset], false)); 
    lcd.setCursor(0, 1); 
    lcd.write(byte(6));
    lcd.setCursor(2, 1); 
    lcd.write(byte(5));
    lcd.setCursor(5, 1); 
    lcd.print("Start");
}

int curingTimeTextPreviousLength = 0;

void updateCuringScreen(int percentage, String remainingTimeText){    
    percentage = constrain(percentage, 0, 100);
    //percentage
    lcd.setCursor(0, 0);  
    lcd.print("%");
    lcd.print(percentage);   

    //time left    
    if(curingTimeTextPreviousLength != remainingTimeText.length()){
      lcd.setCursor(14-(curingTimeTextPreviousLength), 0);
      //text length changed, clear the time text cells on lcd
      for (int i2 = 0; i2 < curingTimeTextPreviousLength; i2++) {
        lcd.print(" ");
      }
      curingTimeTextPreviousLength = remainingTimeText.length();
    }
    //print the time text
    lcd.setCursor(14-(remainingTimeText.length()), 0); 
    lcd.write(byte(7)); 
    lcd.print(":");
    lcd.print(remainingTimeText);    

    //percentage bar
    int countSegment = 16;
    int countSubSegment = 6;
    int totalSubSegments = (countSegment*countSubSegment) - 2;
    int activeSegments = map(percentage, 0, 100, 0, totalSubSegments) + 1;
    for (int i = 0; i <= (int)(activeSegments/countSubSegment); i++) {
      lcd.setCursor(i, 1); 
      if(activeSegments > (i+1)*countSubSegment){
        //full segment
        lcd.write((byte)4); 
      }else{
        //sub segment
        int subSegmentCharCode = (activeSegments - (i*countSubSegment)) - 1;        
        if(subSegmentCharCode >= 0 && subSegmentCharCode < countSubSegment-1){          
          lcd.write(byte(subSegmentCharCode)); 
        }
      }
    }
}
