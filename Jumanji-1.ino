//LCD Library
#include <LiquidCrystal.h>
//Servo Motor Library
#include <Servo.h>
//EEPROM library that is used to store the latest state of the game 
#include <EEPROM.h>


//LCD Module connections
const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = A0;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Seven segment
const byte SEVEN_SEGMENT_A = 8;
const byte SEVEN_SEGMENT_B = 9;
const byte SEVEN_SEGMENT_C = 10;
const byte SEVEN_SEGMENT_D = 12;

//analog keypad pin
const byte KEYPAD_PIN = A5;
//Servo motor pin
const byte SERVO_ADDRESS = 11;
// Push button
const byte PUSH_BUTTON = 2;
// Buzzer pin
const int speakerPin = 13;
//green and red leds
const byte GREEN_LED = A3;
const byte RED_LED = A2;
//Intervals for the Keypad response
unsigned long kaypadPrevMillis = 0;
unsigned long keypadInterval = 1000;

//Servo motor object
Servo myservo;  
//angle current position
byte posServo = 0;    
//Intervals for the motor (rotation time per degree)
unsigned long previousMillisServo = 0;
//holds the motor rotation time per degree 
unsigned int intervalServo = 300;
//slowest rotation can be achieved (400 ms/degree)
const unsigned int maxIntervalServo = 400;
//fastest rotation can be achieved (100 ms/degree)
const unsigned int minIntervalServo = 100;
//the award by increasing the rotation time with it (motor moves slower)
//the penalty by decreasing the rotation time with it (motor moves faster)
const byte award_penalty = 50; 
//termination angle which states that the player has fallen in the hole
const byte MAX_ANGLE = 90;

//Array f questions
const char *Questions[] = {
    "2x - 5 = 7",
    "x^2 = 169",
    "x^2 -2x + 1 = 0",
    "10^x = 10000",
    "x^3 - 1 = 26",
    "Log(x) = 2",
    "1,4,9,x,25",
    "2,4,x,16,32",
    "1,3,6,x,15,21"
}; 
//Array of MCQs
const char *MCQs[] = {
    "3   7   5   6",
    "14  15  13  11",
    "1   2   3   4",
    "3   5   4   6",
    "5   3   2   4",
    "1   2  10  100",
    "15  11  16  13",
    "10  8   6   12",
    "8   9   10  None"
}; 
//Array of correct answers indices 
const char answers[] = {'4','3','1','3','2','4','3','2','3'};

//holds current question index
byte current_question = 0;
//holds last displayed question on LCD
//used to prevent re-printing the question on the LCD every time
byte last_displayed = 100;
//holds the player score
byte score = 0;
//state of game ending wihch is reached by at least one of the following cases:
  //1- Solving all questions
  //2- Motor reaches angle 90
bool game_end = false;
//state that the game is started by clicking the push button
bool did_start = false;
//to prevent receiving keypad press for specific period of time
//used to solve the debounce problem
bool acceptKey = false;

/**
 * applying delay using timers
 */
void nonBlockingDelay(unsigned long d){
  unsigned long prev = millis();
  while((unsigned long)(millis() - prev) < d);
}

/**
 * Plays an alert noisy tone indicates that player failed
 */
void failTone(){
  tone(speakerPin, 100);
  nonBlockingDelay(2000);
  noTone(speakerPin);
}

/**
 * Plays a happy tone indicates that player won the game
 */
void winTone(){
  tone(speakerPin,500, 300);
  nonBlockingDelay(300);
  tone(speakerPin,500, 300);
  nonBlockingDelay(300);
  tone(speakerPin,500, 300);
  nonBlockingDelay(300);
  tone(speakerPin,800, 200);
  nonBlockingDelay(200);
  tone(speakerPin,500, 600);
  nonBlockingDelay(600);
  tone(speakerPin,600, 1000);
}

/**
 * sets the values of the 7 segement 
 */
void setSevenSegment(int number){
  byte bits[4] = {0,0,0,0};
  int k = 3;
  while (number) {
     int bt = number % 2;
     bits[k] = (bool)bt;
     number /= 2;
     k--;
  }
  digitalWrite(SEVEN_SEGMENT_A, bits[3]);
  digitalWrite(SEVEN_SEGMENT_B, bits[2]);
  digitalWrite(SEVEN_SEGMENT_C, bits[1]);
  digitalWrite(SEVEN_SEGMENT_D, bits[0]);
}

