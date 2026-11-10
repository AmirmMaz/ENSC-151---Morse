#include <Wire.h>
#include "rgb_lcd.h"

//Variables for hardware
const int BUTTON_PIN = 2;
const int TOUCH_PIN = 4;
const int BUZZER_PIN = 6;
const int KNOB_PIN = A0;

const int MODE_SEND = 0;
const int MODE_RECEIVE = 1;

const int KNOB_MIN = 0;
const int KNOB_MAX = 1023;

const int MIN_WPM = 5;
const int MAX_WPM = 20;
int mode = MODE_RECEIVE;
int previousTouch = LOW;

//Timing variables
int wpm = MIN_WPM;
int previousWpm = -1;
unsigned long unitMs = 1200 / MIN_WPM;

//For lcd
char textRow[17]; //spot for \0
rgb_lcd lcd;

//Variables for Morse -> Text
int lastButtonState = LOW; //Changed from HIGH -> LOW
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;
String currentMorse = "";
bool wordSpacePrinted = true; // Prevents endless spaces when idle


//Hardware functions
void updateSpeed();
void checkTouchMode();
void drawStatusRow();
void clearTextRow();
void showChar(char c);

//Morse -> English functions
char translateMorse(String phrase);
void decodeButton();
void resetDecoder();

//English -> Morse functions
void playIncomingMorse();

void setup() {

  pinMode(BUTTON_PIN, INPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.begin(16,2);
  Serial.begin(9600);
  drawStatusRow();

  clearTextRow();

}

void loop() {
  checkTouchMode();
  updateSpeed();

  if(mode == MODE_RECEIVE){
    decodeButton();

  } else{
    playIncomingMorse();
  }
}

void checkTouchMode(){
  int currentTouch = digitalRead(TOUCH_PIN);
  //Changes the mode between receive and send with touch sensor
  if(previousTouch == LOW && currentTouch == HIGH){
    if(mode==MODE_RECEIVE){
      mode = MODE_SEND;
      clearTextRow();
      resetDecoder();
      drawStatusRow();
    } else{
      mode = MODE_RECEIVE;
      clearTextRow();
      resetDecoder();
      drawStatusRow();
    }
  }
  previousTouch = currentTouch;
}

void updateSpeed(){
  int knobValue = analogRead(KNOB_PIN);
  wpm = map(knobValue, KNOB_MIN, KNOB_MAX, MIN_WPM, MAX_WPM);
  wpm = constrain(wpm, MIN_WPM, MAX_WPM);
  unitMs = 1200 / wpm;
  if(wpm != previousWpm){
    drawStatusRow();
  }
  previousWpm = wpm;
}

void drawStatusRow(){
  if(mode == MODE_RECEIVE){
    lcd.setRGB(0, 255, 0);
    lcd.setCursor(0, 0);
    lcd.print("RECEIVE");

  } else if(mode == MODE_SEND){
    lcd.setRGB(0, 0, 255);
    lcd.setCursor(0, 0);
    lcd.print("SEND   ");

  }
  
  if(wpm<10){
    lcd.print("  ");
    lcd.print(wpm);
    lcd.print(" WPM");

  } else{
    lcd.print(" ");
    lcd.print(wpm);
    lcd.print(" WPM");

  }
  
}

void clearTextRow(){
  for (int i = 0; i < 16; i++){
    textRow[i] = ' ';
  }
  textRow[16] = '\0';
  lcd.setCursor(0, 1);
  lcd.print(textRow);
}

void showChar(char c){
  for(int i = 0; i < 15; i++){
    textRow[i] = textRow[i+1];
  }
  textRow[15] = c;
  lcd.setCursor(0, 1);
  lcd.print(textRow);
}

char translateMorse(String phrase){
  if (phrase == ".-") { return 'A'; }
  else if (phrase == "-...") { return 'B'; }
  else if (phrase == "-.-.") { return 'C'; }
  else if (phrase == "-..") { return 'D'; }
  else if (phrase == ".") { return 'E'; }
  else if (phrase == "..-.") { return 'F'; }
  else if (phrase == "--.") { return 'G'; }
  else if (phrase == "....") { return 'H'; }
  else if (phrase == "..") { return 'I'; }
  else if (phrase == ".---") { return 'J'; }
  else if (phrase == "-.-") { return 'K'; }
  else if (phrase == ".-..") { return 'L'; }
  else if (phrase == "--") { return 'M'; }
  else if (phrase == "-.") { return 'N'; }
  else if (phrase == "---") { return 'O'; }
  else if (phrase == ".--.") { return 'P'; }
  else if (phrase == "--.-") { return 'Q'; }
  else if (phrase == ".-.") { return 'R'; }
  else if (phrase == "...") { return 'S'; }
  else if (phrase == "-") { return 'T'; }
  else if (phrase == "..-") { return 'U'; }
  else if (phrase == "...-") { return 'V'; }
  else if (phrase == ".--") { return 'W'; }
  else if (phrase == "-..-") { return 'X'; }
  else if (phrase == "-.--") { return 'Y'; }
  else if (phrase == "--..") { return 'Z'; }
  else { return '?'; }
}

void decodeButton() {
  int currentState = digitalRead(BUTTON_PIN);
  unsigned long currentTime = millis();

  //Thresholds that relate back to the speed knob
  unsigned long dotThreshold = 2 * unitMs; //short vs long press for dot/dash
  unsigned long letterThreshold = 2 * unitMs; //pausing this long ends a letter
  unsigned long wordThreshold = 5 * unitMs; //pause this long ends word

  if (lastButtonState == LOW && currentState == HIGH) {
    pressedTime = currentTime; 
    delay(50); 
 }

else if (lastButtonState == HIGH && currentState == LOW) {
  unsigned long pressDuration = currentTime - pressedTime;
  releasedTime = currentTime;
  wordSpacePrinted = false;
  if (pressDuration > 20 && pressDuration <= dotThreshold) {
    currentMorse += ".";
  } else if (pressDuration > dotThreshold) {
    currentMorse += "-";
  }
  delay(50);
  }

 if (currentState == LOW) {
    if (currentMorse.length() > 0 && currentTime - releasedTime > letterThreshold) {
      if (currentMorse == ".....") {
        clearTextRow();              // five dots = clear message
        wordSpacePrinted = true;     // no space after a clear
      } else {
        showChar(translateMorse(currentMorse));
      }
      currentMorse = "";
    }
    else if (currentMorse.length() == 0 && !wordSpacePrinted
             && currentTime - releasedTime > wordThreshold) {
      showChar(' ');
      wordSpacePrinted = true;
    }
  }
  lastButtonState = currentState;
  }
}

void resetDecoder(){
  currentMorse = ""; //drops half tapped letter
  wordSpacePrinted = true;
  lastButtonState = digitalRead(BUTTON_PIN);

}

void playIncomingMorse(){
  //to do
}
