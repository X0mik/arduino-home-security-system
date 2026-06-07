#include <Keypad.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Servo.h>

LiquidCrystal display(22, 23, 39, 37, 35, 33, 31, 29, 27, 25);
Servo servo;


const int pin_red = 10;
const int pin_green = 11;
const int pin_blue = 12;
const int pin_pirP = 20;
const int pin_pirB = 21;
const int pin_buzzer = 45;

const int rows = 4;
const int cols = 4;

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
volatile bool motionDetected = false;

int wrong_pass = 0;

String pass = "";
String encrPass = "";
String btText = "";


enum State{
  ZAMCENO,
  ODEMCENO,
  POPLACH
};

enum Color{
  RED,
  GREEN,
  BLUE,
  OFF
};


State stav = State::ODEMCENO;

void setup()
{
  display.begin(16,2);
  servo.attach(13);
  pinMode(pin_buzzer, OUTPUT);
  pinMode(pin_red, OUTPUT);
  pinMode(pin_green, OUTPUT);
  pinMode(pin_blue, OUTPUT);
  Serial.begin(9600);
  Serial3.begin(9600);
  pinMode(pin_pirP, INPUT);
  pinMode(pin_pirB, INPUT);

  setState(State::ODEMCENO);
}

void loop()
{
  time = millis();

  keypadFunc();
  bluetoothFunc();

  if(motionDetected){
    motionDetected = false;
    if(stav == State::ZAMCENO){
      setState(State::POPLACH);
    }
  }
}

// -- LED --

void color(int red, int green, int blue){
  analogWrite(pin_red, red);
  analogWrite(pin_green, green);
  analogWrite(pin_blue, blue);
}

void setLed(Color clr){
  
  switch(clr){
    case Color::RED:
      color(255, 0, 0);
      break;
    case Color::GREEN:
      color(0, 255, 0);
      break;
    case Color::BLUE:
      color(0, 0, 255);
      break;
    case Color::OFF:
      color(0, 0, 0);
      break;
  }
}

// -- KEYPAD --

void keypadFunc(){
  char key = keypad.getKey();
  if(stav == State::POPLACH){
    if(time - previousTime >= 500){
        previousTime = time;
        if(alarmLedOn){
          setLed(RED);
          tone(pin_buzzer, 4000);
        }
        else{
          setLed(OFF);
          tone(pin_buzzer, 3000);
        }
        alarmLedOn = !alarmLedOn;
    }
  }
  else{
    if(isDigit(key) && pass.length() < 4){
      tone(pin_buzzer, 2000, 50);
    }
    else if(isDigit(key) && pass.length() >= 4){
      tone(pin_buzzer, 500, 150);
    }

  }

  /*if(key == 'B'){
    setState(State::ODEMCENO);
  }
  else if(key == 'C'){
    setState(State::ZAMCENO);
  }
  else if(key == 'D'){
    setState(State::POPLACH);
  }*/

  if(isDigit(key) && pass.length() < 4){
    pass += key;
    clearRow(1);
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
  }

  if (pass.length() == 4 && key == '#'){
    clearRow(1);
    if(newPin){
      pinToEEPROM();
    }
    else{
      passCheck();
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
}

void pinToEEPROM(){
  tone(pin_buzzer, 2500, 100);
  for(int i = 0; i < pass.length(); i++){
    EEPROM.write(i, pass[i]);
  }
  newPin = false;
  display.setCursor(0,1);
  display.print("PIN ULOZEN");
}

void passCheck(){
  for(int i = 0; i < pass.length(); i++){
    correctPass = true;
    if(pass[i] != EEPROM.read(i)){
       correctPass = false;
      break;
    }
  }
  if(correctPass){
    wrong_pass = 0;
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
    tone(pin_buzzer, 2500, 100);
  }
  else{
    wrong_pass += 1;
      if(wrong_pass >=3){
        wrong_pass = 0;
        setState(State::POPLACH);
      }
    tone(pin_buzzer, 500, 150);
    clearRow(1);
    display.setCursor(0,1);
    display.print("SPATNE HESLO!");
  }
}

// -- BLUETOOTH --

void bluetoothFunc(){
  if(Serial3.available() > 0){
    btText = Serial3.readStringUntil('|');
    if(btText == "ZAMCENO"){
      setState(State::ZAMCENO);
    }
    else if(btText == "ODEMCENO"){
      setState(State::ODEMCENO);
    }
    else if(btText.length() == 4){
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
}

// -- CURRENT STATE (LOCKED, UNLOCKED, ALARM) --

void setState(State state){
  stav = state;
  pass = "";
  clearRow(0);
  display.setCursor(0,0);
  noTone(pin_buzzer);
  switch(stav){
    case State::ZAMCENO:
      motionDetected = false;
      attachInterrupt(digitalPinToInterrupt(pin_pirP), detect, RISING);
      attachInterrupt(digitalPinToInterrupt(pin_pirB), detect, RISING);
      display.print("Stav: ZAMCENO");
      Serial3.print("ZAMCENO|");
      servo.write(0);
      setLed(BLUE);
      break;
    case State::ODEMCENO:
      detachInterrupt(digitalPinToInterrupt(pin_pirP));
      detachInterrupt(digitalPinToInterrupt(pin_pirB));
      display.print("Stav: ODEMCENO");
      Serial3.print("ODEMCENO|");
      servo.write(180);
      setLed(GREEN);
      break;
    case State::POPLACH:
      display.print("!!! POPLACH !!!");
      Serial3.print("POPLACH|");
      servo.write(0);
      break;
  }
}

// -- ROW CLEARING FOR LCD --

void clearRow(int row){
  display.setCursor(0,row);
  display.print("                ");
}

// -- PIR SENSOR MOVEMENT DETECTION FUNCTION --

void detect(){
  motionDetected = true;
}