/**
 * sets the values for the leds (green and red)
 */
void setGreenRedLeds(byte greenValue, byte redValue){
  digitalWrite(GREEN_LED, greenValue);
  digitalWrite(RED_LED, redValue);
}

/**
 * ISR for pressing the start push button
 */
void startGame(){
  Serial.println("Started");
  //initialize question index and score
  current_question = 0;
  last_displayed = -1;
  score = 0;
  game_end = false;
  did_start = true;
  //initialize servo position
  posServo = 0;
  myservo.write(0);
  //initialize keypad accesptance keys
  acceptKey = false;
  //initialiize timers of keypad and servo with current time
  kaypadPrevMillis = millis();
  previousMillisServo = millis();
  //initialize the speed of the motor
  intervalServo = 300;
  //initialize leds to be off, seven segment to be 0
  setGreenRedLeds(0, 0);
  setSevenSegment(0); 
  
  
  //saveState();
}

/**
 * when game is ended, checks if winning or losing
 */
void endGameCheck(){
    //move to endgame state
    game_end = true;
    //flush EEPROM memory as we don't need to keep state any more since game is ended
    flushMemory();
    lcd.clear();
    lcd.print("Endgame!");
    //case of losing, if player solved less than 7 question correctly or reached angle 90 degree
    if(posServo >= MAX_ANGLE || score<7){
      Serial.println("You die, Fall in the hole!");
      //make the player fall
      posServo = MAX_ANGLE;
      myservo.write(posServo);
      setGreenRedLeds(0, 1);
      lcd.setCursor(0, 1);
      lcd.print("You Die!");
      failTone();
    }
    //case of winning if solved at least 7 questions correctly
    else if(score >= 7){
      Serial.println("You live");
      //send back to home safe;
      posServo = 0;
      myservo.write(posServo);
      setGreenRedLeds(1, 0);
      lcd.setCursor(0, 1);
      lcd.print("You Live!");
      winTone(); 
    } 
}

/**
 * Displays the question on the LCD
 */
void displayQuestion(){
  //if questions are not finished yet
  if(current_question < 9){
    lcd.clear();
    lcd.print(Questions[current_question]);
    lcd.setCursor(0, 1);
    lcd.print(MCQs[current_question]);
    last_displayed = current_question;
  }
  //if game is over
  else{
     endGameCheck();     
  }
}

/**
 * returns the choice of the player on analog keypad
 */
char getKeyPressed(){
  int key = analogRead(KEYPAD_PIN);
  char index = '0';
  //Serial.println(key);
  if(key > 70 && key <= 90){
    Serial.println(key);
    index = '1';}
  else if(key > 40 && key <= 55){
    Serial.println(key);
    index = '2';}
  else if(key > 0 && key <= 20){
    Serial.println(key);
    index = '3';}
  else if(key > 145 && key <= 200){
    Serial.println(key);
    index = '4';}
  return index;
}

/**
 * this function ereases the used addresses of the program state 
 */
void flushMemory(){
  EEPROM.write(0, 0);
  EEPROM.write(10, 0);
  EEPROM.write(20, 0);
  EEPROM.write(30, 0);
  EEPROM.write(40, 0);
  EEPROM.write(50, 0);
  EEPROM.write(60, 0);
}
/**
 * this function saves the program state 
 */
void saveState(){
  //since we only increase/descrease the speed by large number
  //so we can store a factor of it in the memory as the EEPROM only takes bytes
  byte speedServo = intervalServo/5;
  EEPROM.put(0, speedServo);

  //save some important params
  //1-Servo angle in address 10
  //2-Curent question in address 20
  //3-Current Displayed question in address 30
  //4-player score in address 40
  //5-game starting flag in address 50
  //6-flag that we already started saving on memory (to make next read from memory not from program)
  EEPROM.put(10, posServo);
  EEPROM.put(20, current_question);
  EEPROM.put(30, last_displayed);
  EEPROM.put(40, score);
  EEPROM.put(50, did_start);
  EEPROM.put(60, 1);
}

/**
 * This function reads the state params from EEPROM
 */
