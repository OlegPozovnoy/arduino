
#include "connectionworkaround.h"
#include "maintimefunctions.h"

#include <string.h>

#define GMT_OFFSET -4//ofsetting hours that we received form google.com
#define WAIT_TILL_READTHENEXTCOMMAND 60000 //sets the period in ms used for connecting to the internet and reading the commands

/*Globals*/
String lastCommand;//previous command we read from arduino71721.azurewebsites.net. We read only one command from Internet (the title of the site), and entering the process cycle only if lastCommand<>newCommand
String newCommand; // new command we read from arduino71721.azurewebsites.net
bool isConnecctedToWifi;// global to store if we are connected to wifi
unsigned long internetConnection;//used to store mills inside the loop(), to check is its time to reconnect to the internet and read new command
bool isInternetTimeSet = false;//global variable, used to understand if we already read the time from the internet. Since we got current time from internet - no need to reconnect

/*main function related to Internet*/
void makeTweet(/*char**/ String msg);// connect to arduino-tweet.appspot.com and make tweet to @Arduino71721
String readTwitter(); //read command form  the title of arduino71721.azurewebsites.net, we change header of the site using FileZilla
void getTime(); //connecting to my favorite google.com to get GMT time

/*command processing*/
void setTimeFromInternet(String response);//convert response from Google.com adn sets the internal daytime
void processInsertDelete(String response); //processing Insert/Delete timestamp commands from  arduino71721.azurewebsites.net
void processTwitterCommand (String command);//main function that describes, how weprocess our command
/*modem routine */
void getResponse();//convert serial1(ESP) from char to string and output
void connectionStuff();//rebot modem and reconnect to wifi

void setup() {
  Serial.begin(9600);  // Open serial Connection to Computer
  Serial1.begin(115200);  // Open serial Connection to ESP8266

  while (!Serial) {
    ;
  }

  initServo();//servo
  Serial.println("Servo is initialized");
  delay(1000);
  initLCD(); //lcd
  Serial.println("LCD is initialized");
  delay(1000);
  initDayTime();//daytime
  Serial.println("Daytime is initialized");
  //  Serial.println("EndSetup");
  delay(1000);
  internetConnection = millis();

}

void loop()
{

  if (millis() - internetConnection > WAIT_TILL_READTHENEXTCOMMAND)
  {

    internetConnection = millis();
    if (isInternetTimeSet == false) // read time from the Internet
      getTime();

    newCommand = readTwitter(); // read command from the twitter

    if (newCommand.length() == 0)
      newCommand = String(lastCommand);//do nothing if we read unsucessfully

    if (lastCommand.equals(newCommand) == false)
    {
      Serial.println("Old command:" + lastCommand);
      Serial.println("New command:" + newCommand);
      processTwitterCommand(newCommand);//processing new command
      //makeTweet(newCommand);

      lastCommand = newCommand;
    }


  }

  ActionLoop();//rest of the commands
  delay(1000);

}


void makeTweet(String tweet)// tweets tweet
{
  Serial.println("Make tweet:" + tweet);
  connectionStuff();//we rebot each time we connect to the internet, it's longer, but safer
   if (isConnecctedToWifi==false)
    return; 
  //Builds the connection string for the ESP8266
  String cmd = "AT+CIPSTART=\"TCP\",\"arduino-tweet.appspot.com\",80";
  Serial1.println(cmd);  //Run the command
  Serial.println(cmd);    //Print this to the debug window
  delay(1000);

  while (Serial1.available()) {
    Serial.write(Serial1.read());
  }
  //tweeting
  String httpcmd = "POST /update HTTP/1.1\r\nHost: arduino-tweet.appspot.com\r\nContent-Length: ";
  httpcmd += (int)(tweet.length() + strlen("983776073849729025-DnneUUCTXIHezXhJWeD2mFBuslpFH42") + 14);
  httpcmd += "\r\n\r\ntoken=983776073849729025-DnneUUCTXIHezXhJWeD2mFBuslpFH42&status=";
  httpcmd += tweet; //String(msg);
  httpcmd += "\r\n";
  Serial.println("Resulting httpcommand");
  Serial.println(httpcmd);
  Serial.println(httpcmd.length());


  Serial.print("AT+CIPSEND=");
  Serial.println(httpcmd.length());

  Serial1.print("AT+CIPSEND=");
  Serial1.println(httpcmd.length());
  delay(1000);
  getResponse();
  //
  Serial.print(">");
  Serial1.println(httpcmd);
  Serial.println("HTTP command:");
  Serial.println(httpcmd);
  delay(3000);

  getResponse();

  Serial.println("AT+CIPCLOSE");
  Serial1.println("AT+CIPCLOSE"); //Close the Web Connection
  delay(1000);
  getResponse();
}


