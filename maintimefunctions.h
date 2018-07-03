#include "Servo.h"

Servo servo; // creates servo object to control the SG90 servo

#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR   0x27  //If 0x3f doesn't work change this to 0x27
#define BACKLIGHT_PIN  3 //LCD
#define PIN_SERVO 7 //SERVO
#define BUZZERPIN 5//buzzer pin
#define _ARRAY_SIZE_ 5// size of the array that contains scheduled actions

LiquidCrystal_I2C lcd(I2C_ADDR, 2, 1, 0, 4, 5, 6, 7); //These pin numbers are hard coded in on the serial backpack board.


typedef struct// time of the day, and now I realize that I could store just number of seconds from the midnight and some of the functions would beccome unnecessary. But that's the error you do, when study Java OOP for 1 semester 
{
  int hours;
  int minutes;
  int seconds;
  bool isset; //is active, if false - mean void, no matter what hours minutes seconds are
}  daytime;

/*Global vars*/
unsigned long initialmilliseconds;//we store millis() here, that was actual, when we set up current time, currentDayTime is always computed as [millis()-initialmilliseconds]+startDayTime
daytime currentDayTime;//current datetime that is updated dynamically by loop
daytime startDayTime; // initial datetime used as a base for calculation of the current datetime
daytime tmp[1];//temporary datetime variable used in all calculations as an array[1] for simplicity
daytime feedschedule[_ARRAY_SIZE_];// array which holds all scheduled daytime actions

/*daytime operations*/
void setDaytime( daytime * myDaytime, int hour, int minute, int second);
void addToSchedule(int hour, int minute, int seconds);// add hour:minute:seconds time to schedule (array feedschedule)
void dropFromSchedule(int hour, int minute, int seconds);// drop hour:minute:seconds time from the feedschedule array
void addDayTime( daytime * myDaytime, int hour, int minute, int second);// increases the first argument myDaytime by number of hour minute second
void copyDayTime( daytime * receiver, daytime copier);  // copies copier values to receiver
//void unsetDayTime(daytime * myDaytime);// set particular daytime inactive //not used
void updateDayTime(); // updates variable responsible for keeping current datetmie (currentDayTime), ised inside the loop 
void sortSchedule(); // sorts feedschedule array in order to be properly processed by action(we can compare current datetime only with feedschedule[0]) and for nice look on LCD
long numberOfSecondsToWait(daytime* a);// used as a core for the sortSchedule() function, for a daytime *a we compute number for seconds from currentDayTime to a
long getSecondsFromMidnight(daytime * a); // used in long numberOfSecondsToWait(daytime* a), computes number of seconds from the midnight to datetime *a
void setCurrentDayTime(int hour, int minute, int second);// init global variable currentDayTime that is used to get current datetime

/*initialization*/
void initServo();//initialization routine related to Servo
void initLCD();//initialization routine related to Servo
void initDayTime();  //init routine related to daytime, called from initializeDayTime
void initializeDayTime(); // all initialization if the daytime routine: set initial datetime to 1:00:00, put initial values to  feedschedule array

/*Display/output*/
String printDayTime(daytime* myDayTime);//returns string that will be printed to LCD, for current time
String printHourMin(daytime* myDayTime);//returns string for the schedule(without seconds), uset for output feed schedule on LCD
void displayLCDSchedule();//function displays current schedule on LCD 
void serailPrintDatetime(daytime * a); // prints daytime to Serial, used for debug purposes
void serialPrintSchedule();// only for debug, prints current time state [currentDayTime + feedschedule] to the Serial
void TwitterPrintSchedule();// tweets current schedule along with current datetime
void  makeTweet(String tweet); // just declaration, the main function is in Twitter6 code

/*Action*/
void ActionLoop(); // all routime we are doing inside the loop
void doAction(); // here we describe an action we want to be performed when there's a scheduled time


void setDaytime(daytime * myDaytime, int hour, int minute, int second)//sets daytime
{
  myDaytime->isset = true;
  myDaytime->hours = 0;
  myDaytime->minutes = 0;
  myDaytime->seconds = 0;
  addDayTime(myDaytime, hour, minute, second);
}

void addToSchedule(int hour, int minute, int seconds)// add hour:minute:seconds time to schedule (array feedschedule)
{

  setDaytime(&tmp[0], hour, minute, seconds);

  if (numberOfSecondsToWait(&tmp[0]) < numberOfSecondsToWait(&feedschedule[_ARRAY_SIZE_ - 1]))
  {
    setDaytime(&(feedschedule[_ARRAY_SIZE_ - 1]),  hour, minute, seconds);
    feedschedule[_ARRAY_SIZE_ - 1].isset = true;
    serialPrintSchedule();
    sortSchedule();
    serialPrintSchedule();
  }
}

