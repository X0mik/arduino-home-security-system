#include <Keypad.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Servo.h>
//#include <SoftwareSerial.h>

//SoftwareSerial bluetooth(47, 46);
LiquidCrystal display(22, 23, 39, 37, 35, 33, 31, 29, 27, 25);
Servo servo;

const int pin_red = 10;
const int pin_green = 11;
const int pin_blue = 12;
const int rows = 4;
const int cols = 4;

const int pin_pirP = 20;
const int pin_pirB = 21;

volatile bool motionDetected = false;

char keys[rows][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'},
};

byte col_pins[] = {2, 3, 4, 5};
byte row_pins[] = {6, 7, 8, 9};  

Keypad keypad = Keypad(makeKeymap(keys), row_pins, col_pins, rows, cols);

unsigned long time;
unsigned long previousTime = 0;

bool alarmLedOn = true;
bool newPin = false;
bool correctPass;

String pass = "";
String encrPass = "";
String btText = "";

enum State{
  ZAMCENO,
  ODEMCENO,
  POPLACH
};

State stav = State::ODEMCENO;

void setup()
{
  display.begin(16,2);
  servo.attach(13);
  pinMode(45, OUTPUT); // buzzer
  pinMode(pin_red, OUTPUT);
  pinMode(pin_green, OUTPUT);
  pinMode(pin_blue, OUTPUT);
  Serial.begin(9600);
  Serial3.begin(9600);
  setState(State::ODEMCENO);

  pinMode(pin_pirP, INPUT);
  pinMode(pin_pirB, INPUT);
}

void loop()
{
  time = millis();
  char key = keypad.getKey();

  if(key){
    Serial.println(key);
    if(stav != State::POPLACH){
      if(isDigit(key) && pass.length() < 4){
        tone(45, 2000, 50);
      }
      else if(isDigit(key) && pass.length() >= 4){
        tone(45, 500, 150);
      }

    }
  }
  if(key == 'B'){
    setState(State::ODEMCENO);
  }
  else if(key == 'C'){
    setState(State::ZAMCENO);
  }
  else if(key == 'D'){
    setState(State::POPLACH);
  }

  if(isDigit(key) && pass.length() < 4){
    pass += key;
    clearRow(1);
    //if(pass != ""){
      display.setCursor(0,1);
      encrPass = "";
      for(int i = 0; i < pass.length(); i++){
        encrPass += "*";
      }
      if(newPin){
        display.print("NOVY PIN: " + encrPass);
      }
      else{
        display.print("PIN: " + encrPass);
      }
    //}
  }

  if (pass.length() == 4 && key == '#'){
    clearRow(1);
    if(newPin){
      tone(45, 2500, 100);
      for(int i = 0; i < pass.length(); i++){
        EEPROM.write(i, pass[i]);
      }
      newPin = false;
      display.setCursor(0,1);
      display.print("PIN ULOZEN");
    }
    else{
      for(int i = 0; i < pass.length(); i++){
        correctPass = true;
        if(pass[i] != EEPROM.read(i)){
          /*display.setCursor(0,1);
          display.print("SPATNE HESLO!");
          setState(State::POPLACH);*/
          correctPass = false;
          break;
        }
      }
      if(correctPass){
        display.setCursor(0,1);
        display.print("SPRAVNE HESLO");
        if(stav == State::ODEMCENO){
          setState(State::ZAMCENO);
        }
        else if(stav == State::ZAMCENO){
          setState(State::ODEMCENO);
        }
        else if(stav == State::POPLACH){
          setState(State::ODEMCENO);
        }
        tone(45, 2500, 100);
      }
      else{
        tone(45, 500, 150);
        clearRow(1);
        display.setCursor(0,1);
        display.print("SPATNE HESLO!");
        //setState(State::POPLACH);
      }
    }
    pass = "";
    delay(2000);
    clearRow(1);
  }

  if (key == 'A' && stav == State::ODEMCENO){
    clearRow(1);
    if(newPin){
      newPin = false;
    }
    else{
      newPin = true;
      display.setCursor(0,1);
      display.print("NOVY PIN: ");
    }
  }

  if(stav == State::POPLACH){
    if(time - previousTime >= 500){
        previousTime = time;
        if(alarmLedOn){
          color(255,0,0);
          tone(45, 4000);
        }
        else{
          color(0,0,0);
          tone(45, 3000);
        }
        alarmLedOn = !alarmLedOn;
    }
  }

  if(Serial3.available() > 0){
    btText = Serial3.readStringUntil('|');
    if(btText == "ZAMCENO"){
      setState(State::ZAMCENO);
    }
    else if(btText == "ODEMCENO"){
      setState(State::ODEMCENO);
    }
    else if(btText.length() == 4 /*&& isDigit(btText)*/){
      for(int i = 0; i<4; i++){
        EEPROM.write(i, btText[i]);
      }
      clearRow(1);
      display.setCursor(0,1);
      display.print("PIN ULOZEN");
      delay(2000);
      clearRow(1);
    }
  }
  if(motionDetected){
    motionDetected = false;
    if(stav == State::ZAMCENO){
      setState(State::POPLACH);
    }
  }
}

void color(int red, int green, int blue){
  analogWrite(pin_red, red);
  analogWrite(pin_green, green);
  analogWrite(pin_blue, blue);
}

void setState(State state){
  stav = state;
  pass = "";
  clearRow(0);
  display.setCursor(0,0);
  noTone(45);
  switch(stav){
    case State::ZAMCENO:
      attachInterrupt(digitalPinToInterrupt(pin_pirP), detect, RISING);
      attachInterrupt(digitalPinToInterrupt(pin_pirB), detect, RISING);
      display.print("Stav: ZAMCENO");
      Serial3.print("ZAMCENO|");
      servo.write(0);
      color(0,0,255);
      break;
    case State::ODEMCENO:
      detachInterrupt(digitalPinToInterrupt(pin_pirP));
      detachInterrupt(digitalPinToInterrupt(pin_pirB));
      display.print("Stav: ODEMCENO");
      Serial3.print("ODEMCENO|");
      servo.write(180);
      color(0,255,0);
      break;
    case State::POPLACH:
      display.print("!!! POPLACH !!!");
      Serial3.print("POPLACH|");
      servo.write(0);
      break;
  }
}

void clearRow(int row){
  display.setCursor(0,row);
  display.print("                ");
}

void detect(){
  //setState(State::POPLACH);
  motionDetected = true;
}