String readTwitter() { //read command form  the title of arduino71721.azurewebsites.net, we change header of the site using FileZilla
  String line = "";
  String tmp="";
  Serial.println("read tweet:");
  connectionStuff();
   if (isConnecctedToWifi==false)
    return;   
  //Builds the connection string for the ESP8266
  //String cmd = "AT+CIPSTART=\"TCP\",\"wordpress.com\",80";
  String cmd = "AT+CIPSTART=\"TCP\",\"arduino71721.azurewebsites.net\",80";
  Serial1.println(cmd);  //Run the command
  Serial.println(cmd);    //Print this to the debug window
  delay(1000);

  getResponse(); // convert all chars to srring and print to Serial

  //setting up the connection to my page

  //String httpcmd = "GET  http://fking85.wordpress.com/ HTTP/1.1\r\nHost:wordpress.com\r\n\r\n";
    String httpcmd = "GET  http://arduino71721.azurewebsites.net/ HTTP/1.1\r\nHost:arduino71721.azurewebsites.net\r\n\r\n";
  if (!echoCommand("AT+CIPSEND=" + String(httpcmd.length()), ">", CONTINUE))
  {
    echoCommand("AT+CIPCLOSE", "", CONTINUE);
    Serial.println("Connection timeout.");
    connectionStuff();
    return "";
  }

  echoCommand(httpcmd, "OK", CONTINUE);  // GET
  line = "";
  long t = millis();
 // Serial1.setTimeout(10000);

 int s = 0;
  while ((millis() - t) < 5000) { //the request is processed quite fast, so 10 secs is enough
    if (Serial1.available()) {
     
      if (s<=16) //we dont need first 10 rows and if we are going to store - we will get memory overflow - so we will skip it
        {
          Serial.println(Serial1.readStringUntil('\r'));
                    s++;
        }
       else
        {
      line+= Serial1.readStringUntil('\r');
          Serial.println(line);}
    }
  }

      if (Serial1.available()) 
      {
             line+= Serial1.readString();
   //  Serial.println(Serial1.readStringUntil('\r'));
          Serial.println(line);
      }
  
  Serial.println("The following response has been received:");
  Serial.println(line);

  String result;

  if (line.indexOf("<title>") > 0 && line.indexOf("<title>") < line.indexOf("</title>"))
    result = line.substring(line.indexOf("<title>") + strlen("<title>"), line.indexOf("</title>"));
  else
    result = "";

  Serial.println("The resulting command is:");
  Serial.println(result);

  Serial1.println("AT+CIPCLOSE");
  getResponse();
  return result;
}




void getTime() { //connecting to my favorite google.com to get GMT time
  String line = "";
  //Serial.println("get time:");

  connectionStuff();

   if (isConnecctedToWifi==false)
    return; 
  //Builds the connection string for the ESP8266
  String cmd = "AT+CIPSTART=\"TCP\",\"google.com\",80";
  Serial1.println(cmd);  //Run the command
  Serial.println(cmd);    //Print this to the debug window
  delay(1000);

  getResponse();

  String httpcmd = "GET http://www.google.com/ HTTP/1.1\r\nHost:google.com\r\n\r\n";

  if (!echoCommand("AT+CIPSEND=" + String(httpcmd.length()), ">", CONTINUE))
  {
    echoCommand("AT+CIPCLOSE", "", CONTINUE);
    connectionStuff();
    Serial.println("Connection timeout.");
    return "";
  }

  echoCommand(httpcmd, "OK", CONTINUE);  // GET
  line = "";
  long t = millis();
  while ((millis() - t) < 5000) { //the request is processed quite fast, so 10 secs is enough
    if (Serial1.available()) {
      //parse everything received into a string
      line += Serial1.readStringUntil('\n');
    }
  }

  Serial.println("The following response has been received:");
  Serial.println(line);

  String result;
  if (line.indexOf("Date:") > 0 && line.indexOf("Date:") < line.indexOf("GMT"))
    result = line.substring(line.indexOf("Date:") + strlen("Date:"), line.indexOf("GMT"));
  else
    result = "";

  Serial.println("CurrentTime:");
  //   result = result.trim();
  Serial.println(result);
  setTimeFromInternet(result);
  Serial1.println("AT+CIPCLOSE");
  getResponse();

  return ;

}