void dropFromSchedule(int hour, int minute, int seconds)// drop hour:minute:seconds time from the schedule
{
  // daytime tmp;
  setDaytime(&tmp[0], hour, minute, seconds);

  for (int i = 0; i < _ARRAY_SIZE_; i++)
  {
    if (getSecondsFromMidnight(&tmp[0]) == getSecondsFromMidnight(&feedschedule[i]))
      feedschedule[i].isset = false;
  }
  sortSchedule();

}

void addDayTime(daytime * myDaytime, int hour, int minute, int second)// all arguments must be >0, adds hour:minute:second shift to myDayTime datetime
{
  myDaytime->seconds =  (myDaytime->seconds + second);
  minute += (myDaytime->seconds) / 60;
  (myDaytime->seconds) = (myDaytime->seconds) % 60;

  (myDaytime->minutes) =  (myDaytime->minutes + minute) % 60;
  hour += (myDaytime->minutes) / 60;
  myDaytime->minutes = (myDaytime->minutes) % 60;

  myDaytime->hours = (myDaytime->hours + hour) % 24;
}


void copyDayTime(daytime * receiver, daytime copier)// just in case, not used yet
{
  receiver->seconds = copier.seconds;
  receiver->minutes = copier.minutes;
  receiver->hours = copier.hours;
}

/*
void unsetDayTime(daytime * myDaytime)// set particular daytime inactive
{
  myDaytime->isset = false;
}

*/

String printDayTime(daytime* myDayTime)//returns string that will be printed to LCD, for current time
{
  String result = "";
  result += myDayTime->hours;
  result += ":";
  result += myDayTime->minutes;
  result += ":";
  result += myDayTime->seconds;
  return   result;
}

String printHourMin(daytime* myDayTime)//returns string for the schedule(without seconds), uset for output feed schedule on LCD
{
  String result = "";
  result += myDayTime->hours;
  result += ":";
  result += myDayTime->minutes;
  return   result;
}

void setCurrentDayTime(int hour, int minute, int second)// init variables used to get current datetime
{
  setDaytime(&startDayTime, hour, minute, second);
  setDaytime(&currentDayTime, hour, minute, second);

  initialmilliseconds =  millis();
  sortSchedule();
}

void initializeDayTime() {// all initialization if the daytime routine
  //Serial.println("Start initializing:");
  setDaytime(&startDayTime, 0, 0, 0);
  setDaytime(&currentDayTime, 0, 0, 0);

  initialmilliseconds =  millis();

  for (int i = 0; i < _ARRAY_SIZE_ ; i++)
  {
    setDaytime(&(feedschedule[i]), 0, 0, 0);
    feedschedule[i].isset = false;
  }
  //Serial.println("Process tmp:");
  //delay(1000);
  setDaytime(&tmp[0], 0, 0, 0);

  //Serial.println("End initializing:")  ;
}

void sortSchedule() { // sorts feedschedule array in order to be properly processed by action(we can compare current datetime only with feedschedule[0]) and for nice look on LCD

  int i, j;

  for (i = 0; i < _ARRAY_SIZE_; i++)
    for (j = i + 1; j < _ARRAY_SIZE_; j++)
      {
        if (numberOfSecondsToWait(&feedschedule[i]) > numberOfSecondsToWait(&feedschedule[j]))      
      {
        setDaytime(&tmp[0], feedschedule[i].hours, feedschedule[i].minutes, feedschedule[i].seconds);
        tmp[0].isset = feedschedule[i].isset;
        // aka tmp = feedschedule[i];

        setDaytime(&(feedschedule[i]), feedschedule[j].hours, feedschedule[j].minutes, feedschedule[j].seconds);
        feedschedule[i].isset = feedschedule[j].isset;
        // aka feedschedule[i] = feedschedule[j];
        setDaytime(&(feedschedule[j]), tmp[0].hours, tmp[0].minutes, tmp[0].seconds);
        feedschedule[j].isset = tmp[0].isset;
        //aka  feedschedule[j] = tmp;
      }

      }
}

void updateDayTime() {// updates variables responsible for keeping current datetmie, ised in the loop

  unsigned long utime;

  utime = millis();

  copyDayTime(&currentDayTime, startDayTime);
  addDayTime(&currentDayTime, 0, 0, ((utime - initialmilliseconds) / 1000) % 86400);

  if (getSecondsFromMidnight(&feedschedule[0]) < getSecondsFromMidnight(&currentDayTime) + 2 && feedschedule[0].isset == true)
  {
    Serial.println("Time to perform the scheduled action");
    doAction();
  }
  sortSchedule();
  serialPrintSchedule();
}