void readState(){
  //checks flag if we saved anything before 
  if(EEPROM.read(60) == 1){
    Serial.println("Reading State");
    intervalServo = ((unsigned int)(EEPROM.read(0)))*5;
    posServo = EEPROM.read(10);
    current_question = EEPROM.read(20);
    last_displayed = EEPROM.read(30);
    score = EEPROM.read(40);
    did_start = EEPROM.read(50);
  }
}

void setup(){
  //read from EEPROM latest state if exists;
  readState();
  //initialize the LCD
  lcd.begin(16, 2);
  if(did_start == false){
    lcd.clear();
    lcd.print("    Jumanjii!   ");
    lcd.setCursor(0, 1);
    lcd.print("Find X value!");
  }else{
    displayQuestion();
  }
  //initialize inputs and outputs;
  Serial.begin(9600);
  pinMode(PUSH_BUTTON, INPUT);
  pinMode(KEYPAD_PIN, INPUT);
  //interrupt on start button
  attachInterrupt(digitalPinToInterrupt(PUSH_BUTTON), startGame, HIGH);
  //servo object initialization
  myservo.attach(SERVO_ADDRESS);
  myservo.write(posServo);
  //seven segment pins
  pinMode(SEVEN_SEGMENT_A, OUTPUT);
  pinMode(SEVEN_SEGMENT_B, OUTPUT);
  pinMode(SEVEN_SEGMENT_C, OUTPUT);
  pinMode(SEVEN_SEGMENT_D, OUTPUT);
  setSevenSegment(score);
  //leds pins
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  //buzzer pin
  pinMode(speakerPin, OUTPUT);  
  
  Serial.println("Entering Void Setup");
  Serial.println(EEPROM.read(60));
  //saveState();
}

void loop() {
    
  //checks if the game is not ended and started already
  if(!game_end && did_start){

    //solves the problem of LCD blinking
    //if the current question is the same as displayed question
    if(last_displayed != current_question)
      displayQuestion();

    //timer to accespt input keypad
    unsigned long currentMillis = millis();
    if((unsigned long)(currentMillis - kaypadPrevMillis) >= keypadInterval){
      acceptKey = true;
      //Serial.println("Accept Key now");
    }
    char key = '0';
    //when time is passedm then read the key presses
    if(acceptKey){
      setGreenRedLeds(0, 0);
      key = getKeyPressed(); 
    }

    //if the user entered an answer
    if (key != '0' ){
      //reset the timer for next read of the keypad
      kaypadPrevMillis = millis();
      //prevent reading until time is passed(solves the debounce problem)
      acceptKey = false;
      
      Serial.print("Entered: ");
      Serial.println(key);
      Serial.print("Answer: ");
      Serial.println(answers[current_question]);

      //case the answer is correct
      if(key == answers[current_question]){
          //increase score and turn on the green led
          score += 1;
          setGreenRedLeds(1, 0);
          //checks if the motor speed can be decreased (didn't reach the max time)
          if((intervalServo + award_penalty) < maxIntervalServo)
               intervalServo += award_penalty; //decrease motor speed as an award
      }
      else{
          //no score change, turn on the red led
          setGreenRedLeds(0, 1);
          //checks if the motor speed can be increased (didn't reach the min time)
          if((intervalServo - award_penalty) >= minIntervalServo)
               intervalServo -= award_penalty; //increase motor speed as a penalty
      }  
      //show next question, update score 
      current_question += 1;
      Serial.print("Score: ");
      Serial.println(score);
      setSevenSegment(score);

      //saveState();
    }

    //check servo timer to increase it the new speed
    unsigned long currentMillisServo = millis();
    if((unsigned long)(currentMillisServo - previousMillisServo) >= intervalServo){
      //if player didn't fall yet
      if(posServo <= MAX_ANGLE)
        myservo.write(posServo);
      //if player falls, then end questions
      else{
        current_question = 10;
      }  
      //update timer and increase the angle by one step
      previousMillisServo = currentMillisServo;
      posServo++;

      //save state after 2 degrees of rotation
      //we chose this rate to first save memory multiple writes (as EEPROM has limited writes)
      //also it's error = (delta/2) = 1 degree only,
      //which is not noticed by eye or affects game time significantly
      if(posServo%2 == 0)
        saveState();
    }
  }
}