void getResponse()//convert serial1(ESP) from char to string and output
{
  String retVal = "";
  char c;

  //DEBUG LOOP- display ESP output to serial Monitor.
  while (Serial1.available()) {
    c = Serial1.read();
    retVal += c;
  }
  Serial.println(retVal);
}




void connectionStuff()//rebot and connect to wifi
{
  
  isConnecctedToWifi = false;
  getResponse();

  Serial1.println("AT+RST");  //Issue Reset Command
  Serial.println("AT+RST");
  delay(1000);
  getResponse();


  Serial.println("AT+CWMODE=1");
  Serial1.println("AT+CWMODE=1");  //Set single client mode.
  delay(1000);
  getResponse();

 // boolean connection_established = false;
  for (int i = 0; i < 3; i++)
  {
    if (connectWiFi())
    {
      isConnecctedToWifi = true;
      break;
    }
  }

  Serial.println("AT+CIFSR");
  Serial1.println("AT+CIFSR"); //Display IP Information
  delay(1000);
  getResponse();

  Serial.println("AT+CIPMUX=0");
  Serial1.println("AT+CIPMUX=0");  //Sets up Single connection mode.
  delay(1000);
  getResponse();

}

void setTimeFromInternet(String response)//convert response from Google com to internal time
{
  int hour;
  int minute;
  int second;

  int i = response.indexOf(":");
  int j = response.indexOf(":", i + 2);
Serial.println("Setting time:");
Serial.println(i);
Serial.println(j);
Serial.println(response);
Serial.println(response.substring(i - 2, i ));
Serial.println(response.substring(i + 1, j ));
Serial.println(response.substring(j + 1, j + 3));

  if (i > 0 && j > 0 && (j - i) == 3)
  {
    hour = (response.substring(i - 2, i)).toInt();
    minute = (response.substring(i + 1, j )).toInt();
    second = (response.substring(j + 1, j + 3)).toInt();

    if (hour + GMT_OFFSET < 0)
      hour = hour + 24 - GMT_OFFSET;
    else
      hour = hour + GMT_OFFSET;
    setCurrentDayTime(hour, minute, second); // should works fine if hours>24
    TwitterPrintSchedule() ;
    isInternetTimeSet = true;
  }
}

void processInsertDelete(String response) //process schedule management
{
  int hour;
  int minute;
  int second;

  int i = response.indexOf(":");
  int j = response.indexOf(":", i + 2);
  
Serial.println("Adjusting schedule:");
Serial.println(i);
Serial.println(j);
Serial.println(response);
Serial.println(response.substring(i - 2, i ));
Serial.println(response.substring(i + 1, j ));
Serial.println(response.substring(j + 1, j + 3));

  if (i > 0 && j > 0 && (j - i) == 3)
  {
    hour = (response.substring(i - 2, i  )).toInt();
    minute = (response.substring(i + 1, j )).toInt();
    second = (response.substring(j + 1, j + 3)).toInt();

    if (response.startsWith("DELETE"))
      dropFromSchedule(hour, minute, second);

    if (response.startsWith("INSERT"))
      {
        dropFromSchedule(hour, minute, second);
        addToSchedule(hour, minute, second);
      }

    TwitterPrintSchedule() ;
  }
}

void processTwitterCommand (String command)//process command rececived from the wordpress
{
  Serial.println("processing command:");
  Serial.print(command);
  
  if (command == "STATUS")
    TwitterPrintSchedule();
  if (command.startsWith("DELETE") || command.startsWith("INSERT"))
    processInsertDelete(command);
  if (command.startsWith("NOW"))
  {
    doAction();
    makeTweet("ActionIsDone");
  }
}