long numberOfSecondsToWait(daytime* a)// used as a core for scheduled datetime sorting
{
  long i;
  long j;

  if ((a->isset) == false)
  {
    return  86401;// 24h + 1 sec
  }

  i = getSecondsFromMidnight(a);
  j = getSecondsFromMidnight(&currentDayTime);

  if (i == j)
  {
    return 0;
  }

  if (i > j) {
    return i - j;
  }

  return 86400 + i - j;//24h
}

long getSecondsFromMidnight(daytime* a)//just convenient function for comparison 2 datetimes, mainly used in numberOfSecondsToWait
{
  return (long)(((long)a->hours) * 3600 + ((long)a->minutes) * 60 + (a->seconds));
}


void initServo()//init routine related to Servo
{
  servo.attach(PIN_SERVO);
  servo.write(0);
}

void initLCD() { //init routine related to LCD
  lcd.begin (16, 2);    //Initalize the LCD.
  lcd.setBacklightPin(3, POSITIVE); //Setup the backlight.
  lcd.setBacklight(HIGH); //Switch on the backlight.
  lcd.clear();
}

void initDayTime() { //init routine related to daytime
  initializeDayTime();
  setCurrentDayTime(1, 0, 0);
  Serial.println("CurrentDaytime set");
  
  addToSchedule(8, 1, 0);
  //Serial.println("Added1 to schedule");
  delay(100);
  addToSchedule(8, 2, 0);
  //Serial.println("Added1 to schedule");
  delay(100);
  addToSchedule(12, 0, 0);
  //Serial.println("Added1 to schedule");
  delay(100);
  addToSchedule(16, 0, 0);
  //Serial.println("Added1 to schedule");
  delay(100);
  addToSchedule(20, 0, 0);
  //Serial.println("Added1 to schedule");
}


void displayLCDSchedule()//function displays current schedule on LCD
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(printDayTime(&currentDayTime));

  lcd.setCursor(11, 0);
  if (_ARRAY_SIZE_ >= 1)
    if (feedschedule[0].isset == true)
      lcd.print(printHourMin(&(feedschedule[0])));

  lcd.setCursor(0, 1);
  if (_ARRAY_SIZE_ >= 2)
    if (feedschedule[1].isset == true)
      lcd.print(printHourMin(&(feedschedule[1])));

  lcd.setCursor(6, 1);
  if (_ARRAY_SIZE_ >= 3)
    if (feedschedule[2].isset == true)
      lcd.print(printHourMin(&(feedschedule[2])));


  lcd.setCursor(11, 1);
  if (_ARRAY_SIZE_ >= 4)
    if (feedschedule[3].isset == true)
      lcd.print(printHourMin(&(feedschedule[3])));
}


void serailPrintDatetime(daytime * a)// only for debug
{
  Serial.println();
  Serial.print(a->hours);
  Serial.print(":");
  Serial.print(a->minutes);
  Serial.print(":");
  Serial.print(a->seconds);
  Serial.print(":");
  if (a->isset == false)
    Serial.print("false");
  else
    Serial.print("true");
}

void serialPrintSchedule()// only for debug
{
 
  Serial.println ("CurrentTime:");

  Serial.print (currentDayTime.hours);
  Serial.print (":");
  Serial.print (currentDayTime.minutes);
  Serial.print (":");
  Serial.println (currentDayTime.seconds);
  Serial.println ("_SCHEDULE:"); 
  for (int i = 0; i < _ARRAY_SIZE_; i++)
  {
    Serial.println();
    Serial.print(feedschedule[i].hours);
    Serial.print(":");
    Serial.print(feedschedule[i].minutes);
    Serial.print(":");
    Serial.print(feedschedule[i].seconds);
    Serial.print(":");
    if (feedschedule[i].isset == false)
      Serial.print("false");
    else
      Serial.print("true");
  }
  Serial.println();
}


void TwitterPrintSchedule()// tweets current schedule along with current datetime
{
  String tweet = "CurrentTime:";

  tweet += (currentDayTime.hours);
  tweet += (":");
  tweet += (currentDayTime.minutes);
  tweet += ("_SCHEDULE:");
  for (int i = 0; i < _ARRAY_SIZE_; i++)
  {
    if (feedschedule[i].isset == true)
    {
      tweet += (feedschedule[i].hours);
      tweet += (":");
      tweet += (feedschedule[i].minutes);
      tweet += "_";
    }
  }
  makeTweet(tweet);

}


void ActionLoop() { //loop routine related to current datetime and LCD representatiob
  
  Serial.println("Entering action loop");
  updateDayTime();
  displayLCDSchedule();
  serialPrintSchedule();

}

void doAction() { // core function, represents action (feed)
  
  Serial.println("Action");
  servo.write(90);

  for (int i = 0; i <= 5; i++)
  {
    tone(BUZZERPIN, 330, 800);
    delay(1000);
  }

  servo.write(0);
<<<<<<< HEAD
}
=======
}
>>>>>>> 817c86810769588d779cb05766bf09d65af9fb2a
